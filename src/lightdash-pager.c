/* Copyright (C) 2001 Havoc Pennington
 * Copyright (C) 2003 Kim Woelders
 * Copyright (C) 2003 Red Hat, Inc.
 * Copyright (C) 2003, 2005-2007 Vincent Untz
 * Copyright (C) 2015 adlo
 * 
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, see <http://www.gnu.org/licenses/>
 */
 
#include <gtk/gtk.h>
#include <libwnck/libwnck.h>
#include <cairo/cairo.h>
#include "lightdash-pager.h"
#include "lightdash-xutils.h"

struct _LightdashPagerPrivate
{
	WnckScreen *screen;
	
	GdkPixbuf *bg_cache;
	
	int n_rows;
};

G_DEFINE_TYPE (LightdashPager, lightdash_pager, GTK_TYPE_WIDGET);

#define LIGHTDASH_PAGER_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), LIGHTDASH_PAGER_TYPE, LightdashPagerPrivate))

#define POINT_IN_RECT(xcoord, ycoord, rect) \
 ((xcoord) >= (rect).x &&                   \
  (xcoord) <  ((rect).x + (rect).width) &&  \
  (ycoord) >= (rect).y &&                   \
  (ycoord) <  ((rect).y + (rect).height))

static void lightdash_pager_realize (GtkWidget *widget);
static GdkPixbuf*
lightdash_pager_get_background (LightdashPager *pager,
                           int        width,
                           int        height);
static void
lightdash_pager_drag_data_received (GtkWidget          *widget,
	  	               GdkDragContext     *context,
			       gint                x,
			       gint                y,
			       GtkSelectionData   *selection_data,
			       guint               info,
			       guint               time);

static gboolean
lightdash_pager_drag_drop  (GtkWidget        *widget,
		       GdkDragContext   *context,
		       gint              x,
		       gint              y,
		       guint             time);	
		       
static gboolean lightdash_pager_button_release (GtkWidget *widget, GdkEventButton *event);

static void lightdash_pager_active_workspace_changed
	(WnckScreen *screen, WnckWorkspace *previously_active_workspace, LightdashPager *pager);
	
static void lightdash_pager_workspace_created_callback
	(WnckScreen *screen, WnckWorkspace *space, LightdashPager *pager);

static void lightdash_pager_workspace_destroyed_callback
	(WnckScreen *screen, WnckWorkspace *space, LightdashPager *pager);		       	       

#if GTK_CHECK_VERSION (3, 0, 0)
static gboolean lightdash_pager_draw (GtkWidget *widget, cairo_t *cr);
#else
static gboolean lightdash_pager_expose_event (GtkWidget *widget, GdkEventExpose *event);
#endif

static void lightdash_pager_init (LightdashPager *pager)
{
	static const GtkTargetEntry targets [] = {
			{"application/x-wnck-window-id", 0, 0}
		};
		
	pager->priv = LIGHTDASH_PAGER_GET_PRIVATE (pager);
	
	pager->priv->screen = NULL;
	pager->priv->n_rows = 1;
	pager->priv->bg_cache = NULL;
	
	gtk_drag_dest_set (GTK_WIDGET (pager), GTK_DEST_DEFAULT_MOTION, 
		targets, G_N_ELEMENTS (targets), GDK_ACTION_MOVE);
	
	g_signal_connect (G_OBJECT(pager), "expose-event", 
		G_CALLBACK (lightdash_pager_expose_event), NULL);
		
	g_signal_connect (GTK_WIDGET (pager), "drag-data-received",
		G_CALLBACK (lightdash_pager_drag_data_received), NULL);
	
	g_signal_connect (GTK_WIDGET (pager), "drag-drop",
		G_CALLBACK (lightdash_pager_drag_drop), NULL);
		
	g_signal_connect (GTK_WIDGET (pager), "button-release-event",
		G_CALLBACK (lightdash_pager_button_release), NULL);
		
		
}

static void lightdash_pager_class_init (LightdashPagerClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
	
	g_type_class_add_private (klass, sizeof (LightdashPagerPrivate));
	
	widget_class->realize = lightdash_pager_realize;
	
}

static void lightdash_pager_realize (GtkWidget *widget)
{
	GdkWindowAttr attributes;
	gint attributes_mask;
	LightdashPager *pager;
	GtkAllocation allocation;
	GdkWindow *window;
	GtkStyle *style;
	GtkStyle *new_style;
	
	pager = LIGHTDASH_PAGER (widget);
	
	gtk_widget_set_realized (widget, TRUE);
	
	gtk_widget_get_allocation (widget, &allocation);
	
	attributes.window_type = GDK_WINDOW_CHILD;
	attributes.x = allocation.x;
	attributes.y = allocation.y;
	attributes.width = allocation.width;
	attributes.height = allocation.height;
	attributes.wclass = GDK_INPUT_OUTPUT;
	attributes.visual = gtk_widget_get_visual (widget);
	#if GTK_CHECK_VERSION (3, 0, 0)
	#else
	attributes.colormap = gtk_widget_get_colormap (widget);
	#endif
	attributes.event_mask = gtk_widget_get_events (widget) |GDK_EXPOSURE_MASK |
							GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK |
							GDK_LEAVE_NOTIFY_MASK | GDK_POINTER_MOTION_MASK |
							GDK_POINTER_MOTION_HINT_MASK;
	#if GTK_CHECK_VERSION (3, 0, 0)
	attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL;
	#else
	attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;
	#endif
	
	window = gdk_window_new (gtk_widget_get_parent_window (widget), &attributes, attributes_mask);
	gtk_widget_set_window (widget, window);
	gdk_window_set_user_data (window, widget);
	
	style = gtk_widget_get_style (widget);
	
	new_style = gtk_style_attach (style, window);
	if (new_style != style)
	{
		gtk_widget_set_style (widget, style);
		style = new_style;
	}
	
	gtk_style_set_background (style, window, GTK_STATE_NORMAL);
	
	if (pager->priv->screen == NULL)
		pager->priv->screen = wnck_screen_get_default ();
		
	g_signal_connect (pager->priv->screen, "active-workspace-changed",
            G_CALLBACK (lightdash_pager_active_workspace_changed), pager);
            
	g_signal_connect (pager->priv->screen, "workspace-created",
            G_CALLBACK (lightdash_pager_workspace_created_callback), pager);
            
	g_signal_connect (pager->priv->screen, "workspace-destroyed",
            G_CALLBACK (lightdash_pager_workspace_destroyed_callback), pager);                         
            
    gtk_widget_queue_draw (widget);
	
}

static void get_workspace_rect (LightdashPager *pager,
	int space, GdkRectangle *rect)
{
  int hsize, vsize;
  int n_spaces;
  int spaces_per_row;
  GtkWidget *widget;
  int col, row;
  GtkAllocation allocation;
  GtkStyle *style;
  int focus_width;

  widget = GTK_WIDGET (pager);

  gtk_widget_get_allocation (widget, &allocation);

  style = gtk_widget_get_style (widget);
  gtk_widget_style_get (widget,
			"focus-line-width", &focus_width,
			NULL);
  
  hsize = allocation.width - 2 * focus_width;
  vsize = allocation.height - 2 * focus_width;
  
  
  n_spaces = wnck_screen_get_workspace_count (pager->priv->screen);

  g_assert (pager->priv->n_rows > 0);
  spaces_per_row = (n_spaces + pager->priv->n_rows - 1) / pager->priv->n_rows;
      
      rect->width = (hsize - (pager->priv->n_rows - 1)) / pager->priv->n_rows;
      rect->height = (vsize - (spaces_per_row - 1)) / spaces_per_row;

      col = space / spaces_per_row;
      row = space % spaces_per_row;

      if (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL)
        col = pager->priv->n_rows - col - 1;

      rect->x = (rect->width + 1) * col; 
      rect->y = (rect->height + 1) * row;
      
      if (col == pager->priv->n_rows - 1)
	rect->width = hsize - rect->x;
      
      if (row  == spaces_per_row - 1)
	rect->height = vsize - rect->y;
	
	
  rect->x += focus_width;
  rect->y += focus_width;
  
}

static void
draw_window (GdkDrawable        *drawable,
             GtkWidget          *widget,
             WnckWindow         *win,
             const GdkRectangle *winrect,
             GtkStateType        state,
             gboolean            translucent)
{
  GtkStyle *style;
  cairo_t *cr;
  GdkPixbuf *icon;
  int icon_x, icon_y, icon_w, icon_h;
  gboolean is_active;
  GdkColor *color;
  gdouble translucency;

  style = gtk_widget_get_style (widget);

  is_active = wnck_window_is_active (win);
  translucency = translucent ? 0.4 : 1.0;

  cr = gdk_cairo_create (drawable);
  cairo_rectangle (cr, winrect->x, winrect->y, winrect->width, winrect->height);
  cairo_clip (cr);

  if (is_active)
    color = &style->light[state];
  else
    color = &style->bg[state];
  cairo_set_source_rgba (cr,
                         color->red / 65535.,
                         color->green / 65535.,
                         color->blue / 65535.,
                         translucency);
  cairo_rectangle (cr,
                   winrect->x + 1, winrect->y + 1,
                   MAX (0, winrect->width - 2), MAX (0, winrect->height - 2));
  cairo_fill (cr);

  icon = wnck_window_get_icon (win);

  icon_w = icon_h = 0;
          
  if (icon)
    {              
      icon_w = gdk_pixbuf_get_width (icon);
      icon_h = gdk_pixbuf_get_height (icon);

      /* If the icon is too big, fall back to mini icon.
       * We don't arbitrarily scale the icon, because it's
       * just too slow on my Athlon 850.
       */
      if (icon_w > (winrect->width - 2) ||
          icon_h > (winrect->height - 2))
        {
          icon = wnck_window_get_mini_icon (win);
          if (icon)
            {
              icon_w = gdk_pixbuf_get_width (icon);
              icon_h = gdk_pixbuf_get_height (icon);

              /* Give up. */
              if (icon_w > (winrect->width - 2) ||
                  icon_h > (winrect->height - 2))
                icon = NULL;
            }
        }
    }

  if (icon)
    {
      icon_x = winrect->x + (winrect->width - icon_w) / 2;
      icon_y = winrect->y + (winrect->height - icon_h) / 2;
                
      cairo_save (cr);
      gdk_cairo_set_source_pixbuf (cr, icon, icon_x, icon_y);
      cairo_rectangle (cr, icon_x, icon_y, icon_w, icon_h);
      cairo_clip (cr);
      cairo_paint_with_alpha (cr, translucency);
      cairo_restore (cr);
    }
          
  if (is_active)
    color = &style->fg[state];
  else
    color = &style->fg[state];
  cairo_set_source_rgba (cr,
                         color->red / 65535.,
                         color->green / 65535.,
                         color->blue / 65535.,
                         translucency);
  cairo_set_line_width (cr, 1.0);
  cairo_rectangle (cr,
                   winrect->x + 0.5, winrect->y + 0.5,
                   MAX (0, winrect->width - 1), MAX (0, winrect->height - 1));
  cairo_stroke (cr);

  cairo_destroy (cr);
}

static int
workspace_at_point (LightdashPager *pager,
                    int        x,
                    int        y,
                    int       *viewport_x,
                    int       *viewport_y)
{
  GtkWidget *widget;
  int i;
  int n_spaces;
  GtkAllocation allocation;
  int focus_width;
  int xthickness;
  int ythickness;

  widget = GTK_WIDGET (pager);

  gtk_widget_get_allocation (widget, &allocation);

  gtk_widget_style_get (GTK_WIDGET (pager),
			"focus-line-width", &focus_width,
			NULL);

  //if (pager->priv->shadow_type != GTK_SHADOW_NONE)
   //{
      GtkStyle *style;

      style = gtk_widget_get_style (widget);

      xthickness = focus_width + style->xthickness;
      ythickness = focus_width + style->ythickness;
    //}
  //else
    //{
      //xthickness = focus_width;
      //ythickness = focus_width;
    //}

  n_spaces = wnck_screen_get_workspace_count (pager->priv->screen);
  
  i = 0;
  while (i < n_spaces)
    {
      GdkRectangle rect;
      
      get_workspace_rect (pager, i, &rect);

      /* If workspace is on the edge, pretend points on the frame belong to the
       * workspace.
       * Else, pretend the right/bottom line separating two workspaces belong
       * to the workspace.
       */

      if (rect.x == xthickness)
        {
          rect.x = 0;
          rect.width += xthickness;
        }
      if (rect.y == ythickness)
        {
          rect.y = 0;
          rect.height += ythickness;
        }
      if (rect.y + rect.height == allocation.height - ythickness)
        {
          rect.height += ythickness;
        }
      else
        {
          rect.height += 1;
        }
      if (rect.x + rect.width == allocation.width - xthickness)
        {
          rect.width += xthickness;
        }
      else
        {
          rect.width += 1;
        }

      if (POINT_IN_RECT (x, y, rect))
        {
	  double width_ratio, height_ratio;
	  WnckWorkspace *space;

	  space = wnck_screen_get_workspace (pager->priv->screen, i);
          g_assert (space != NULL);

          /* Scale x, y mouse coords to corresponding screenwide viewport coords */
          
          width_ratio = (double) wnck_workspace_get_width (space) / (double) rect.width;
          height_ratio = (double) wnck_workspace_get_height (space) / (double) rect.height;

          if (viewport_x)
            *viewport_x = width_ratio * (x - rect.x);
          if (viewport_y)
            *viewport_y = height_ratio * (y - rect.y);

	  return i;
	}

      ++i;
    }

  return -1;
}

static gboolean
lightdash_pager_drag_drop  (GtkWidget        *widget,
		       GdkDragContext   *context,
		       gint              x,
		       gint              y,
		       guint             time)
{
  LightdashPager *pager = LIGHTDASH_PAGER (widget);
  GdkAtom target;
  
  target = gtk_drag_dest_find_target (widget, context, NULL);

  if (target != GDK_NONE)
    gtk_drag_get_data (widget, context, target, time);
  else 
    gtk_drag_finish (context, FALSE, FALSE, time);

  //wnck_pager_clear_drag (pager);
  
  return TRUE;
}

static void
lightdash_pager_drag_data_received (GtkWidget          *widget,
	  	               GdkDragContext     *context,
			       gint                x,
			       gint                y,
			       GtkSelectionData   *selection_data,
			       guint               info,
			       guint               time)
{
  LightdashPager *pager = LIGHTDASH_PAGER (widget);
  WnckWorkspace *space;
  GList *tmp;
  gint i;
  gulong xid;

  if ((gtk_selection_data_get_length (selection_data) != sizeof (gulong)) ||
      (gtk_selection_data_get_format (selection_data) != 8))
    {
      gtk_drag_finish (context, FALSE, FALSE, time);
      return;
    }
  
  i = workspace_at_point (pager, x, y, NULL, NULL);
  space = wnck_screen_get_workspace (pager->priv->screen, i);
  if (!space)
    {
      gtk_drag_finish (context, FALSE, FALSE, time);
      return;
    }
  
  xid = *((gulong *) gtk_selection_data_get_data (selection_data));
	      
  for (tmp = wnck_screen_get_windows_stacked (pager->priv->screen); tmp != NULL; tmp = tmp->next)
    {
      if (wnck_window_get_xid (tmp->data) == xid)
	{
	  WnckWindow *win = tmp->data;
	  wnck_window_move_to_workspace (win, space);
	  if (space == wnck_screen_get_active_workspace (pager->priv->screen))
	    wnck_window_activate (win, time);
	  gtk_drag_finish (context, TRUE, FALSE, time);
	  return;
	}
    }

  gtk_drag_finish (context, FALSE, FALSE, time);
}

static gboolean lightdash_pager_button_release (GtkWidget *widget, GdkEventButton *event)
{
	WnckWorkspace *space;
	LightdashPager *pager;
	int i;
	int j;
	int viewport_x;
	int viewport_y;
	
	if (event->button !=1)
		return FALSE;
		
	pager = LIGHTDASH_PAGER (widget);
	
	      i = workspace_at_point (pager,
                      event->x, event->y,
                      &viewport_x, &viewport_y);
                              
     if (space = wnck_screen_get_workspace (pager->priv->screen, i))
     { 
		 if (space != wnck_screen_get_active_workspace (pager->priv->screen))
			wnck_workspace_activate (space, event->time);
	}
		
	return FALSE;
		
}
			
static void lightdash_pager_draw_workspace (LightdashPager *pager,
	int workspace, GdkRectangle *rect, GdkPixbuf *bg_pixbuf)
{
	GdkWindow *window;
	WnckWorkspace *space;
	gboolean is_current;
	GtkStyle *style;
	GtkWidget *widget;
	GtkStateType state;
	
	space = wnck_screen_get_workspace (pager->priv->screen, workspace);
	widget = GTK_WIDGET (pager);
	is_current = (space == wnck_screen_get_active_workspace (pager->priv->screen));
	
	if (is_current)
		state = GTK_STATE_SELECTED;
	else
		state = GTK_STATE_NORMAL;
	
	window = gtk_widget_get_window (widget);
	style = gtk_widget_get_style (widget);
	
	if (bg_pixbuf)
	{
		gdk_draw_pixbuf (window,
			style->dark_gc[state],
			bg_pixbuf,
			0, 0,
			rect->x, rect->y,
			-1, -1,
			GDK_RGB_DITHER_MAX,
			0, 0);
	}
	else
    {
      cairo_t *cr;

      cr = gdk_cairo_create (window);

      if (!wnck_workspace_is_virtual (space))
        {
          gdk_cairo_set_source_color (cr, &style->dark[state]);
          cairo_rectangle (cr, rect->x, rect->y, rect->width, rect->height);
          cairo_fill (cr);
        }
      else
        {
          //FIXME prelight for dnd in the viewport?
          int workspace_width, workspace_height;
          int screen_width, screen_height;
          double width_ratio, height_ratio;
          double vx, vy, vw, vh; /* viewport */

          workspace_width = wnck_workspace_get_width (space);
          workspace_height = wnck_workspace_get_height (space);
          screen_width = wnck_screen_get_width (pager->priv->screen);
          screen_height = wnck_screen_get_height (pager->priv->screen);

          if ((workspace_width % screen_width == 0) &&
              (workspace_height % screen_height == 0))
            {
              int i, j;
              int active_i, active_j;
              int horiz_views;
              int verti_views;

              horiz_views = workspace_width / screen_width;
              verti_views = workspace_height / screen_height;

              /* do not forget thin lines to delimit "workspaces" */
              width_ratio = (rect->width - (horiz_views - 1)) /
                            (double) workspace_width;
              height_ratio = (rect->height - (verti_views - 1)) /
                             (double) workspace_height;

              if (is_current)
                {
                  active_i =
                    wnck_workspace_get_viewport_x (space) / screen_width;
                  active_j =
                    wnck_workspace_get_viewport_y (space) / screen_height;
                }
              else
                {
                  active_i = -1;
                  active_j = -1;
                }

              for (i = 0; i < horiz_views; i++)
                {
                  /* "+ i" is for the thin lines */
                  vx = rect->x + (width_ratio * screen_width) * i + i;

                  if (i == horiz_views - 1)
                    vw = rect->width + rect->x - vx;
                  else
                    vw = width_ratio * screen_width;

                  vh = height_ratio * screen_height;

                  for (j = 0; j < verti_views; j++)
                    {
                      /* "+ j" is for the thin lines */
                      vy = rect->y + (height_ratio * screen_height) * j + j;

                      if (j == verti_views - 1)
                        vh = rect->height + rect->y - vy;

                      if (active_i == i && active_j == j)
                        gdk_cairo_set_source_color (cr,
                                                    &style->dark[GTK_STATE_SELECTED]);
                      else
                        gdk_cairo_set_source_color (cr,
                                                    &style->dark[GTK_STATE_NORMAL]);
                      cairo_rectangle (cr, vx, vy, vw, vh);
                      cairo_fill (cr);
                    }
                }
            }
          else
            {
              width_ratio = rect->width / (double) workspace_width;
              height_ratio = rect->height / (double) workspace_height;

              /* first draw non-active part of the viewport */
              gdk_cairo_set_source_color (cr, &style->dark[GTK_STATE_NORMAL]);
              cairo_rectangle (cr, rect->x, rect->y, rect->width, rect->height);
              cairo_fill (cr);

              if (is_current)
                {
                  /* draw the active part of the viewport */
                  vx = rect->x +
                    width_ratio * wnck_workspace_get_viewport_x (space);
                  vy = rect->y +
                    height_ratio * wnck_workspace_get_viewport_y (space);
                  vw = width_ratio * screen_width;
                  vh = height_ratio * screen_height;

                  gdk_cairo_set_source_color (cr, &style->dark[GTK_STATE_SELECTED]);
                  cairo_rectangle (cr, vx, vy, vw, vh);
                  cairo_fill (cr);
                }
            }
        }

      cairo_destroy (cr);
    }
		
}
	
#if GTK_CHECK_VERSION (3, 0, 0)
static gboolean lightdash_pager_draw (GtkWidget *widget, cairo_t *cr)
#else
static gboolean lightdash_pager_expose_event (GtkWidget *widget, GdkEventExpose *event)
#endif
{
	LightdashPager *pager;
	int i;
	int n_spaces;
	WnckWorkspace *active_space;
	GdkPixbuf *bg_pixbuf;
	gboolean first;
	#if GTK_CHECK_VERSION (3, 0, 0)
	#else
	GdkWindow *window;
	GtkAllocation allocation;
	#endif
	GtkStyle *style;
	int focus_width;
	
	bg_pixbuf = NULL;
	
	pager = LIGHTDASH_PAGER (widget);
	
	n_spaces = wnck_screen_get_workspace_count (pager->priv->screen);
	active_space = wnck_screen_get_active_workspace (pager->priv->screen);
	bg_pixbuf = NULL;
	first = TRUE;
	
	#if GTK_CHECK_VERSION (3, 0, 0)
	#else
	window = gtk_widget_get_window  (widget);
	gtk_widget_get_allocation (widget, &allocation);
	#endif
	
	style = gtk_widget_get_style (widget);
	gtk_widget_style_get (widget,
							"focus-line-width", &focus_width,
							NULL);
												
	if (gtk_widget_has_focus (widget))
	#if GTK_CHECK_VERSION (3, 0, 0)	
		gtk_paint_focus (style,
						cr,
						gtk_widget_get_state (widget),
						widget,
						"pager",
						0, 0,
						gtk_widget_get_allocated_width (widget),
						gtk_widget_get_allocated_height (widget));
	#else
		gtk_paint_focus (style,
						window,
						gtk_widget_get_state (widget),
						NULL,
						widget,
						"pager",
						0, 0,
						allocation.width,
						allocation.height);
	#endif
	
	i = 0;
	while (i < n_spaces)
	{
		GdkRectangle rect, intersect;
		
		get_workspace_rect (pager, i, &rect);
		if (first)
		{
			bg_pixbuf = lightdash_pager_get_background (pager,
														rect.width,
														rect.height);
			first = FALSE;
		}
		lightdash_pager_draw_workspace (pager, i, &rect, bg_pixbuf);
		
		++i;
	}
	
	
	
	return FALSE;	
	
}

static GdkPixbuf*
lightdash_pager_get_background (LightdashPager *pager,
                           int        width,
                           int        height)
{
  Pixmap p;
  GdkPixbuf *pix = NULL;
  
  /* We have to be careful not to keep alternating between
   * width/height values, otherwise this would get really slow.
   */
  if (pager->priv->bg_cache &&
      gdk_pixbuf_get_width (pager->priv->bg_cache) == width &&
      gdk_pixbuf_get_height (pager->priv->bg_cache) == height)
    return pager->priv->bg_cache;

  if (pager->priv->bg_cache)
    {
      g_object_unref (G_OBJECT (pager->priv->bg_cache));
      pager->priv->bg_cache = NULL;
    }

  if (pager->priv->screen == NULL)
    return NULL;


  /* FIXME this just globally disables the thumbnailing feature */
  return NULL;
  
#define MIN_BG_SIZE 10
  
  if (width < MIN_BG_SIZE || height < MIN_BG_SIZE)
    return NULL;
  
  p = wnck_screen_get_background_pixmap (pager->priv->screen);
  
  if (p != None)
  {
      pix = _lightdash_gdk_pixbuf_get_from_pixmap (NULL,
                                              p,
                                              0, 0, 0, 0,
                                              -1, -1);
  }


  if (pix)
    {
      pager->priv->bg_cache = gdk_pixbuf_scale_simple (pix,
                                                       width,
                                                       height,
                                                       GDK_INTERP_BILINEAR);

      g_object_unref (G_OBJECT (pix));
    }

  return pager->priv->bg_cache;
}

static void lightdash_pager_active_workspace_changed
	(WnckScreen *screen, WnckWorkspace *previously_active_workspace, LightdashPager *pager)
{
	gtk_widget_queue_draw (GTK_WIDGET (pager));
}

static void lightdash_pager_workspace_created_callback
	(WnckScreen *screen, WnckWorkspace *space, LightdashPager *pager)
{
	gtk_widget_queue_draw (GTK_WIDGET (pager));
}

static void lightdash_pager_workspace_destroyed_callback
	(WnckScreen *screen, WnckWorkspace *space, LightdashPager *pager)
{
	gtk_widget_queue_draw (GTK_WIDGET (pager));
}

GtkWidget * lightdash_pager_new ()
{
	return GTK_WIDGET(g_object_new (lightdash_pager_get_type (), NULL));
}
