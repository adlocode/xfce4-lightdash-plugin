/*
 * Copyright (C) 2011 Nick Schermer <nick@xfce.org>
 * 
 * Copyright (C) 2015-2016, 2018 adlo
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif
#include <libxfce4util/libxfce4util.h>
#include <libxfce4ui/libxfce4ui.h>
#include <gdk/gdkkeysyms.h>
#include <xfconf/xfconf.h>
#include <glib/gstdio.h>
#include <libwnck/libwnck.h>

#include <cairo/cairo.h>

#include <src/appfinder-window.h>
#include <src/appfinder-model.h>
#include <src/appfinder-category-model.h>
#include <src/appfinder-preferences.h>
#include <src/appfinder-actions.h>
#include <src/appfinder-private.h>
#include "lightdash.h"
#include "lightdash-window-switcher.h"
#include "lightdash-pager.h"
#include "table-layout.h"
#include "lightdash-composited-window.h"

#if GTK_CHECK_VERSION (3, 0, 0)
#else
#include <exo/exo.h>
#endif

#ifdef GDK_WINDOWING_X11
#include <gdk/gdkx.h>
#define APPFINDER_WIDGET_XID(widget) ((guint) GDK_WINDOW_XID (gtk_widget_get_window (GTK_WIDGET (widget))))
#else
#define APPFINDER_WIDGET_XID(widget) (0)
#endif



#define DEFAULT_WINDOW_WIDTH   400
#define DEFAULT_WINDOW_HEIGHT  400
#define DEFAULT_PANED_POSITION 180


//static gboolean            opt_collapsed = FALSE;




static GSList             *windows = NULL;
static gboolean            service_owner = FALSE;
static XfceAppfinderModel *model_cache = NULL;

#ifdef DEBUG
//static GHashTable         *objects_table = NULL;
//static guint               objects_table_count = 0;
#endif

enum
{
	PROP_PLUGIN = 1,
	N_PROPERTIES
};

static GParamSpec *obj_properties[N_PROPERTIES] = {NULL, };

static void       xfce_appfinder_window_finalize                      (GObject                     *object);

static void appfinder_window_destroyed (GtkWidget *window);
//static void       xfce_appfinder_window_unmap                         (GtkWidget                   *widget);
static gboolean   xfce_appfinder_window_key_press_event               (GtkWidget                   *widget,
                                                                       GdkEventKey                 *event);
static gboolean xfce_lightdash_window_key_press_event_after (GtkWidget   *widget,
							     GdkEventKey *event,
							     XfceAppfinderWindow *window);
static gboolean   xfce_appfinder_window_window_state_event            (GtkWidget                   *widget,
                                                                       GdkEventWindowState         *event);
static void       xfce_appfinder_window_view                          (XfceAppfinderWindow         *window);
static gboolean   xfce_appfinder_window_popup_menu                    (GtkWidget                   *view,
                                                                       XfceAppfinderWindow         *window);
static void       xfce_appfinder_window_set_padding                   (GtkWidget                   *entry,
                                                                       GtkWidget                   *align);
static gboolean   xfce_appfinder_window_completion_match_func         (GtkEntryCompletion          *completion,
                                                                       const gchar                 *key,
                                                                       GtkTreeIter                 *iter,
                                                                       gpointer                     data);
static void       xfce_appfinder_window_entry_changed                 (XfceAppfinderWindow         *window);
static void       xfce_appfinder_window_entry_activate                (GtkEditable                 *entry,
                                                                       XfceAppfinderWindow         *window);
static void       xfce_appfinder_window_entry_icon_released           (GtkEntry                    *entry,
                                                                       GtkEntryIconPosition         icon_pos,
                                                                       GdkEvent                    *event,
                                                                       XfceAppfinderWindow         *window);
static void       xfce_appfinder_window_drag_begin                    (GtkWidget                   *widget,
                                                                       GdkDragContext              *drag_context,
                                                                       XfceAppfinderWindow         *window);
static void       xfce_appfinder_window_drag_data_get                 (GtkWidget                   *widget,
                                                                       GdkDragContext              *drag_context,
                                                                       GtkSelectionData            *data,
                                                                       guint                        info,
                                                                       guint                        drag_time,
                                                                       XfceAppfinderWindow         *window);
static gboolean   xfce_appfinder_window_treeview_key_press_event      (GtkWidget                   *widget,
                                                                       GdkEventKey                 *event,
                                                                       XfceAppfinderWindow         *window);
static gboolean   lightdash_window_treeview_key_release_event         (GtkWidget           *widget,
                                                                       GdkEventKey         *event,
                                                                        XfceAppfinderWindow *window);
static void       xfce_appfinder_window_category_changed              (GtkTreeSelection            *selection,
                                                                       XfceAppfinderWindow         *window);
static void       xfce_appfinder_window_category_set_categories       (XfceAppfinderModel          *model,
                                                                       XfceAppfinderWindow         *window);
static void       xfce_lightdash_window_preferences                   (GtkWidget                   *button,
                                                                       XfceAppfinderWindow         *window);
static void       xfce_appfinder_window_property_changed              (XfconfChannel               *channel,
                                                                       const gchar                 *prop,
                                                                       const GValue                *value,
                                                                       XfceAppfinderWindow         *window);
static gboolean   xfce_appfinder_window_item_visible                  (GtkTreeModel                *model,
                                                                       GtkTreeIter                 *iter,
                                                                       gpointer                     data);
static void       xfce_appfinder_window_item_changed                  (XfceAppfinderWindow         *window);
static void       xfce_appfinder_window_row_activated                 (XfceAppfinderWindow         *window);
static void       xfce_appfinder_window_icon_theme_changed            (XfceAppfinderWindow         *window);
static void       xfce_appfinder_window_launch_clicked                (XfceAppfinderWindow         *window);
static void       xfce_appfinder_window_execute                       (XfceAppfinderWindow         *window,
 
                                                                      gboolean                     close_on_succeed);
static void       xfce_appfinder_window_popup_menu_toggle_bookmark    (GtkWidget           *mi,
                                                                        XfceAppfinderWindow *window);
static gint       lightdash_window_sort_items                    (GtkTreeModel                *model,
                                                                       GtkTreeIter                 *a,
                                                                       GtkTreeIter                 *b,
                                                                       gpointer                     data);
static void lightdash_window_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);
static void lightdash_window_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void lightdash_window_constructed (GObject *object);
static void lightdash_window_realize (GtkWidget *widget);                                                                  
                                                                                                                                                                                                    
gboolean
xfce_lightdash_window_expose (GtkWidget *widget, GdkEvent *event, XfceAppfinderWindow *window);

void
xfce_lightdash_window_screen_changed (GtkWidget *widget, GdkScreen *wscreen, XfceAppfinderWindow *window);


struct _XfceAppfinderWindowClass
{
  GtkWindowClass __parent__;
};

struct _XfceAppfinderWindow
{
  GtkWindow __parent__;
  
  LightdashPlugin *lightdash_plugin;

  XfceAppfinderModel         *model;

  GtkTreeModel               *sort_model;
  GtkTreeModel               *filter_model;

  XfceAppfinderCategoryModel *category_model;

  XfceAppfinderActions       *actions;

  GtkIconTheme               *icon_theme;

  GtkEntryCompletion         *completion;

  XfconfChannel              *channel;

  GtkWidget                  *paned;
  GtkWidget                  *entry;
  //GtkWidget                  *image;
  GtkWidget                  *view;
  GtkWidget                  *viewscroll;
  GtkWidget					 *scroll;
  GtkWidget                  *sidepane;
  GtkWidget					 *taskview_container;
  GtkWidget					 *window_switcher;
  GtkWidget					 *apps_button;
  GList						 *bookmarks_buttons;
  GtkWidget					 *pager;	
  GtkWidget					 *icon_bar;

  GdkPixbuf                  *icon_find;

  GtkWidget                  *bbox;
  GtkWidget                  *button_launch;
  GtkWidget                  *button_preferences;
  GtkWidget                  *bin_collapsed;
  GtkWidget                  *bin_expanded;

  GarconMenuDirectory        *filter_category;
  gchar                      *filter_text;

  guint                       idle_entry_changed_id;

  gint                        last_window_height;

  gulong                      property_watch_id;
  gulong                      categories_changed_id;
  gboolean					  supports_alpha;
  
  WnckWindow				  *root;
  WnckWindow				  *wnck_window;
  LightdashCompositedWindow *cw;
  LightdashCompositor *compositor;
  WnckScreen *screen;
};



static const GtkTargetEntry target_list[] =
{
  { "text/uri-list", 0, 0 }
};



G_DEFINE_TYPE (XfceAppfinderWindow, xfce_appfinder_window, GTK_TYPE_WINDOW)



static void
xfce_appfinder_window_class_init (XfceAppfinderWindowClass *klass)
{
  GObjectClass   *gobject_class;
  GtkWidgetClass *gtkwidget_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = xfce_appfinder_window_finalize;
  gobject_class->set_property = lightdash_window_set_property;
  gobject_class->get_property = lightdash_window_get_property;
  gobject_class->constructed = lightdash_window_constructed;

  gtkwidget_class = GTK_WIDGET_CLASS (klass);
  //gtkwidget_class->unmap = xfce_appfinder_window_unmap;
  gtkwidget_class->key_press_event = xfce_appfinder_window_key_press_event;
  gtkwidget_class->window_state_event = xfce_appfinder_window_window_state_event;
  gtkwidget_class->realize = lightdash_window_realize;
  
  obj_properties[PROP_PLUGIN] =
	g_param_spec_pointer ("plugin",
						"Xfce panel plugin",
						"Xfce panel plugin",
						G_PARAM_CONSTRUCT | G_PARAM_READWRITE);
						
  g_object_class_install_properties (gobject_class,
									N_PROPERTIES,
									obj_properties);
}
static void
xfce_lightdash_window_apps_button_toggled (GtkToggleButton *button, XfceAppfinderWindow *window)
{
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button)))
        {
			gtk_widget_show_all (window->paned);
			gtk_widget_hide (window->taskview_container);
		}
		
		else
		{
			gtk_widget_hide (window->paned);
			gtk_widget_show_all (window->taskview_container);
			
		}
}
	
static void
xfce_lightdash_window_show (GtkWidget *widget, XfceAppfinderWindow *window)
{
	gtk_window_set_keep_above (GTK_WINDOW (window), TRUE);
	
	gtk_widget_hide (window->paned);
	gtk_widget_show_all (window->taskview_container);
	gtk_entry_set_text (GTK_ENTRY(window->entry), "");
	gtk_widget_grab_focus (window->entry);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (window->apps_button), FALSE);
	gtk_window_maximize (GTK_WINDOW (window));
	
	if (window->root && !window->cw && window->lightdash_plugin->show_desktop)
	{
		window->cw = lightdash_composited_window_new_from_window (window->root);
		g_signal_connect_swapped (window->cw, "damage-event",
					G_CALLBACK (gtk_widget_queue_draw), GTK_WIDGET (window));
		
	}
	else if (window->cw && window->lightdash_plugin->show_desktop == FALSE)
	{
		g_object_unref (window->cw);
		window->cw = NULL;
	}

}

static void lightdash_window_bookmark_button_clicked (GtkButton *button, gpointer data)
{
	LightdashBookmark *bookmark;

  GtkTreeModel *model;
  GtkTreeIter   orig;
  GarconMenuItem *item;
  const gchar     *command, *p;
  GString         *string;
  gboolean         succeed = FALSE;
  GError       *error = NULL;
  gboolean      result = FALSE;
  GdkScreen    *screen = NULL;
  const gchar  *text;
  gchar        **argv;
  gchar        *cmd = NULL;
  gboolean      regular_command = FALSE;
  gboolean      save_cmd;
  gboolean      only_custom_cmd = FALSE;

  bookmark = data;
  item = bookmark->item;

  succeed = lightdash_model_execute_menu_item (model, item, screen,
                                               &regular_command, &error);

  gtk_widget_hide (GTK_WIDGET (bookmark->window));

}

void lightdash_window_bookmark_menu_open_activate (GtkMenuItem       *menuitem,
                                                   LightdashBookmark *bookmark)
{
  GtkTreeModel *model;
  GtkTreeIter   orig;
  GarconMenuItem *item;
  const gchar     *command, *p;
  GString         *string;
  gboolean         succeed = FALSE;
  GError       *error = NULL;
  gboolean      result = FALSE;
  GdkScreen    *screen = NULL;
  const gchar  *text;
  gchar        **argv;
  gchar        *cmd = NULL;
  gboolean      regular_command = FALSE;
  gboolean      save_cmd;
  gboolean      only_custom_cmd = FALSE;

  item = bookmark->item;

  succeed = lightdash_model_execute_menu_item (model, item, screen,
                                               &regular_command, &error);

  gtk_widget_hide (GTK_WIDGET (bookmark->window));

}

static gboolean lightdash_window_bookmark_popup_menu (GtkWidget *widget,
                                                      GdkEventButton *event,
                                                      LightdashBookmark *bookmark)
{
  if (event->button == 3)
	{
    GtkWidget *menu = gtk_menu_new ();
    gchar *uri;
    gtk_menu_attach_to_widget (GTK_MENU (menu), bookmark->button, NULL);

    uri = garcon_menu_item_get_uri (bookmark->item);
    g_object_set_data_full (G_OBJECT (menu), "uri", uri, g_free);

    GtkWidget *open = gtk_menu_item_new_with_label (_("Open New Window"));
    gtk_menu_attach (GTK_MENU (menu), open,
                     0, 1, 0, 1);
    g_signal_connect (G_OBJECT (open), "activate",
                      G_CALLBACK (lightdash_window_bookmark_menu_open_activate),
                      bookmark);
    gtk_widget_show (open);

    GtkWidget *remove = gtk_menu_item_new_with_label (_("Remove from Bookmarks"));
    gtk_menu_attach (GTK_MENU (menu), remove,
                     0, 1, 1, 2);
    g_signal_connect (G_OBJECT (remove), "activate",
                      G_CALLBACK (xfce_appfinder_window_popup_menu_toggle_bookmark),
                      bookmark->window);
    gtk_widget_show (remove);

		GdkEventButton *event_button = (GdkEventButton *) event;
    gtk_menu_popup_at_widget (GTK_MENU (menu), bookmark->button,
                              GDK_GRAVITY_EAST, GDK_GRAVITY_WEST, NULL);

		return TRUE;
	}
  return FALSE;
}

static void lightdash_window_bookmark_added (XfceAppfinderModel  *model,
                                             LightdashBookmark         *bookmark,
                                             XfceAppfinderWindow *window)
{
	GtkWidget *button;
	gboolean valid;
	GdkPixbuf *icon_large;
	gboolean is_bookmark;
	//LightdashBookmark *bookmark;

	lightdash_table_layout_attach_next (bookmark->button, LIGHTDASH_TABLE_LAYOUT (window->icon_bar));
	window->bookmarks_buttons = g_list_prepend (window->bookmarks_buttons, bookmark);
	gtk_widget_set_size_request (bookmark->button, 70, 70);
	bookmark->model = model;
	bookmark->window = GTK_WIDGET (window);

	g_signal_connect (bookmark->button, "clicked",
		G_CALLBACK (lightdash_window_bookmark_button_clicked), bookmark);
  g_signal_connect (bookmark->button, "button-press-event",
		G_CALLBACK (lightdash_window_bookmark_popup_menu), bookmark);
}

static void lightdash_window_bookmark_removed (XfceAppfinderModel  *model,
                                             GarconMenuItem *item,
                                             XfceAppfinderWindow *window)
{
	GList *li;

	for (li = window->bookmarks_buttons; li != NULL; li = li->next)
	{
		LightdashBookmark *bookmark = (LightdashBookmark *)li->data;
		if (bookmark->item == item)
		{
			window->bookmarks_buttons = g_list_remove (window->bookmarks_buttons, (gconstpointer) bookmark);
			lightdash_bookmark_free (bookmark);
      bookmark = NULL;
		}
	}
}


static void lightdash_window_bookmarks_changed (XfceAppfinderModel *model, XfceAppfinderWindow *window)
{
	GtkWidget *button;
	gboolean valid;
	GtkTreeIter iter;
	GdkPixbuf *icon_large;
	gboolean is_bookmark;
	LightdashBookmark *bookmark;
	
	g_list_free_full (window->bookmarks_buttons, (GDestroyNotify) lightdash_bookmark_free);
	lightdash_table_layout_set_position (LIGHTDASH_TABLE_LAYOUT (window->icon_bar), 0, 1);
	window->bookmarks_buttons = NULL;

	valid = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (model), &iter);

	while (valid)
	{

		gtk_tree_model_get (GTK_TREE_MODEL (model), &iter,
					XFCE_APPFINDER_MODEL_COLUMN_ICON_LARGE, &icon_large,
					XFCE_APPFINDER_MODEL_COLUMN_BOOKMARK, &is_bookmark,
					-1);
					
		if (is_bookmark)
		{
			bookmark = g_slice_new0 (LightdashBookmark);
			GtkWidget *image = gtk_image_new_from_pixbuf (icon_large);
			button = gtk_button_new ();
			gtk_container_add (GTK_CONTAINER (button), image);
			gtk_widget_show (image);
			lightdash_table_layout_attach_next (button, LIGHTDASH_TABLE_LAYOUT (window->icon_bar));
			window->bookmarks_buttons = g_list_prepend (window->bookmarks_buttons, bookmark);
			gtk_widget_set_size_request (button, 70, 70);
			gtk_widget_show (button);
			g_object_unref (icon_large);
			bookmark->button = button;
			//bookmark->iter = iter;
			bookmark->model = model;
			bookmark->window = window;
			
			g_signal_connect (button, "clicked",
				G_CALLBACK (lightdash_window_bookmark_button_clicked), bookmark);
		}

		valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (model), &iter);
	}
}

void
xfce_lightdash_window_screen_changed (GtkWidget *widget, GdkScreen *wscreen, XfceAppfinderWindow *window)
{
	GdkScreen *screen;
	#if GTK_CHECK_VERSION (3, 0, 0)
	GdkVisual *visual;
	#else
	GdkColormap *colormap;
	#endif
	
	screen = gtk_widget_get_screen (widget);
	
	#if GTK_CHECK_VERSION (3, 0, 0)
	visual = gdk_screen_get_rgba_visual (screen);

	if (!visual)
	{
		visual = gdk_screen_get_system_visual (screen);
		window->supports_alpha = FALSE;
	}
	else
	{
		window->supports_alpha = TRUE;
	}
	
	gtk_widget_set_visual (widget, visual);
	#else
	colormap = gdk_screen_get_rgba_colormap (screen);
	if (!colormap)
	{
		colormap = gdk_screen_get_system_colormap (screen);
		window->supports_alpha = FALSE;
	}
	else
	{
		window->supports_alpha = TRUE;
	}
	
	gtk_widget_set_colormap (widget, colormap);
	#endif
}

static void lightdash_window_workspace_changed (WnckScreen *screen, WnckWindow *previous, XfceAppfinderWindow *window)
{
	if (gtk_widget_get_visible (GTK_WIDGET (window)))
		gdk_window_focus (gtk_widget_get_window (GTK_WIDGET (window)), GDK_CURRENT_TIME);
		
		/* gtk_window_present () doesn't seem to work in this instance */

}

#if GTK_CHECK_VERSION (3, 0, 0)

gboolean
xfce_lightdash_window_draw (GtkWidget *widget, cairo_t *cr, XfceAppfinderWindow *window)
#else

gboolean
xfce_lightdash_window_expose (GtkWidget *widget, GdkEvent *event, XfceAppfinderWindow *window)
#endif
{
	GtkStyle *style;
	GdkColor color;
	GdkWindow *gdk_win;
	int x, y;
	#if GTK_CHECK_VERSION (3, 0, 0)
	#else
	cairo_t *cr;
	#endif
	
	
	if (!gtk_widget_get_realized (widget))
	{
		gtk_widget_realize (widget);
	}
	
	style = gtk_widget_get_style (widget);
	
	if (style == NULL)
	{
		return FALSE;
	}
	
	color = style->bg[GTK_STATE_NORMAL];
	#if GTK_CHECK_VERSION (3, 0, 0)
	#else
	cr = gdk_cairo_create (gtk_widget_get_window (widget));
	#endif
	
	if (window->cw && window->lightdash_plugin->show_desktop)
	{
		gdk_win = gtk_widget_get_window (widget);
		gdk_window_get_origin (gdk_win, &x, &y);
		cairo_set_source_surface (cr,
						window->cw->surface,
						-x, -y);
		cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
		cairo_paint (cr);
	}
	
	if (window->supports_alpha)
	{
	cairo_set_source_rgba (cr, color.red/65535.0, 
							color.green/65535.0, 
							color.blue/65535.0, 
							window->lightdash_plugin->opacity/100.0);
	}
	else
	{
		cairo_set_source_rgb (cr, color.red/65535.0, 
							color.green/65535.0, 
							color.blue/65535.0);
	}
	
	if (window->cw && window->lightdash_plugin->show_desktop)						
		cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
	else
		cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
	cairo_paint (cr);
	
	#if GTK_CHECK_VERSION (3, 0, 0)
	#else
	cairo_destroy (cr);
	#endif
	
	return FALSE;
	
}
	
	
static void
xfce_appfinder_window_init (XfceAppfinderWindow *window)
{
  GtkWidget          *vbox, *vbox2;
  GtkWidget          *entry;
  GtkWidget          *pane;
  GtkWidget          *scroll;
  GtkWidget          *sidepane;
  GtkWidget          *image;
  GtkWidget          *hbox;
  GtkWidget			 *main_hbox;
  GtkWidget          *align;
  GtkTreeViewColumn  *column;
  GtkCellRenderer    *renderer;
  GtkTreeSelection   *selection;
  GtkWidget          *bbox;
  GtkWidget          *button;
  GtkEntryCompletion *completion;
  gint                integer;
   GtkWidget *icon_apps;
    
    window->cw = NULL;
    window->root = NULL;
    window->supports_alpha = FALSE;
    window->bookmarks_buttons = NULL;
    
    gtk_window_maximize (GTK_WINDOW (window));
	gtk_window_set_decorated (GTK_WINDOW (window), FALSE);
    gtk_window_set_skip_taskbar_hint (GTK_WINDOW (window), TRUE);
    gtk_window_set_skip_pager_hint (GTK_WINDOW (window), TRUE);
    gtk_window_stick (GTK_WINDOW (window));
    gtk_window_set_modal (GTK_WINDOW (window), TRUE);
    gtk_window_set_keep_above (GTK_WINDOW (window), TRUE);
    gtk_widget_set_name (GTK_WIDGET (window), "lightdash-window");
    gtk_widget_set_app_paintable (GTK_WIDGET (window), TRUE);
	
  window->channel = xfconf_channel_get ("xfce4-lightdash");
  window->last_window_height = xfconf_channel_get_int (window->channel, "/last/window-height", DEFAULT_WINDOW_HEIGHT);

  window->category_model = xfce_appfinder_category_model_new ();
  xfconf_g_property_bind (window->channel, "/category-icon-size", G_TYPE_UINT,
                          G_OBJECT (window->category_model), "icon-size");

  window->model = xfce_appfinder_model_get ();
  xfconf_g_property_bind (window->channel, "/item-icon-size", G_TYPE_UINT,
                          G_OBJECT (window->model), "icon-size");
	

  gtk_window_set_title (GTK_WINDOW (window), _("Application Finder"));
  //integer = xfconf_channel_get_int (window->channel, "/last/window-width", DEFAULT_WINDOW_WIDTH);
  integer = DEFAULT_WINDOW_WIDTH;
  gtk_window_set_default_size (GTK_WINDOW (window), integer, -1);
  gtk_window_set_icon_name (GTK_WINDOW (window), XFCE_APPFINDER_STOCK_EXECUTE);
#if GTK_CHECK_VERSION (3, 0, 0)
  gtk_window_set_has_resize_grip (GTK_WINDOW (window), FALSE);
#endif

  //if (xfconf_channel_get_bool (window->channel, "/always-center", FALSE))
    //gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_CENTER);

#if GTK_CHECK_VERSION (3, 0, 0)
  main_hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_widget_set_margin_top (main_hbox, 6);
  gtk_widget_set_margin_bottom (main_hbox, 30);
  gtk_widget_set_margin_start (main_hbox, 1);
  gtk_widget_set_margin_end (main_hbox, 3);
  gtk_container_add (GTK_CONTAINER (window), main_hbox);
  
#else
  main_hbox = gtk_hbox_new (FALSE, 6);
  vbox = gtk_vbox_new (FALSE, 6);
  align = gtk_alignment_new (0.0, 0.0, 1.0, 1.0);
  gtk_alignment_set_padding (GTK_ALIGNMENT (align), 6, 30, 1, 3);
  gtk_container_add (GTK_CONTAINER (window), align);
  gtk_container_add (GTK_CONTAINER (align), main_hbox);
  gtk_widget_show (align);
#endif
  
  gtk_widget_show (main_hbox);
  
  //#if GTK_CHECK_VERSION (3, 0, 0)
  //window->icon_bar = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  //#else
  window->icon_bar = lightdash_table_layout_new (1, 1, TRUE);
  //#endif
  
  gtk_box_pack_start (GTK_BOX (main_hbox), window->icon_bar, FALSE, FALSE, 5);
  gtk_box_pack_start (GTK_BOX (main_hbox), vbox, TRUE, TRUE, 0);
  
  window->apps_button = gtk_toggle_button_new ();
  gtk_widget_set_tooltip_text (window->apps_button, _("Show Applications"));
  gtk_button_set_focus_on_click (GTK_BUTTON (window->apps_button), FALSE);
  gtk_widget_set_size_request (window->apps_button, 70, 70);
  lightdash_table_layout_attach_next (window->apps_button, LIGHTDASH_TABLE_LAYOUT (window->icon_bar));
  gtk_widget_show (window->apps_button);
  
  g_signal_connect (G_OBJECT (window->apps_button), "toggled",
      G_CALLBACK (xfce_lightdash_window_apps_button_toggled), window);
          
  g_signal_connect (G_OBJECT (window), "show",
      G_CALLBACK (xfce_lightdash_window_show), window);
      
  g_signal_connect (G_OBJECT (window), "screen-changed",
      G_CALLBACK (xfce_lightdash_window_screen_changed), window);
      
  xfce_lightdash_window_screen_changed (GTK_WIDGET (window), NULL, window);
  
  gtk_widget_show (vbox);
  gtk_widget_show (window->icon_bar);

#if GTK_CHECK_VERSION (3, 0, 0)
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
#else
  hbox = gtk_hbox_new (FALSE, 6);
#endif
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);
  gtk_widget_show (hbox);

  window->icon_find = xfce_appfinder_model_load_pixbuf (XFCE_APPFINDER_STOCK_FIND, XFCE_APPFINDER_ICON_SIZE_48);
  
   icon_apps = gtk_image_new_from_icon_name ("applications-other", 
	GTK_ICON_SIZE_DIALOG);
	gtk_container_add (GTK_CONTAINER(window->apps_button), icon_apps);
	gtk_widget_show (icon_apps);
  

#if GTK_CHECK_VERSION (3, 0, 0)
  vbox2 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
#else
  vbox2 = gtk_vbox_new (FALSE, 0);
  align = gtk_alignment_new (0.0, 0.0, 1.0, 0.0);
  gtk_box_pack_start (GTK_BOX (vbox2), align, TRUE, TRUE, 0);
  gtk_widget_show (align);
#endif
  gtk_box_pack_start (GTK_BOX (hbox), vbox2, TRUE, TRUE, 0);
  gtk_widget_show (vbox2);

  window->entry = entry = gtk_entry_new ();

#if GTK_CHECK_VERSION (3, 0, 0)
  gtk_widget_set_halign (entry, GTK_ALIGN_FILL);
  gtk_box_pack_start (GTK_BOX (vbox2), entry, TRUE, TRUE, 0);
#else 
  gtk_container_add (GTK_CONTAINER (align), entry);
#endif

  g_signal_connect (G_OBJECT (entry), "icon-release",
      G_CALLBACK (xfce_appfinder_window_entry_icon_released), window);
  #if GTK_CHECK_VERSION (3, 0, 0)
  g_signal_connect (G_OBJECT (entry), "realize",
      G_CALLBACK (xfce_appfinder_window_set_padding), entry);
  #else
  g_signal_connect (G_OBJECT (entry), "realize",
      G_CALLBACK (xfce_appfinder_window_set_padding), align);
  #endif
  g_signal_connect_swapped (G_OBJECT (entry), "changed",
      G_CALLBACK (xfce_appfinder_window_entry_changed), window);
  g_signal_connect (G_OBJECT (entry), "activate",
      G_CALLBACK (xfce_appfinder_window_entry_activate), window);
  gtk_entry_set_icon_from_icon_name (GTK_ENTRY (entry),
                                     GTK_ENTRY_ICON_SECONDARY,
                                     XFCE_APPFINDER_STOCK_GO_DOWN);
  gtk_widget_show (entry);

  window->completion = completion = gtk_entry_completion_new ();
  gtk_entry_completion_set_model (completion, GTK_TREE_MODEL (window->model));
  gtk_entry_completion_set_match_func (completion, xfce_appfinder_window_completion_match_func, window, NULL);
  gtk_entry_completion_set_text_column (completion, XFCE_APPFINDER_MODEL_COLUMN_COMMAND);
  gtk_entry_completion_set_popup_completion (completion, TRUE);
  gtk_entry_completion_set_popup_single_match (completion, FALSE);
  gtk_entry_completion_set_inline_completion (completion, TRUE);
  
#if GTK_CHECK_VERSION (3, 0, 0)
  window->bin_collapsed = gtk_paned_new (GTK_ORIENTATION_HORIZONTAL);
#else
  window->bin_collapsed = gtk_alignment_new (0.0, 0.0, 1.0, 1.0);
#endif
  gtk_box_pack_start (GTK_BOX (vbox2), window->bin_collapsed, FALSE, TRUE, 0);
  gtk_widget_show (window->bin_collapsed);

#if GTK_CHECK_VERSION (3, 0, 0)
  window->paned = pane = gtk_paned_new (GTK_ORIENTATION_HORIZONTAL);
#else
  window->paned = pane = gtk_hpaned_new ();
#endif

#if GTK_CHECK_VERSION (3, 0, 0)
  window->taskview_container = gtk_grid_new ();
  gtk_grid_set_column_homogeneous (GTK_GRID(window->taskview_container), TRUE);
#else
  window->taskview_container = gtk_table_new (1, 8, TRUE);
#endif
  gtk_box_pack_start (GTK_BOX (vbox), window->taskview_container, TRUE, TRUE, 0);
  
 gtk_widget_add_events (GTK_WIDGET (window), GDK_KEY_PRESS_MASK);
  
  //Add window switcher  
  window->window_switcher = lightdash_window_switcher_new ();
  #if GTK_CHECK_VERSION (3, 0, 0)
  gtk_widget_set_vexpand (window->window_switcher, TRUE);
  gtk_grid_attach (GTK_GRID (window->taskview_container), 
				window->window_switcher, 
				0, 0, 7, 1);
  #else
  gtk_table_attach_defaults (GTK_TABLE (window->taskview_container), 
						window->window_switcher, 
						0, 8, 0, 1);
  #endif							
  gtk_widget_add_events (window->window_switcher, GDK_KEY_PRESS_MASK);
  gtk_widget_show (window->window_switcher);

  
  g_signal_connect_swapped (G_OBJECT (window->window_switcher), "task-button-clicked",
							G_CALLBACK (gtk_widget_hide), GTK_WIDGET (window));
  
						
	//Add pager
	window->pager = lightdash_pager_new ();
	#if GTK_CHECK_VERSION (3, 0, 0)
	gtk_grid_attach (GTK_GRID (window->taskview_container), 
						window->pager, 
						8, 0, 1, 1);
	#else
	gtk_table_attach_defaults(GTK_TABLE (window->taskview_container), 
						window->pager, 
						8, 9, 0, 1);
	#endif
	gtk_widget_show (window->pager);
  
  gtk_widget_show_all (window->taskview_container);
  
  
  gtk_box_pack_start (GTK_BOX (vbox), pane, TRUE, TRUE, 3);
  //integer = xfconf_channel_get_int (window->channel, "/last/pane-position", DEFAULT_PANED_POSITION);
  integer = DEFAULT_PANED_POSITION;
  gtk_paned_set_position (GTK_PANED (pane), integer);
  g_object_set (G_OBJECT (pane), "position-set", TRUE, NULL);
  
  

  scroll = window->scroll = gtk_scrolled_window_new (NULL, NULL);
  gtk_paned_add1 (GTK_PANED (pane), scroll);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scroll), GTK_SHADOW_ETCHED_IN);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_widget_show (window->scroll);

  sidepane = window->sidepane = gtk_tree_view_new_with_model (GTK_TREE_MODEL (window->category_model));
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (sidepane), FALSE);
  gtk_tree_view_set_enable_search (GTK_TREE_VIEW (sidepane), FALSE);
  g_signal_connect_swapped (G_OBJECT (sidepane), "start-interactive-search",
      G_CALLBACK (gtk_widget_grab_focus), entry);
  g_signal_connect (G_OBJECT (sidepane), "key-press-event",
      G_CALLBACK (xfce_appfinder_window_treeview_key_press_event), window);
  
   g_signal_connect_after (G_OBJECT (window), "key-press-event",
      G_CALLBACK (xfce_lightdash_window_key_press_event_after), window);
      
  gtk_tree_view_set_row_separator_func (GTK_TREE_VIEW (sidepane),
      xfce_appfinder_category_model_row_separator_func, NULL, NULL);
  gtk_container_add (GTK_CONTAINER (scroll), sidepane);
  //gtk_widget_show (sidepane);

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (sidepane));
  gtk_tree_selection_set_mode (selection, GTK_SELECTION_BROWSE);
  g_signal_connect (G_OBJECT (selection), "changed",
      G_CALLBACK (xfce_appfinder_window_category_changed), window);

  column = gtk_tree_view_column_new ();
  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_append_column (GTK_TREE_VIEW (sidepane), GTK_TREE_VIEW_COLUMN (column));

  renderer = gtk_cell_renderer_pixbuf_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (column), renderer, FALSE);
  gtk_tree_view_column_set_attributes (GTK_TREE_VIEW_COLUMN (column), renderer,
                                       "pixbuf", XFCE_APPFINDER_CATEGORY_MODEL_COLUMN_ICON, NULL);

  renderer = gtk_cell_renderer_text_new ();
  g_object_set (renderer, "ellipsize", PANGO_ELLIPSIZE_END, NULL);
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (column), renderer, TRUE);
  gtk_tree_view_column_set_attributes (GTK_TREE_VIEW_COLUMN (column), renderer,
                                       "text", XFCE_APPFINDER_CATEGORY_MODEL_COLUMN_NAME, NULL);

  
  
  window->viewscroll = scroll = gtk_scrolled_window_new (NULL, NULL);
  gtk_paned_add2 (GTK_PANED (pane), scroll);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scroll), GTK_SHADOW_ETCHED_IN);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

  /* set the icon or tree view */
  xfce_appfinder_window_view (window);

#if GTK_CHECK_VERSION (3, 0, 0)
  //window->bbox = hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
#else
  //window->bbox = hbox = gtk_hbox_new (FALSE, 6);
#endif
  //gtk_widget_show (hbox);
  
  window->button_launch = gtk_button_new ();
  gtk_widget_set_sensitive (window->button_launch, FALSE);

  window->icon_theme = gtk_icon_theme_get_for_screen (gtk_window_get_screen (GTK_WINDOW (window)));
  g_signal_connect_swapped (G_OBJECT (window->icon_theme), "changed",
      G_CALLBACK (xfce_appfinder_window_icon_theme_changed), window);
  g_object_ref (G_OBJECT (window->icon_theme));

  /* load categories in the model */
  xfce_appfinder_window_category_set_categories (NULL, window);
  window->categories_changed_id =
      g_signal_connect (G_OBJECT (window->model), "categories-changed",
                        G_CALLBACK (xfce_appfinder_window_category_set_categories),
                        window);

  /* monitor xfconf property changes */
  //window->property_watch_id =
    //g_signal_connect (G_OBJECT (window->channel), "property-changed",
        //G_CALLBACK (xfce_appfinder_window_property_changed), window);
        
  //lightdash_window_bookmarks_changed (window->model, window);
  
  g_signal_connect (G_OBJECT (window->model), "bookmark-added",
	G_CALLBACK (lightdash_window_bookmark_added), window);

  g_signal_connect (G_OBJECT (window->model), "bookmark-removed",
	G_CALLBACK (lightdash_window_bookmark_removed), window);
        
  gtk_widget_grab_focus (entry);

  APPFINDER_DEBUG ("constructed window");
}


static void lightdash_window_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	XfceAppfinderWindow *window = XFCE_APPFINDER_WINDOW (object);
	
	switch (property_id)
	{
		case PROP_PLUGIN:
		window->lightdash_plugin = (LightdashPlugin *) g_value_get_pointer (value);
		break;
		
		default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		break;
	}
}

static void lightdash_window_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	XfceAppfinderWindow *window = XFCE_APPFINDER_WINDOW (object);
	
	switch (property_id)
	{
		case PROP_PLUGIN:
		g_value_set_pointer (value, window->lightdash_plugin);
		break;
		
		default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		break;
	}
}

static void lightdash_window_constructed (GObject *object)
{
	XfceAppfinderWindow *window;
	
	window = XFCE_APPFINDER_WINDOW (object);
	
	#if GTK_CHECK_VERSION (3, 0, 0) 
  g_signal_connect (G_OBJECT (window), "draw",
      G_CALLBACK (xfce_lightdash_window_draw), window);
  #else   
  g_signal_connect (G_OBJECT (window), "expose-event",
      G_CALLBACK (xfce_lightdash_window_expose), window);
  #endif
}
	
	


static void
xfce_appfinder_window_finalize (GObject *object)
{
  XfceAppfinderWindow *window = XFCE_APPFINDER_WINDOW (object);

  if (window->idle_entry_changed_id != 0)
    g_source_remove (window->idle_entry_changed_id);

  g_signal_handler_disconnect (window->channel, window->property_watch_id);
  g_signal_handler_disconnect (window->model, window->categories_changed_id);

  /* release our reference on the icon theme */
  g_signal_handlers_disconnect_by_func (G_OBJECT (window->icon_theme),
      xfce_appfinder_window_icon_theme_changed, window);
  g_object_unref (G_OBJECT (window->icon_theme));

  g_object_unref (G_OBJECT (window->model));
  g_object_unref (G_OBJECT (window->category_model));
  g_object_unref (G_OBJECT (window->filter_model));
  g_object_unref (G_OBJECT (window->sort_model));
  g_object_unref (G_OBJECT (window->completion));
  g_object_unref (G_OBJECT (window->icon_find));

  if (window->actions != NULL)
    g_object_unref (G_OBJECT (window->actions));

  if (window->filter_category != NULL)
    g_object_unref (G_OBJECT (window->filter_category));
  g_free (window->filter_text);

  (*G_OBJECT_CLASS (xfce_appfinder_window_parent_class)->finalize) (object);
}

static void
appfinder_window_destroyed (GtkWidget *window)
{
  //XfconfChannel *channel;

  if (windows == NULL)
    return;

  /* take a reference on the model */
  if (model_cache == NULL)
    {
      APPFINDER_DEBUG ("main took reference on the main model");
      model_cache = xfce_appfinder_model_get ();
    }

  /* remove from internal list */
  windows = g_slist_remove (windows, window);

  /* check if we're going to the background
   * if the last window is closed */
  if (windows == NULL)
    {
      if (!service_owner)
        {
          /* leave if we're not the daemon or started
           * with --disable-server */
          gtk_main_quit ();
        }
      else
        {
          /* leave if the user disable the service in the prefereces */
          //channel = xfconf_channel_get ("xfce4-lightdash");
          //if (!xfconf_channel_get_bool (channel, "/enable-service", FALSE))
            //gtk_main_quit ();
        }
    }
}

GtkWidget *
lightdash_window_new (const gchar *startup_id,
                      gboolean     expanded, LightdashPlugin *lightdash_plugin)
{
  GtkWidget *window;

  window = g_object_new (XFCE_TYPE_APPFINDER_WINDOW,
                         "startup-id", IS_STRING (startup_id) ? startup_id : NULL,
                         "plugin", lightdash_plugin,
                         NULL);
  
  appfinder_refcount_debug_add (G_OBJECT (window), startup_id);
  xfce_appfinder_window_set_expanded (XFCE_APPFINDER_WINDOW (window), expanded);
	
  windows = g_slist_prepend (windows, window);
  g_signal_connect (G_OBJECT (window), "destroy",
                    G_CALLBACK (appfinder_window_destroyed), NULL);
                    
  return window;
}

/*
static void
xfce_appfinder_window_unmap (GtkWidget *widget)
{
  XfceAppfinderWindow *window = XFCE_APPFINDER_WINDOW (widget);
  gint                 width, height;
  gint                 position;

  position = gtk_paned_get_position (GTK_PANED (window->paned));
  gtk_window_get_size (GTK_WINDOW (window), &width, &height);
  if (!gtk_widget_get_visible (window->paned))
    height = window->last_window_height;

  (*GTK_WIDGET_CLASS (xfce_appfinder_window_parent_class)->unmap) (widget);

  xfconf_channel_set_int (window->channel, "/last/window-height", height);
  xfconf_channel_set_int (window->channel, "/last/window-width", width);
  xfconf_channel_set_int (window->channel, "/last/pane-position", position);

  return (*GTK_WIDGET_CLASS (xfce_appfinder_window_parent_class)->unmap) (widget);
} */

static void lightdash_window_on_desktop_window_opened (XfceAppfinderWindow *window)
{	
	window->root = lightdash_compositor_get_root_window (window->compositor);
	
	if (window->root)
	{
		g_signal_handlers_disconnect_by_func (window->screen,
					G_CALLBACK (lightdash_window_on_desktop_window_opened), window);
	}
}	

static void lightdash_window_realize (GtkWidget *widget)
{
	XfceAppfinderWindow *window = XFCE_APPFINDER_WINDOW (widget);
	(*GTK_WIDGET_CLASS (xfce_appfinder_window_parent_class)->realize) (widget);
	
	window->compositor = lightdash_compositor_get_default ();
	window->screen = lightdash_compositor_get_wnck_screen (window->compositor);
	g_signal_connect (window->screen, "active-window-changed",
					G_CALLBACK (lightdash_window_workspace_changed), window);
	
	if (!window->root)				
		window->root = lightdash_compositor_get_root_window (window->compositor);
		
	if (!window->root)
	{
		g_signal_connect_swapped (window->screen,
				"window-opened", G_CALLBACK (lightdash_window_on_desktop_window_opened), window);
	}
}


static gboolean
xfce_appfinder_window_key_press_event (GtkWidget   *widget,
                                       GdkEventKey *event)
{
  XfceAppfinderWindow   *window = XFCE_APPFINDER_WINDOW (widget);
  GtkWidget             *entry;
  XfceAppfinderIconSize  icon_size = XFCE_APPFINDER_ICON_SIZE_DEFAULT_ITEM;
  entry = XFCE_APPFINDER_WINDOW (widget)->entry;

  if (event->keyval == GDK_KEY_Escape)
    {
      const gchar *text = gtk_entry_get_text (GTK_ENTRY (entry));
      if ((text != NULL) && (*text != '\0'))
      	gtk_entry_set_text (GTK_ENTRY (entry), "");
      else if (gtk_widget_get_visible (window->paned))
        {
          gtk_widget_hide (window->paned);
          gtk_widget_show_all (window->taskview_container);
          gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (window->apps_button), FALSE);
        }
      else
      	gtk_widget_hide (widget);
      return TRUE;
    }

  if (gtk_widget_get_visible (window->paned))
    {
   if (event->keyval == GDK_KEY_Up || event->keyval == GDK_KEY_Down)
	{
		if (gtk_window_get_focus (GTK_WINDOW (widget)) == entry)
			{
				gtk_widget_grab_focus (window->view);
			}
	}
}

  return  (*GTK_WIDGET_CLASS (xfce_appfinder_window_parent_class)->key_press_event) (widget, event);
}

static gboolean
xfce_lightdash_window_key_press_event_after
(GtkWidget   *widget, GdkEventKey *event, XfceAppfinderWindow *window)
{

	if (event->is_modifier)
		{
			return FALSE;
		}

	if (gtk_window_get_focus (GTK_WINDOW (widget)) != window->entry)
	{
		gtk_widget_grab_focus (window->entry);
		gtk_window_propagate_key_event (GTK_WINDOW (window), event);
		return TRUE;
	}

	return FALSE;

}

static gboolean
xfce_appfinder_window_window_state_event (GtkWidget           *widget,
                                          GdkEventWindowState *event)
{
  XfceAppfinderWindow *window = XFCE_APPFINDER_WINDOW (widget);
  gint                 width;

  if ((event->new_window_state & GDK_WINDOW_STATE_FULLSCREEN) != 0)
    gtk_window_unfullscreen (GTK_WINDOW (widget));

  if ((*GTK_WIDGET_CLASS (xfce_appfinder_window_parent_class)->window_state_event) != NULL)
    return (*GTK_WIDGET_CLASS (xfce_appfinder_window_parent_class)->window_state_event) (widget, event);

  return FALSE;
}



static void
xfce_appfinder_window_set_item_width (XfceAppfinderWindow *window)
{
  gint                   width;
  XfceAppfinderIconSize  icon_size;
  GtkOrientation         item_orientation = GTK_ORIENTATION_VERTICAL;
  GList                 *renderers;
  GList                 *li;

  appfinder_return_if_fail (GTK_IS_ICON_VIEW (window->view));

  g_object_get (G_OBJECT (window->model), "icon-size", &icon_size, NULL);

  /* some hard-coded values for the cell size that seem to work fine */
  switch (icon_size)
    {
    case XFCE_APPFINDER_ICON_SIZE_SMALLEST:
      width = 16 * 3.75;
      break;

    case XFCE_APPFINDER_ICON_SIZE_SMALLER:
      width = 24 * 3;
      break;

    case XFCE_APPFINDER_ICON_SIZE_SMALL:
      width = 36 * 2.5;
      break;

    case XFCE_APPFINDER_ICON_SIZE_NORMAL:
      width = 48 * 2;
      break;

    case XFCE_APPFINDER_ICON_SIZE_LARGE:
      width = 64 * 1.5;
      break;

    case XFCE_APPFINDER_ICON_SIZE_LARGER:
      width = 96 * 1.75;
      break;

    case XFCE_APPFINDER_ICON_SIZE_LARGEST:
      width = 128 * 1.25;
      break;
    }

  if (xfconf_channel_get_bool (window->channel, "/text-beside-icons", FALSE))
    {
      item_orientation = GTK_ORIENTATION_HORIZONTAL;
      width *= 2;
    }

#if GTK_CHECK_VERSION (2, 22, 0)
  gtk_icon_view_set_item_orientation (GTK_ICON_VIEW (window->view), item_orientation);
#else
  gtk_icon_view_set_orientation (GTK_ICON_VIEW (window->view), item_orientation);
#endif
  gtk_icon_view_set_item_width (GTK_ICON_VIEW (window->view), width);

  if (item_orientation == GTK_ORIENTATION_HORIZONTAL)
    {
      /* work around the yalign = 0.0 gtk uses */
      renderers = gtk_cell_layout_get_cells (GTK_CELL_LAYOUT (window->view));
      for (li = renderers; li != NULL; li = li->next)
        g_object_set (li->data, "yalign", 0.5, NULL);
      g_list_free (renderers);
    }
}



static gboolean
xfce_appfinder_window_view_button_press_event (GtkWidget           *widget,
                                               GdkEventButton      *event,
                                               XfceAppfinderWindow *window)
{
  gint         x, y;
  GtkTreePath *path;
  gboolean     have_selection = FALSE;

  if (event->button == 3
      && event->type == GDK_BUTTON_PRESS)
    {
      if (GTK_IS_TREE_VIEW (widget))
        {
          gtk_tree_view_convert_widget_to_bin_window_coords (GTK_TREE_VIEW (widget),
                                                             event->x, event->y, &x, &y);

          if (gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (widget), x, y, &path, NULL, NULL, NULL))
            {
              gtk_tree_view_set_cursor (GTK_TREE_VIEW (widget), path, NULL, FALSE);
              gtk_tree_path_free (path);
              have_selection = TRUE;
            }
        }
      else
        {
          path = gtk_icon_view_get_path_at_pos (GTK_ICON_VIEW (widget), event->x, event->y);
          if (path != NULL)
            {
              gtk_icon_view_select_path (GTK_ICON_VIEW (widget), path);
              gtk_icon_view_set_cursor (GTK_ICON_VIEW (widget), path, NULL, FALSE);
              gtk_tree_path_free (path);
              have_selection = TRUE;
            }
        }

      if (have_selection)
        return xfce_appfinder_window_popup_menu (widget, window);
    }

  return FALSE;
}



static void
xfce_appfinder_window_view (XfceAppfinderWindow *window)
{
  GtkTreeViewColumn *column;
  GtkCellRenderer   *renderer;
  GtkTreeModel      *filter_model;
  GtkTreeSelection  *selection;
  GtkWidget         *view;
  gboolean           icon_view;

  //icon_view = xfconf_channel_get_bool (window->channel, "/icon-view", FALSE);
  icon_view = FALSE;
  if (window->view != NULL)
    {
      if (icon_view && GTK_IS_ICON_VIEW (window->view))
        return;
      gtk_widget_destroy (window->view);
    }

  window->filter_model = gtk_tree_model_filter_new (GTK_TREE_MODEL (window->model), NULL);
  gtk_tree_model_filter_set_visible_func (GTK_TREE_MODEL_FILTER (window->filter_model), xfce_appfinder_window_item_visible, window, NULL);

  window->sort_model = gtk_tree_model_sort_new_with_model (GTK_TREE_MODEL (window->filter_model));
  gtk_tree_sortable_set_default_sort_func (GTK_TREE_SORTABLE (window->sort_model), lightdash_window_sort_items, window->entry, NULL);

  if (icon_view)
    {
      #if GTK_CHECK_VERSION (3, 0, 0)
      window->view = view = gtk_icon_view_new_with_model (window->filter_model/*sort_model*/);
      gtk_icon_view_set_activate_on_single_click (GTK_ICON_VIEW (view), TRUE);
      #else
      window->view = view = exo_icon_view_new_with_model (window->filter_model);
      exo_icon_view_set_single_click (EXO_ICON_VIEW (view), TRUE);
      #endif
      gtk_icon_view_set_selection_mode (GTK_ICON_VIEW (view), GTK_SELECTION_BROWSE);
      gtk_icon_view_set_pixbuf_column (GTK_ICON_VIEW (view), XFCE_APPFINDER_MODEL_COLUMN_ICON);
      gtk_icon_view_set_text_column (GTK_ICON_VIEW (view), XFCE_APPFINDER_MODEL_COLUMN_TITLE);
      gtk_icon_view_set_tooltip_column (GTK_ICON_VIEW (view), XFCE_APPFINDER_MODEL_COLUMN_TOOLTIP);
      xfce_appfinder_window_set_item_width (window);

      g_signal_connect_swapped (G_OBJECT (view), "selection-changed",
          G_CALLBACK (xfce_appfinder_window_item_changed), window);
      g_signal_connect_swapped (G_OBJECT (view), "item-activated",
          G_CALLBACK (xfce_appfinder_window_row_activated), window);
    }
  else
    {
      #if GTK_CHECK_VERSION (3, 0, 0)
      window->view = view = gtk_tree_view_new_with_model (window->sort_model);
      gtk_tree_view_set_activate_on_single_click (GTK_TREE_VIEW (view), TRUE);
      #else
      window->view = view = GTK_WIDGET (exo_tree_view_new ());
      gtk_tree_view_set_model (GTK_TREE_VIEW (view), window->sort_model);
      exo_tree_view_set_single_click (EXO_TREE_VIEW (view), TRUE);
      #endif
      gtk_tree_view_set_hover_selection (GTK_TREE_VIEW (view), TRUE);
      gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (view), FALSE);
      gtk_tree_view_set_enable_search (GTK_TREE_VIEW (view), FALSE);
      gtk_tree_view_set_tooltip_column (GTK_TREE_VIEW (view), XFCE_APPFINDER_MODEL_COLUMN_TOOLTIP);
      g_signal_connect_swapped (G_OBJECT (view), "row-activated",
          G_CALLBACK (xfce_appfinder_window_row_activated), window);
      g_signal_connect_swapped (G_OBJECT (view), "start-interactive-search",
          G_CALLBACK (gtk_widget_grab_focus), window->entry);
      g_signal_connect (G_OBJECT (view), "key-press-event",
           G_CALLBACK (xfce_appfinder_window_treeview_key_press_event), window);
      g_signal_connect (G_OBJECT (view), "key-release-event",
           G_CALLBACK (lightdash_window_treeview_key_release_event), window);

      selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (view));
      gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
      g_signal_connect_swapped (G_OBJECT (selection), "changed",
          G_CALLBACK (xfce_appfinder_window_item_changed), window);

      column = gtk_tree_view_column_new ();
      gtk_tree_view_column_set_spacing (column, 2);
      gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
      gtk_tree_view_append_column (GTK_TREE_VIEW (view), GTK_TREE_VIEW_COLUMN (column));

      renderer = gtk_cell_renderer_pixbuf_new ();
      gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (column), renderer, FALSE);
      gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (column), renderer,
                                      "pixbuf", XFCE_APPFINDER_MODEL_COLUMN_ICON, NULL);

      renderer = gtk_cell_renderer_text_new ();
      g_object_set (renderer, "ellipsize", PANGO_ELLIPSIZE_END, NULL);
      gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (column), renderer, TRUE);
      gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (column), renderer,
                                      "markup", XFCE_APPFINDER_MODEL_COLUMN_ABSTRACT, NULL);
    }

  gtk_drag_source_set (view, GDK_BUTTON1_MASK, target_list,
                       G_N_ELEMENTS (target_list), GDK_ACTION_COPY);
  g_signal_connect (G_OBJECT (view), "drag-begin",
      G_CALLBACK (xfce_appfinder_window_drag_begin), window);
  g_signal_connect (G_OBJECT (view), "drag-data-get",
      G_CALLBACK (xfce_appfinder_window_drag_data_get), window);
  g_signal_connect (G_OBJECT (view), "popup-menu",
      G_CALLBACK (xfce_appfinder_window_popup_menu), window);
  g_signal_connect (G_OBJECT (view), "button-press-event",
      G_CALLBACK (xfce_appfinder_window_view_button_press_event), window);
  gtk_container_add (GTK_CONTAINER (window->viewscroll), view);
  gtk_widget_show (view);

  //g_object_unref (G_OBJECT (filter_model));
}



static gboolean
xfce_appfinder_window_view_get_selected (XfceAppfinderWindow  *window,
                                         GtkTreeModel        **model,
                                         GtkTreeIter          *iter)
{
  GtkTreeSelection *selection;
  gboolean          have_iter;
  GList            *items;

  appfinder_return_val_if_fail (XFCE_IS_APPFINDER_WINDOW (window), FALSE);
  appfinder_return_val_if_fail (model != NULL, FALSE);
  appfinder_return_val_if_fail (iter != NULL, FALSE);

  if (GTK_IS_TREE_VIEW (window->view))
    {
      selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (window->view));
      have_iter = gtk_tree_selection_get_selected (selection, model, iter);
    }
  else
    {
      items = gtk_icon_view_get_selected_items (GTK_ICON_VIEW (window->view));
      appfinder_assert (g_list_length (items) <= 1);
      if (items != NULL)
        {
          *model = gtk_icon_view_get_model (GTK_ICON_VIEW (window->view));
          have_iter = gtk_tree_model_get_iter (*model, iter, items->data);

          gtk_tree_path_free (items->data);
          g_list_free (items);
        }
      else
        {
          have_iter = FALSE;
        }
    }

  return have_iter;
}



static void
xfce_appfinder_window_popup_menu_toggle_bookmark (GtkWidget           *mi,
                                                  XfceAppfinderWindow *window)
{
  const gchar  *uri;
  GFile        *gfile;
  gchar        *desktop_id;
  GtkWidget    *menu = gtk_widget_get_parent (mi);
  GtkTreeModel *filter;
  GtkTreeModel *model;
  GError       *error = NULL;

  uri = g_object_get_data (G_OBJECT (menu), "uri");
  if (uri != NULL)
    {
      gfile = g_file_new_for_uri (uri);
      desktop_id = g_file_get_basename (gfile);
      g_object_unref (G_OBJECT (gfile));

      /* toggle bookmarks */
      filter = g_object_get_data (G_OBJECT (menu), "model");
      model = gtk_tree_model_filter_get_model (GTK_TREE_MODEL_FILTER (window->filter_model));
      xfce_appfinder_model_bookmark_toggle (XFCE_APPFINDER_MODEL (model), desktop_id, &error);

      g_free (desktop_id);

      if (G_UNLIKELY (error != NULL))
        {
          g_printerr ("%s: failed to save bookmarks: %s\n", G_LOG_DOMAIN, error->message);
          g_error_free (error);
        }
    }
}



static void
xfce_appfinder_window_popup_menu_execute (GtkWidget           *mi,
                                          XfceAppfinderWindow *window)
{
  xfce_appfinder_window_execute (window, FALSE);
}



static void
xfce_appfinder_window_popup_menu_edit (GtkWidget           *mi,
                                       XfceAppfinderWindow *window)
{
  GError      *error = NULL;
  gchar       *cmd;
  const gchar *uri;
  GtkWidget   *menu = gtk_widget_get_parent (mi);

  appfinder_return_if_fail (GTK_IS_MENU (menu));
  appfinder_return_if_fail (XFCE_IS_APPFINDER_WINDOW (window));

  uri = g_object_get_data (G_OBJECT (menu), "uri");
  if (G_LIKELY (uri != NULL))
    {
      cmd = g_strdup_printf ("exo-desktop-item-edit --xid=0x%x '%s'",
                             APPFINDER_WIDGET_XID (window), uri);
      if (!g_spawn_command_line_async (cmd, &error))
        {
          xfce_dialog_show_error (GTK_WINDOW (window), error,
                                  _("Failed to launch desktop item editor"));
          g_error_free (error);
        }
      g_free (cmd);
    }
}



static void
xfce_appfinder_window_popup_menu_revert (GtkWidget           *mi,
                                         XfceAppfinderWindow *window)
{
  const gchar *uri;
  const gchar *name;
  GError      *error;
  GtkWidget   *menu = gtk_widget_get_parent (mi);

  appfinder_return_if_fail (GTK_IS_MENU (menu));
  appfinder_return_if_fail (XFCE_IS_APPFINDER_WINDOW (window));

  name = g_object_get_data (G_OBJECT (menu), "name");
  if (name == NULL)
    return;

  if (xfce_dialog_confirm (GTK_WINDOW (window), GTK_STOCK_REVERT_TO_SAVED, NULL,
          _("This will permanently remove the custom desktop file from your home directory."),
          _("Are you sure you want to revert \"%s\"?"), name))
    {
      uri = g_object_get_data (G_OBJECT (menu), "uri");
      if (uri != NULL)
        {
          if (g_unlink (uri + 7) == -1)
            {
              error = g_error_new (G_FILE_ERROR, g_file_error_from_errno (errno),
                                   "%s: %s", uri + 7, g_strerror (errno));
              xfce_dialog_show_error (GTK_WINDOW (window), error,
                                      _("Failed to remove desktop file"));
              g_error_free (error);
            }
        }
    }
}



static void
xfce_appfinder_window_popup_menu_hide (GtkWidget           *mi,
                                       XfceAppfinderWindow *window)
{
  const gchar  *uri;
  const gchar  *name;
  GtkWidget    *menu = gtk_widget_get_parent (mi);
  gchar        *path;
  gchar        *message;
  gchar       **dirs;
  guint         i;
  const gchar  *relpath;
  XfceRc       *rc;

  appfinder_return_if_fail (GTK_IS_MENU (menu));
  appfinder_return_if_fail (XFCE_IS_APPFINDER_WINDOW (window));

  name = g_object_get_data (G_OBJECT (menu), "name");
  if (name == NULL)
    return;

  path = xfce_resource_save_location (XFCE_RESOURCE_DATA, "applications/", FALSE);
  /* I18N: the first %s will be replace with users' applications directory, the
   * second with Hidden=true */
  message = g_strdup_printf (_("To unhide the item you have to manually "
                               "remove the desktop file from \"%s\" or "
                               "open the file in the same directory and "
                               "remove the line \"%s\"."), path, "Hidden=true");

  if (xfce_dialog_confirm (GTK_WINDOW (window), NULL, _("_Hide"), message,
          _("Are you sure you want to hide \"%s\"?"), name))
    {
      uri = g_object_get_data (G_OBJECT (menu), "uri");
      if (uri != NULL)
        {
          /* lookup the correct relative path */
          dirs = xfce_resource_lookup_all (XFCE_RESOURCE_DATA, "applications/");
          for (i = 0; dirs[i] != NULL; i++)
            {
              if (g_str_has_prefix (uri + 7, dirs[i]))
                {
                  /* relative path to XFCE_RESOURCE_DATA */
                  relpath = uri + 7 + strlen (dirs[i]) - 13;

                  /* xfcerc can handle everything else */
                  rc = xfce_rc_config_open (XFCE_RESOURCE_DATA, relpath, FALSE);
                  xfce_rc_set_group (rc, G_KEY_FILE_DESKTOP_GROUP);
                  xfce_rc_write_bool_entry (rc, G_KEY_FILE_DESKTOP_KEY_HIDDEN, TRUE);
                  xfce_rc_close (rc);

                  break;
                }
            }
          g_strfreev (dirs);
        }
    }

  g_free (path);
  g_free (message);
}



static gboolean
xfce_appfinder_window_popup_menu (GtkWidget           *view,
                                  XfceAppfinderWindow *window)
{
  GtkWidget    *menu;
  GtkTreeModel *model;
  GtkTreeIter   iter;
  gchar        *title;
  gchar        *uri;
  GtkWidget    *mi;
  GtkWidget    *image;
  GtkWidget    *box;
  GtkWidget    *label;
  gchar        *path;
  gboolean      uri_is_local;
  gboolean      is_bookmark;

  if (xfce_appfinder_window_view_get_selected (window, &model, &iter))
    {
      gtk_tree_model_get (model, &iter,
                          XFCE_APPFINDER_MODEL_COLUMN_TITLE, &title,
                          XFCE_APPFINDER_MODEL_COLUMN_URI, &uri,
                          XFCE_APPFINDER_MODEL_COLUMN_BOOKMARK, &is_bookmark,
                          -1);

      /* custom command don't have an uri */
      if (uri == NULL)
        {
          g_free (title);
          return FALSE;
        }

      uri_is_local = g_str_has_prefix (uri, "file://");

      menu = gtk_menu_new ();
      g_object_set_data_full (G_OBJECT (menu), "uri", uri, g_free);
      g_object_set_data_full (G_OBJECT (menu), "name", title, g_free);
      g_object_set_data (G_OBJECT (menu), "model", model);
      g_signal_connect (G_OBJECT (menu), "selection-done",
          G_CALLBACK (gtk_widget_destroy), NULL);

      mi = gtk_menu_item_new_with_label (title);
      gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
      gtk_widget_set_sensitive (mi, FALSE);
      gtk_widget_show (mi);

      mi = gtk_separator_menu_item_new ();
      gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
      gtk_widget_show (mi);

      mi = gtk_image_menu_item_new_with_mnemonic (is_bookmark ? _("Remove From Bookmarks") : _("Add to Bookmarks"));
      gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
      g_signal_connect (G_OBJECT (mi), "activate",
          G_CALLBACK (xfce_appfinder_window_popup_menu_toggle_bookmark), window);
      gtk_widget_show (mi);

      if (is_bookmark)
        image = gtk_image_new_from_icon_name (XFCE_APPFINDER_STOCK_REMOVE, GTK_ICON_SIZE_MENU);
      else
        image = gtk_image_new_from_icon_name ("bookmark-new", GTK_ICON_SIZE_MENU);
      gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (mi), image);

      mi = gtk_separator_menu_item_new ();
      gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
      gtk_widget_show (mi);

      mi = gtk_menu_item_new ();
      gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
      g_signal_connect (G_OBJECT (mi), "activate",
          G_CALLBACK (xfce_appfinder_window_popup_menu_execute), window);
      
      #if GTK_CHECK_VERSION (3, 0, 0)    
      box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
      #else
      box = gtk_hbox_new (FALSE, 6);
      #endif
      label = gtk_label_new_with_mnemonic (_("La_unch"));
      image = gtk_image_new_from_icon_name (XFCE_APPFINDER_STOCK_EXECUTE, GTK_ICON_SIZE_MENU);
      gtk_container_add (GTK_CONTAINER (box), image);
      gtk_container_add (GTK_CONTAINER (box), label);
      gtk_container_add (GTK_CONTAINER (mi), box);
      gtk_widget_show_all (mi);

      mi = gtk_menu_item_new ();
      #if GTK_CHECK_VERSION (3, 0, 0)    
      box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
      #else
      box = gtk_hbox_new (FALSE, 6);
      #endif
      label = gtk_label_new_with_mnemonic (_("_Edit"));
      image = gtk_image_new_from_icon_name (XFCE_APPFINDER_STOCK_EDIT, GTK_ICON_SIZE_MENU);
      gtk_container_add (GTK_CONTAINER (box), image);
      gtk_container_add (GTK_CONTAINER (box), label);
      gtk_container_add (GTK_CONTAINER (mi), box);
      gtk_widget_show_all (mi);
      gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
      g_signal_connect (G_OBJECT (mi), "activate",
          G_CALLBACK (xfce_appfinder_window_popup_menu_edit), window);

      mi = gtk_menu_item_new ();
      #if GTK_CHECK_VERSION (3, 0, 0)    
      box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
      #else
      box = gtk_hbox_new (FALSE, 6);
      #endif
      label = gtk_label_new_with_mnemonic (_("_Revert"));
      image = gtk_image_new_from_icon_name (XFCE_APPFINDER_STOCK_REVERT_TO_SAVED, GTK_ICON_SIZE_MENU);
      gtk_container_add (GTK_CONTAINER (box), image);
      gtk_container_add (GTK_CONTAINER (box), label);
      gtk_container_add (GTK_CONTAINER (mi), box);
      gtk_widget_show_all (mi);
      gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
      g_signal_connect (G_OBJECT (mi), "activate",
          G_CALLBACK (xfce_appfinder_window_popup_menu_revert), window);
      path = xfce_resource_save_location (XFCE_RESOURCE_DATA, "applications/", FALSE);
      gtk_widget_set_sensitive (mi, uri_is_local && g_str_has_prefix (uri + 7, path));
      g_free (path);

      mi = gtk_menu_item_new_with_mnemonic (_("_Hide"));
      gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
      gtk_widget_set_sensitive (mi, uri_is_local);
      g_signal_connect (G_OBJECT (mi), "activate",
          G_CALLBACK (xfce_appfinder_window_popup_menu_hide), window);
      gtk_widget_show (mi);

      gtk_menu_popup (GTK_MENU (menu),
                      NULL, NULL, NULL, NULL, 3,
                      gtk_get_current_event_time ());

      return TRUE;
    }

  return FALSE;
}



static void
xfce_appfinder_window_update_image (XfceAppfinderWindow *window,
                                    GdkPixbuf           *pixbuf)
{
  if (pixbuf == NULL)
    pixbuf = window->icon_find;

  /* gtk doesn't check this */
  //if (gtk_image_get_pixbuf (GTK_IMAGE (window->image)) != pixbuf)
    //gtk_image_set_from_pixbuf (GTK_IMAGE (window->image), pixbuf);
}



static void
xfce_appfinder_window_set_padding (GtkWidget *entry,
                                   GtkWidget *align)
{
  gint          padding;
  GtkAllocation alloc;

  /* 48 is the icon size of XFCE_APPFINDER_ICON_SIZE_48 */
  gtk_widget_get_allocation (entry, &alloc);
  padding = (48 - alloc.height) / 2;
  #if GTK_CHECK_VERSION (3, 0, 0)
  gtk_widget_set_margin_top (align, MAX (0, padding));
  #else
  gtk_alignment_set_padding (GTK_ALIGNMENT (align), MAX (0, padding), 0, 0, 0);
  #endif
}



static gboolean
xfce_appfinder_window_completion_match_func (GtkEntryCompletion *completion,
                                             const gchar        *key,
                                             GtkTreeIter        *iter,
                                             gpointer            data)
{
  const gchar *text;

  XfceAppfinderWindow *window = XFCE_APPFINDER_WINDOW (data);

  appfinder_return_val_if_fail (GTK_IS_ENTRY_COMPLETION (completion), FALSE);
  appfinder_return_val_if_fail (XFCE_IS_APPFINDER_WINDOW (data), FALSE);
  appfinder_return_val_if_fail (GTK_TREE_MODEL (window->model)
      == gtk_entry_completion_get_model (completion), FALSE);

  /* don't use the casefolded key generated by gtk */
  text = gtk_entry_get_text (GTK_ENTRY (window->entry));

  return xfce_appfinder_model_get_visible_command (window->model, iter, text);
}



static gboolean
xfce_appfinder_window_entry_changed_idle (gpointer data)
{
  XfceAppfinderWindow *window = XFCE_APPFINDER_WINDOW (data);
  const gchar         *text;
  GdkPixbuf           *pixbuf;
  gchar               *normalized;
  GtkTreePath         *path;
  GtkTreeSelection    *selection;

  text = gtk_entry_get_text (GTK_ENTRY (window->entry));

      g_free (window->filter_text);

      if (IS_STRING (text))
        {
          normalized = g_utf8_normalize (text, -1, G_NORMALIZE_ALL);
          window->filter_text = g_utf8_casefold (normalized, -1);
          g_free (normalized);
        }
      else
        {
          window->filter_text = NULL;
        }
        
        if (gtk_entry_get_text_length (GTK_ENTRY(window->entry)) != 0)
        {
			gtk_widget_show_all (window->paned);
			gtk_widget_hide (window->taskview_container);
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (window->apps_button), TRUE);
		}

		else
		{
			gtk_widget_hide (window->paned);
			gtk_widget_show_all (window->taskview_container);
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (window->apps_button), FALSE);
			
		}

      APPFINDER_DEBUG ("refilter entry");
      gtk_tree_model_filter_refilter (GTK_TREE_MODEL_FILTER (window->filter_model));
      APPFINDER_DEBUG ("FILTER TEXT: %s\n", window->filter_text);
      if (GTK_IS_TREE_VIEW (window->view))
        {
          gtk_tree_view_scroll_to_point (GTK_TREE_VIEW (window->view), 0, 0);

          if (window->filter_text == NULL)
            {
              /* if filter is empty, clear selection */
              selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (window->view));
              gtk_tree_selection_unselect_all (selection);

              /* reset cursor */
              path = gtk_tree_path_new_first ();
              gtk_tree_view_set_cursor (GTK_TREE_VIEW (window->view), path, NULL, FALSE);
              gtk_tree_path_free (path);
            }
          else if (gtk_tree_view_get_visible_range (GTK_TREE_VIEW (window->view), &path, NULL))
            {
              /* otherwise select the first match */
              gtk_tree_view_set_cursor (GTK_TREE_VIEW (window->view), path, NULL, FALSE);
              gtk_tree_path_free (path);
            }
        }
      else
        {
          if (window->filter_text == NULL)
            {
              /* if filter is empty, clear selection */
              gtk_icon_view_unselect_all (GTK_ICON_VIEW (window->view));
            }
          else if (gtk_icon_view_get_visible_range (GTK_ICON_VIEW (window->view), &path, NULL))
            {
              /* otherwise select the first match */
              gtk_icon_view_select_path (GTK_ICON_VIEW (window->view), path);
              gtk_icon_view_set_cursor (GTK_ICON_VIEW (window->view), path, NULL, FALSE);
              gtk_tree_path_free (path);
            }
        }

  return FALSE;
}



static void
xfce_appfinder_window_entry_changed_idle_destroyed (gpointer data)
{
  XFCE_APPFINDER_WINDOW (data)->idle_entry_changed_id = 0;
}



static void
xfce_appfinder_window_entry_changed (XfceAppfinderWindow *window)
{
  if (window->idle_entry_changed_id != 0)
    g_source_remove (window->idle_entry_changed_id);

  window->idle_entry_changed_id =
      gdk_threads_add_idle_full (G_PRIORITY_DEFAULT, xfce_appfinder_window_entry_changed_idle,
                       window, xfce_appfinder_window_entry_changed_idle_destroyed);
}



static void
xfce_appfinder_window_entry_activate (GtkEditable         *entry,
                                      XfceAppfinderWindow *window)
{
  GList            *selected_items;
  GtkTreeSelection *selection;
  gboolean          has_selection = FALSE;

  if (gtk_widget_get_visible (window->paned))
    {
      if (GTK_IS_TREE_VIEW (window->view))
        {
          selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (window->view));
          has_selection = gtk_tree_selection_count_selected_rows (selection) > 0;
        }
      else
        {
          selected_items = gtk_icon_view_get_selected_items (GTK_ICON_VIEW (window->view));
          has_selection = (selected_items != NULL);
          g_list_free_full (selected_items, (GDestroyNotify) gtk_tree_path_free);
        }

      if (has_selection)
        xfce_appfinder_window_execute (window, TRUE);
    }
  else if (gtk_widget_get_sensitive (window->button_launch))
    {
      gtk_button_clicked (GTK_BUTTON (window->button_launch));
    }
}



static gboolean
xfce_appfinder_window_pointer_is_grabbed (GtkWidget *widget)
{
#if GTK_CHECK_VERSION (3, 0, 0)
  GdkDeviceManager *device_manager;
  GList            *devices, *li;
  GdkDisplay       *display;
  gboolean          is_grabbed = FALSE;

  display = gtk_widget_get_display (widget);
  device_manager = gdk_display_get_device_manager (display);
  devices = gdk_device_manager_list_devices (device_manager, GDK_DEVICE_TYPE_MASTER);

  for (li = devices; li != NULL; li = li->next)
    {
      if (gdk_device_get_source (li->data) == GDK_SOURCE_MOUSE
          && gdk_display_device_is_grabbed (display, li->data))
        {
          is_grabbed = TRUE;
          break;
        }
    }

  g_list_free (devices);

  return is_grabbed;
#else
  return gdk_pointer_is_grabbed ();
#endif
}

static void
xfce_appfinder_window_drag_begin (GtkWidget           *widget,
                                  GdkDragContext      *drag_context,
                                  XfceAppfinderWindow *window)
{
  GtkTreeModel *model;
  GtkTreeIter   iter;
  GdkPixbuf    *pixbuf;

  if (xfce_appfinder_window_view_get_selected (window, &model, &iter))
    {
      gtk_tree_model_get (model, &iter, XFCE_APPFINDER_MODEL_COLUMN_ICON_LARGE, &pixbuf, -1);
      if (G_LIKELY (pixbuf != NULL))
        {
          gtk_drag_set_icon_pixbuf (drag_context, pixbuf, 0, 0);
          g_object_unref (G_OBJECT (pixbuf));
        }
    }
  else
    {
      gtk_drag_set_icon_stock (drag_context, GTK_STOCK_DIALOG_ERROR, 0, 0);
    }
}



static void
xfce_appfinder_window_drag_data_get (GtkWidget           *widget,
                                     GdkDragContext      *drag_context,
                                     GtkSelectionData    *data,
                                     guint                info,
                                     guint                drag_time,
                                     XfceAppfinderWindow *window)
{
  GtkTreeModel *model;
  GtkTreeIter   iter;
  gchar        *uris[2];

  if (xfce_appfinder_window_view_get_selected (window, &model, &iter))
    {
      uris[1] = NULL;
      gtk_tree_model_get (model, &iter, XFCE_APPFINDER_MODEL_COLUMN_URI, &uris[0], -1);
      gtk_selection_data_set_uris (data, uris);
      g_free (uris[0]);
    }
}



static gboolean
xfce_appfinder_window_treeview_key_press_event (GtkWidget           *widget,
                                                GdkEventKey         *event,
                                                XfceAppfinderWindow *window)
{
  if (widget == window->view)
    {
      if (event->keyval == GDK_KEY_Left)
        {
          gtk_widget_grab_focus (window->sidepane);
          return TRUE;
        }
      else if ((event->keyval == GDK_KEY_Up) || (event->keyval == GDK_KEY_Down))
        {
          gtk_tree_view_set_hover_selection (GTK_TREE_VIEW (window->view), FALSE);
        }
    }
  else if (widget == window->sidepane)
    {
      if (event->keyval == GDK_KEY_Right)
        {
          gtk_widget_grab_focus (window->view);
          return TRUE;
        }
       else
		{
			gtk_widget_grab_focus (window->entry);
			gtk_window_propagate_key_event (GTK_WINDOW (window), event);
			return TRUE;
		}
    }
    
   
		

  return FALSE;
}

static gboolean
lightdash_window_treeview_key_release_event (GtkWidget           *widget,
                                                GdkEventKey         *event,
                                                XfceAppfinderWindow *window)
{
  if ((event->keyval == GDK_KEY_Up) || (event->keyval == GDK_KEY_Down))
    {
      gtk_tree_view_set_hover_selection (GTK_TREE_VIEW (window->view), TRUE);
    }
  return FALSE;
}



static void
xfce_appfinder_window_entry_icon_released (GtkEntry             *entry,
                                           GtkEntryIconPosition  icon_pos,
                                           GdkEvent             *event,
                                           XfceAppfinderWindow  *window)
{
	gtk_entry_set_text (entry, "");
}



static void
xfce_appfinder_window_category_changed (GtkTreeSelection    *selection,
                                        XfceAppfinderWindow *window)
{
  GtkTreeIter          iter;
  GtkTreeModel        *model;
  GarconMenuDirectory *category;
  gchar               *name;

  if (gtk_tree_selection_get_selected (selection, &model, &iter))
    {
      gtk_tree_model_get (model, &iter,
                          XFCE_APPFINDER_CATEGORY_MODEL_COLUMN_DIRECTORY, &category,
                          XFCE_APPFINDER_CATEGORY_MODEL_COLUMN_NAME, &name, -1);

      if (window->filter_category != category)
        {
          if (window->filter_category != NULL)
            g_object_unref (G_OBJECT (window->filter_category));

          if (category == NULL)
            window->filter_category = NULL;
          else
            window->filter_category = g_object_ref (G_OBJECT (category));

          APPFINDER_DEBUG ("refilter category");

          /* update visible items */
          if (GTK_IS_TREE_VIEW (window->view))
            model = gtk_tree_view_get_model (GTK_TREE_VIEW (window->view));
          else
            model = gtk_icon_view_get_model (GTK_ICON_VIEW (window->view));
          gtk_tree_model_filter_refilter (GTK_TREE_MODEL_FILTER (window->filter_model));

          /* store last category */
          if (xfconf_channel_get_bool (window->channel, "/remember-category", FALSE))
            xfconf_channel_set_string (window->channel, "/last/category", name);
        }

      g_free (name);
      if (category != NULL)
        g_object_unref (G_OBJECT (category));
    }
}



static void
xfce_appfinder_window_category_set_categories (XfceAppfinderModel  *signal_from_model,
                                               XfceAppfinderWindow *window)
{
  GSList           *categories;
  GtkTreePath      *path;
  gchar            *name = NULL;
  GtkTreeModel     *model;
  GtkTreeIter       iter;
  GtkTreeSelection *selection;

  appfinder_return_if_fail (GTK_IS_TREE_VIEW (window->sidepane));

  if (signal_from_model != NULL)
    {
      /* reload from the model, make sure we restore the selected category */
      selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (window->sidepane));
      if (gtk_tree_selection_get_selected (selection, &model, &iter))
        gtk_tree_model_get (model, &iter, XFCE_APPFINDER_CATEGORY_MODEL_COLUMN_NAME, &name, -1);
    }

  /* update the categories */
  categories = xfce_appfinder_model_get_categories (window->model);
  if (categories != NULL)
    xfce_appfinder_category_model_set_categories (window->category_model, categories);
  g_slist_free (categories);

  if (name == NULL && xfconf_channel_get_bool (window->channel, "/remember-category", FALSE))
    name = xfconf_channel_get_string (window->channel, "/last/category", NULL);

  path = xfce_appfinder_category_model_find_category (window->category_model, name);
  if (path != NULL)
    {
      gtk_tree_view_set_cursor (GTK_TREE_VIEW (window->sidepane), path, NULL, FALSE);
      gtk_tree_path_free (path);
    }

  g_free (name);
}



static void
xfce_lightdash_window_preferences (GtkWidget           *button,
                                   XfceAppfinderWindow *window)
{
  appfinder_return_if_fail (GTK_IS_WIDGET (button));

  /* preload the actions, to make sure there are default values */
  if (window->actions == NULL)
    window->actions = xfce_appfinder_actions_get ();

  xfce_appfinder_preferences_show (gtk_widget_get_screen (button));
}



static void
xfce_appfinder_window_property_changed (XfconfChannel       *channel,
                                        const gchar         *prop,
                                        const GValue        *value,
                                        XfceAppfinderWindow *window)
{
  appfinder_return_if_fail (XFCE_IS_APPFINDER_WINDOW (window));
  appfinder_return_if_fail (window->channel == channel);

  if (g_strcmp0 (prop, "/icon-view") == 0)
    {
      xfce_appfinder_window_view (window);
    }
  else if (g_strcmp0 (prop, "/item-icon-size") == 0)
    {
      if (GTK_IS_ICON_VIEW (window->view))
        xfce_appfinder_window_set_item_width (window);
    }
  else if (g_strcmp0 (prop, "/text-beside-icons") == 0)
    {
      if (GTK_IS_ICON_VIEW (window->view))
        xfce_appfinder_window_set_item_width (window);
    }
}



static gboolean
xfce_appfinder_window_item_visible (GtkTreeModel *model,
                                    GtkTreeIter  *iter,
                                    gpointer      data)
{
  XfceAppfinderWindow *window = XFCE_APPFINDER_WINDOW (data);

  return xfce_appfinder_model_get_visible (XFCE_APPFINDER_MODEL (model), iter,
                                           window->filter_category,
                                           window->filter_text);
}



static void
xfce_appfinder_window_item_changed (XfceAppfinderWindow *window)
{
  GtkTreeIter       iter;
  GtkTreeModel     *model;
  gboolean          can_launch;
  GdkPixbuf        *pixbuf;

  if (gtk_widget_get_visible (window->paned))
    {
      can_launch = xfce_appfinder_window_view_get_selected (window, &model, &iter);
      gtk_widget_set_sensitive (window->button_launch, can_launch);

      if (can_launch)
        {
          gtk_tree_model_get (model, &iter, XFCE_APPFINDER_MODEL_COLUMN_ICON_LARGE, &pixbuf, -1);
          if (G_LIKELY (pixbuf != NULL))
            {
              xfce_appfinder_window_update_image (window, pixbuf);
              g_object_unref (G_OBJECT (pixbuf));
            }
        }
      else
        {
          xfce_appfinder_window_update_image (window, NULL);
        }
    }
}



static void
xfce_appfinder_window_row_activated (XfceAppfinderWindow *window)
{
    xfce_appfinder_window_execute (window, TRUE);
}



static void
xfce_appfinder_window_icon_theme_changed (XfceAppfinderWindow *window)
{
  appfinder_return_if_fail (XFCE_IS_APPFINDER_WINDOW (window));

  if (window->icon_find != NULL)
    g_object_unref (G_OBJECT (window->icon_find));
  window->icon_find = xfce_appfinder_model_load_pixbuf (XFCE_APPFINDER_STOCK_FIND, XFCE_APPFINDER_ICON_SIZE_48);

  /* drop cached pixbufs */
  if (G_LIKELY (window->model != NULL))
    xfce_appfinder_model_icon_theme_changed (window->model);

  if (G_LIKELY (window->category_model != NULL))
    xfce_appfinder_category_model_icon_theme_changed (window->category_model);

  /* update state */
  xfce_appfinder_window_entry_changed (window);
  xfce_appfinder_window_item_changed (window);
}



static gboolean
xfce_appfinder_window_execute_command (const gchar          *text,
                                       GdkScreen            *screen,
                                       XfceAppfinderWindow  *window,
                                       gboolean              only_custom_cmd,
                                       gboolean             *save_cmd,
                                       GError              **error)
{
  gboolean  succeed = FALSE;
  gchar    *action_cmd = NULL;
  gchar    *expanded;

  appfinder_return_val_if_fail (error != NULL && *error == NULL, FALSE);
  appfinder_return_val_if_fail (GDK_IS_SCREEN (screen), FALSE);

  if (!IS_STRING (text))
    return TRUE;

  if (window->actions == NULL)
    window->actions = xfce_appfinder_actions_get ();

  /* try to match a custom action */
  action_cmd = xfce_appfinder_actions_execute (window->actions, text, save_cmd, error);
  if (*error != NULL)
    return FALSE;
  else if (action_cmd != NULL)
    text = action_cmd;
  else if (only_custom_cmd)
    return FALSE;

  if (IS_STRING (text))
    {
      /* expand variables */
      expanded = xfce_expand_variables (text, NULL);

      /* spawn the command */
      APPFINDER_DEBUG ("spawn \"%s\"", expanded);
      succeed = xfce_spawn_command_line_on_screen (screen, expanded, FALSE, FALSE, error);
      g_free (expanded);
    }

  g_free (action_cmd);

  return succeed;
}



static void
xfce_appfinder_window_launch_clicked (XfceAppfinderWindow *window)
{
  xfce_appfinder_window_execute (window, TRUE);
}



static void
xfce_appfinder_window_execute (XfceAppfinderWindow *window,
                               gboolean             close_on_succeed)
{
  GtkTreeModel *model, *child_model;
  GtkTreeIter   iter, child_iter;
  GError       *error = NULL;
  gboolean      result = FALSE;
  GdkScreen    *screen;
  const gchar  *text;
  gchar        *cmd = NULL;
  gboolean      regular_command = FALSE;
  gboolean      save_cmd;
  gboolean      only_custom_cmd = FALSE;

  screen = gtk_window_get_screen (GTK_WINDOW (window));
  if (gtk_widget_get_visible (window->paned))
    {
      if (!gtk_widget_get_sensitive (window->button_launch))
        {
          only_custom_cmd = TRUE;
          goto entry_execute;
        }

      if (xfce_appfinder_window_view_get_selected (window, &model, &iter))
        {
          child_model = model;

          if (GTK_IS_TREE_MODEL_SORT (model))
            {
              gtk_tree_model_sort_convert_iter_to_child_iter (GTK_TREE_MODEL_SORT (model), &child_iter, &iter);
              iter = child_iter;
              child_model = gtk_tree_model_sort_get_model (GTK_TREE_MODEL_SORT (model));
            }

          gtk_tree_model_filter_convert_iter_to_child_iter (GTK_TREE_MODEL_FILTER (child_model), &child_iter, &iter);
          result = xfce_appfinder_model_execute (window->model, &child_iter, screen, &regular_command, &error);

          if (!result && regular_command)
            {
              gtk_tree_model_get (model, &child_iter, XFCE_APPFINDER_MODEL_COLUMN_COMMAND, &cmd, -1);
              result = xfce_appfinder_window_execute_command (cmd, screen, window, FALSE, NULL, &error);
              g_free (cmd);
            }
        }
    }
  else
    {
      if (!gtk_widget_get_sensitive (window->button_launch))
        return;

      entry_execute:

      text = gtk_entry_get_text (GTK_ENTRY (window->entry));
      save_cmd = TRUE;

      if (xfce_appfinder_window_execute_command (text, screen, window, only_custom_cmd, &save_cmd, &error))
        {
          if (save_cmd)
            result = xfce_appfinder_model_save_command (window->model, text, &error);
          else
            result = TRUE;
        }
    }

  if (!only_custom_cmd)
    {
      gtk_entry_set_icon_from_icon_name (GTK_ENTRY (window->entry), GTK_ENTRY_ICON_PRIMARY,
                                     result ? NULL : XFCE_APPFINDER_STOCK_DIALOG_ERROR);
      gtk_entry_set_icon_tooltip_text (GTK_ENTRY (window->entry), GTK_ENTRY_ICON_PRIMARY,
                                       error != NULL ? error->message : NULL);
    }

  if (error != NULL)
    {
      g_printerr ("%s: failed to execute: %s\n", G_LOG_DOMAIN, error->message);
      g_error_free (error);
    }

  if (result && close_on_succeed)
    gtk_widget_hide (GTK_WIDGET (window));
}



void
xfce_appfinder_window_set_expanded (XfceAppfinderWindow *window,
                                    gboolean             expanded)
{
  GdkGeometry         hints;
  gint                width;
  GtkWidget          *parent;
  GtkEntryCompletion *completion;

  APPFINDER_DEBUG ("set expand = %s", expanded ? "true" : "false");

  /* force window geomentry */

   gtk_window_set_geometry_hints (GTK_WINDOW (window), NULL, NULL, 0);
   gtk_window_get_size (GTK_WINDOW (window), &width, NULL);
   gtk_window_resize (GTK_WINDOW (window), width, window->last_window_height);



  /* repack the button box */
  //g_object_ref (G_OBJECT (window->bbox));
  //parent = gtk_widget_get_parent (window->bbox);
  //if (parent != NULL)
    //gtk_container_remove (GTK_CONTAINER (parent), window->bbox);

    //gtk_container_add (GTK_CONTAINER (window->bin_expanded), window->bbox);


  /* show/hide pane with treeviews */
  //gtk_widget_set_visible (window->paned, TRUE);

  /* toggle icon */
  gtk_entry_set_icon_from_icon_name (GTK_ENTRY (window->entry), GTK_ENTRY_ICON_SECONDARY,
                                     expanded ? XFCE_APPFINDER_STOCK_GO_UP : XFCE_APPFINDER_STOCK_GO_DOWN);
  gtk_entry_set_icon_from_icon_name (GTK_ENTRY (window->entry), GTK_ENTRY_ICON_PRIMARY, NULL);

  /* update completion (remove completed text of restart completion) */
  completion = gtk_entry_get_completion (GTK_ENTRY (window->entry));
  if (completion != NULL)
    gtk_editable_delete_selection (GTK_EDITABLE (window->entry));
  gtk_entry_set_completion (GTK_ENTRY (window->entry), expanded ? NULL : window->completion);
  if (!expanded && gtk_entry_get_text_length (GTK_ENTRY (window->entry)) > 0)
    gtk_entry_completion_insert_prefix (window->completion);

  /* update state */
  xfce_appfinder_window_entry_changed (window);
  xfce_appfinder_window_item_changed (window);
}

static gint
lightdash_window_sort_items (GtkTreeModel *model,
                                  GtkTreeIter  *a,
                                  GtkTreeIter  *b,
                                  gpointer      data)
{
  gchar        *normalized, *casefold, *title_a, *title_b, *found;
  GtkWidget    *entry = GTK_WIDGET (data);
  gint          result = -1;

  appfinder_return_val_if_fail (GTK_IS_ENTRY (entry), 0);

  normalized = g_utf8_normalize (gtk_entry_get_text (GTK_ENTRY (entry)), -1, G_NORMALIZE_ALL);
  casefold = g_utf8_casefold (normalized, -1);
  g_free (normalized);

  gtk_tree_model_get (model, a, XFCE_APPFINDER_MODEL_COLUMN_TITLE, &title_a, -1);
  normalized = g_utf8_normalize (title_a, -1, G_NORMALIZE_ALL);
  title_a = g_utf8_casefold (normalized, -1);
  g_free (normalized);

  gtk_tree_model_get (model, b, XFCE_APPFINDER_MODEL_COLUMN_TITLE, &title_b, -1);
  normalized = g_utf8_normalize (title_b, -1, G_NORMALIZE_ALL);
  title_b = g_utf8_casefold (normalized, -1);
  g_free (normalized);

  if (strcmp (casefold, "") == 0)
    result = g_strcmp0 (title_a, title_b);
  else
    {
      found = g_strrstr (title_a, casefold);
      if (found)
        result -= (G_MAXINT - (found - title_a));

      found = g_strrstr (title_b, casefold);
      if (found)
        result += (G_MAXINT - (found - title_b));
    }

  g_free (casefold);
  g_free (title_a);
  g_free (title_b);
  return result;
}
