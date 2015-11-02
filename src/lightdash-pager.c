/* Copyright (C) 2015 adlo
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

struct _LightdashPagerPrivate
{
	WnckScreen *screen;
	
	int n_rows;
};

G_DEFINE_TYPE (LightdashPager, lightdash_pager, GTK_TYPE_WIDGET);

#define LIGHTDASH_PAGER_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), LIGHTDASH_PAGER_TYPE, LightdashPagerPrivate))

static void lightdash_pager_realize (GtkWidget *widget);

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
	
	g_signal_connect (G_OBJECT(pager), "expose-event", 
		G_CALLBACK (lightdash_pager_expose_event), NULL);
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
			
static void lightdash_pager_draw_workspace (LightdashPager *pager,
	int workspace, GdkRectangle *rect, GdkPixbuf *bg_pixbuf)
{
	GdkWindow *window;
	WnckWorkspace *space;
	GtkStyle *style;
	GtkWidget *widget;
	GtkStateType *state;
	
	space = wnck_screen_get_workspace (pager->priv->screen, workspace);
	widget = GTK_WIDGET (pager);
	
	state = GTK_STATE_NORMAL;
	
	window = gtk_widget_get_window (widget);
	style = gtk_widget_get_style (widget);
	
	gdk_draw_pixbuf (window,
		style->dark_gc[(int)state],
		bg_pixbuf,
		0, 0,
		rect->x, rect->y,
		-1, -1,
		GDK_RGB_DITHER_MAX,
		0, 0);
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
		lightdash_pager_draw_workspace (pager, i, &rect, bg_pixbuf);
	}
		
	
}

GtkWidget * lightdash_pager_new ()
{
	return GTK_WIDGET(g_object_new (lightdash_pager_get_type (), NULL));
}
