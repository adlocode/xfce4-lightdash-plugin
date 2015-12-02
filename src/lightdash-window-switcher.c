/* Copyright (C) 2015 adlo
 * 
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this library; if not, see <http://www.gnu.org/licenses/>
 */
 
 
#include <gtk/gtk.h>
#include <libwnck/libwnck.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xcomposite.h>
#include <X11/extensions/Xrender.h>
#include <X11/extensions/Xdamage.h>
#include <X11/extensions/Xfixes.h>
#include <gdk/gdkx.h>
#include <cairo/cairo.h>
#include <cairo/cairo-xlib.h>
#include <cairo/cairo-xlib-xrender.h>
#include <stdlib.h>
#include <math.h>
#include "lightdash-window-switcher.h"
#include "table-layout.h"

#define DEFAULT_TABLE_COLUMNS 3
#define DEFAULT_TABLE_ROWS 2

/* 
 * This tasklist emits signals when various events happen. This allows
 * the parent application to perform actions based on these events.
 */

static void my_tasklist_update_windows (LightdashWindowsView *tasklist);
static void my_tasklist_on_window_opened (WnckScreen *screen, WnckWindow *window, LightdashWindowsView *tasklist);
static void my_tasklist_on_window_closed (WnckScreen *screen, WnckWindow *window, LightdashWindowsView *tasklist);
static void my_tasklist_active_workspace_changed
	(WnckScreen *screen, WnckWorkspace *previously_active_workspace, LightdashWindowsView *tasklist);
static void my_tasklist_on_name_changed (WnckWindow *window, GtkWidget *label);
static void my_tasklist_window_workspace_changed (WnckWindow *window, LightdashWindowsView *tasklist);
static void my_tasklist_window_state_changed
	(WnckWindow *window, WnckWindowState changed_mask, WnckWindowState new_state, LightdashWindowsView *tasklist);
static void my_tasklist_screen_composited_changed (GdkScreen *screen, LightdashWindowsView *tasklist);
static void my_tasklist_button_clicked (GtkButton *button, WnckWindow *window);
static void my_tasklist_button_emit_click_signal (GtkButton *button, LightdashWindowsView *tasklist);
static void my_tasklist_free_skipped_windows (LightdashWindowsView *tasklist);
static int lightdash_windows_view_xhandler_xerror (Display *dpy, XErrorEvent *e);
static gint my_tasklist_button_compare (gconstpointer a, gconstpointer b, gpointer data);
static gboolean
lightdash_windows_view_drag_drop_handl
(GtkWidget *widget, GdkDragContext *context, gint x, gint y, guint time);
static void
lightdash_windows_view_drag_data_received_handl
(GtkWidget *widget, GdkDragContext *context, gint x, gint y, GtkSelectionData *selection_data, 
	guint target_type, guint time);
gboolean lightdash_windows_view_drag_motion (GtkWidget *widget, GdkDragContext *drag_context,
	gint x, gint y, guint time);
void lightdash_windows_view_reattach_widgets (LightdashWindowsView *tasklist);

#define LIGHT_TASK_TYPE (light_task_get_type())
#define LIGHT_TASK(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGHT_TASK_TYPE, LightTask))
#define LIGHT_TASK_CLASS (klass) (G_TYPE_CHECK_CLASS_CAST ((klass), LIGHT_TASK_TYPE, LightTaskClass))
#define IS_LIGHT_TASK (obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGHT_TASK_TYPE))
#define IS_LIGHT_TASK_CLASS(klass), (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGHT_TASK_TYPE))

GType light_task_get_type (void);

typedef struct _LightTask LightTask;
typedef struct _LightTaskClass LightTaskClass;

struct _LightTask
{
	GObject parent_instance;
	
	LightdashWindowsView *tasklist;
	
	GtkWidget *button;
	GtkWidget *image;
	GtkWidget *label;
	GtkWidget *vbox;
	GtkWidget *action_menu;
	
	WnckWindow *window;
	GdkWindow *gdk_window;
	Window xid;
	const GtkTargetEntry target;
	GdkPixbuf *pixbuf;
	
	gboolean preview_created;
	gboolean scaled;
	
	#if GTK_CHECK_VERSION (3, 0, 0)
	#else
	GdkPixmap *gdk_pixmap;
	#endif
	
	cairo_surface_t *surface;
	
	Damage damage;
	
	gint previous_height;
	gint previous_width;
	
	guint unique_id;
	
	guint name_changed_tag;
	guint icon_changed_tag;
	guint workspace_changed_tag;
	guint state_changed_tag;
	guint button_resized_tag;
	guint expose_tag;
	
	XWindowAttributes attr;
	
	
};

typedef struct _skipped_window
{
	WnckWindow *window;
	gulong tag;
}skipped_window;

struct _LightTaskClass
{
	GObjectClass parent_class;
};

G_DEFINE_TYPE (LightTask, light_task, G_TYPE_OBJECT)

//Declarations with LightTasks

static void light_task_finalize (GObject *object);

static void my_tasklist_drag_begin_handl
(GtkWidget *widget, GdkDragContext *context, LightTask *task);

static void light_task_create_widgets (LightTask *task);

static void my_tasklist_drag_data_get_handl
		(GtkWidget *widget, GdkDragContext *context, GtkSelectionData *selection_data,
        guint target_type, guint time, LightTask *task);

static GdkFilterReturn lightdash_window_event (GdkXEvent *xevent, GdkEvent *event, LightTask *task);

void lightdash_windows_view_button_check_allocate_signal (GtkWidget *widget, GdkRectangle *allocation,
LightTask *task);

void lightdash_windows_view_button_size_changed (GtkWidget *widget,
	GdkRectangle *allocation, LightTask *task);
	
gboolean lightdash_windows_view_image_expose (GtkWidget *widget, GdkEvent *event, LightTask *task);

LightTask * get_task_from_window (LightdashWindowsView *tasklist, WnckWindow *window);

skipped_window * get_skipped_window (LightdashWindowsView *tasklist, WnckWindow *window);

void lightdash_windows_view_update_preview (LightTask *task, gint width, gint height);

cairo_surface_t *
lightdash_windows_view_get_window_picture (LightTask *task);

void lightdash_windows_view_redirect_window (LightTask *task);

void lightdash_windows_view_draw_symbolic_window_rectangle (LightTask *task, gint width, gint height);

static gint
lightdash_popup_handler (GtkWidget *widget, GdkEventButton *event, LightTask *task);



//****************


static void light_task_class_init (LightTaskClass *klass)

{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	
	gobject_class->finalize = light_task_finalize;
}

static void light_task_init (LightTask *task)

{
	
	task->action_menu = NULL;
	
}

static void light_task_finalize (GObject *object)
{
	LightTask *task;
	task = LIGHT_TASK (object);
	

	if (task->button_resized_tag)
	{
		g_signal_handler_disconnect (task->button,
						task->button_resized_tag);
		task->button_resized_tag = 0;
	}
	
	if (task->action_menu)
	{
		gtk_widget_destroy (task->action_menu);
		task->action_menu = NULL;
	}

	
	if (task->button)
	{
		gtk_widget_destroy (task->button);
		task->button = NULL;
		task->image = NULL;
		task->label = NULL;
	}
	
	if (task->icon_changed_tag != 0)
	{
		g_signal_handler_disconnect (task->window,
							task->icon_changed_tag);
		task->icon_changed_tag = 0;
	}
	
	if (task->name_changed_tag)
	{
		g_signal_handler_disconnect (task->window,
							task->name_changed_tag);
		task->name_changed_tag = 0;
	}
	
	if (task->workspace_changed_tag)
	{
		g_signal_handler_disconnect (task->window,
							task->workspace_changed_tag);
		task->workspace_changed_tag = 0;
	}
	
	if (task->state_changed_tag)
	{
		g_signal_handler_disconnect (task->window,
							task->state_changed_tag);
		task->state_changed_tag = 0;
	}
	
	if (task->window)
	{
		g_object_unref (task->window);
		task->window = NULL;
	}
	
	if (task->damage != None)
	{
		XDamageDestroy (task->tasklist->dpy, task->damage);
		task->damage = None;
	}
	
	
	gdk_window_remove_filter (task->gdk_window, (GdkFilterFunc) lightdash_window_event, task);
	
	if (task->surface)
	{	
		cairo_surface_destroy (task->surface);
		task->surface = NULL;
	}
	
	if (task->gdk_pixmap)
	{
		g_object_unref (task->gdk_pixmap);
		task->gdk_pixmap = NULL;
	}
	
}

static LightTask *
light_task_new_from_window (LightdashWindowsView *tasklist, WnckWindow *window)
{
	LightTask *task;
	task = g_object_new (LIGHT_TASK_TYPE, NULL);
	
	task->tasklist = tasklist;
	
	task->damage = None;
	
	task->window = g_object_ref (window);
	
	task->button_resized_tag = 0;
	
	task->expose_tag = 0;
	
	task->xid = wnck_window_get_xid (window);
	
	task->surface = NULL;
	
	task->gdk_pixmap = NULL;
	
	task->preview_created = FALSE;
	
	light_task_create_widgets (task);
	
		
	return task;
	
}

static gint my_tasklist_button_compare (gconstpointer task_a, gconstpointer task_b, gpointer data)
{
	const LightTask *a = task_a;
	const LightTask *b = task_b;
	
	return a->unique_id - b->unique_id;
}

static void my_tasklist_sort (LightdashWindowsView *tasklist)
{
	tasklist->tasks = g_list_sort_with_data (tasklist->tasks,
				my_tasklist_button_compare, tasklist);
}
			
static void my_tasklist_window_icon_changed (WnckWindow *window, LightTask *task)
{
	
	task->pixbuf = wnck_window_get_icon (task->window);
	
	if (task->tasklist->composited)
	{
		if (wnck_window_is_minimized (task->window))
			gtk_image_set_from_pixbuf (GTK_IMAGE(task->image), task->pixbuf);
		return;
	}
	
	gtk_image_set_from_pixbuf (GTK_IMAGE(task->image), task->pixbuf);
	
}




enum {
	TASK_BUTTON_CLICKED_SIGNAL,
	TASK_BUTTON_DRAG_BEGIN_SIGNAL,
	TASK_BUTTON_DRAG_END_SIGNAL,
	LAST_SIGNAL
};

static guint task_button_clicked_signals[LAST_SIGNAL]={0};

static guint task_button_drag_begin_signals[LAST_SIGNAL]={0};

static guint task_button_drag_end_signals[LAST_SIGNAL]={0};

G_DEFINE_TYPE (LightdashWindowsView, lightdash_windows_view, GTK_TYPE_EVENT_BOX);

static void lightdash_windows_view_realize (GtkWidget *widget)
{
	LightdashWindowsView *tasklist;
	GtkWidget *parent_gtk_widget;
	int dv, dr;
	
	tasklist = LIGHTDASH_WINDOWS_VIEW (widget);
	
	(*GTK_WIDGET_CLASS (lightdash_windows_view_parent_class)->realize) (widget);
	
	parent_gtk_widget = gtk_widget_get_toplevel (GTK_WIDGET (tasklist));
	
	if (gtk_widget_is_toplevel (parent_gtk_widget))
	{
		tasklist->parent_gdk_window = gtk_widget_get_window (parent_gtk_widget);
	}
	
	tasklist->screen = wnck_screen_get_default();
	tasklist->gdk_screen = gdk_screen_get_default ();
	tasklist->dpy = gdk_x11_get_default_xdisplay ();
	
	XSetErrorHandler (lightdash_windows_view_xhandler_xerror);
	
	tasklist->composited = gdk_screen_is_composited (tasklist->gdk_screen);
	
	XDamageQueryExtension (tasklist->dpy, &dv, &dr);
	gdk_x11_register_standard_event_type (gdk_screen_get_display (tasklist->gdk_screen),
		dv, dv + XDamageNotify);
	
	wnck_screen_force_update (tasklist->screen);
	
	my_tasklist_update_windows (tasklist);
	
	g_signal_connect (tasklist->screen, "window-opened",
                G_CALLBACK (my_tasklist_on_window_opened), tasklist); 
                
    g_signal_connect (tasklist->screen, "window-closed",
                G_CALLBACK (my_tasklist_on_window_closed), tasklist); 
                
   g_signal_connect (tasklist->screen, "active-workspace-changed",
               G_CALLBACK (my_tasklist_active_workspace_changed), tasklist);
               
   g_signal_connect (tasklist->gdk_screen, "composited-changed",
               G_CALLBACK (my_tasklist_screen_composited_changed), tasklist);
}
	
static void lightdash_windows_view_init (LightdashWindowsView *tasklist)
{
	
	static const GtkTargetEntry targets [] = { {"application/x-wnck-window-id",0,0} };
	
	tasklist->tasks = NULL;
	tasklist->skipped_windows = NULL;
	tasklist->adjusted = FALSE;
	
	tasklist->parent_gdk_window = NULL;
	
	tasklist->update_complete = FALSE;
	
	tasklist->table_columns = DEFAULT_TABLE_COLUMNS;
	tasklist->table_rows = DEFAULT_TABLE_ROWS;
	
	tasklist->window_counter = 0;
	tasklist->unique_id_counter = 0;
	tasklist->total_buttons_area = 0;
	tasklist->table_area = 0;
	
	
	gtk_event_box_set_visible_window (GTK_EVENT_BOX (tasklist), FALSE);
	tasklist->table = lightdash_table_layout_new (DEFAULT_TABLE_ROWS, DEFAULT_TABLE_COLUMNS, TRUE);
	gtk_container_add (GTK_CONTAINER(tasklist), tasklist->table);
	
	gtk_drag_dest_set (GTK_WIDGET (tasklist), 
		GTK_DEST_DEFAULT_MOTION, 
		targets, G_N_ELEMENTS (targets), GDK_ACTION_MOVE);
	
	g_signal_connect (GTK_WIDGET (tasklist), "drag-drop",
		G_CALLBACK (lightdash_windows_view_drag_drop_handl), NULL);
		
	g_signal_connect (GTK_WIDGET (tasklist), "drag-data-received",
		G_CALLBACK (lightdash_windows_view_drag_data_received_handl), NULL);
	
	gtk_widget_show (tasklist->table);
}

static void lightdash_windows_view_class_init (MyTasklistClass *klass)
{	
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
	
	lightdash_windows_view_parent_class = gtk_type_class (gtk_event_box_get_type ());
	
	widget_class->realize = lightdash_windows_view_realize;
	widget_class->drag_motion = lightdash_windows_view_drag_motion;
	
	task_button_clicked_signals [TASK_BUTTON_CLICKED_SIGNAL] = 
		g_signal_new ("task-button-clicked",
		G_TYPE_FROM_CLASS(klass),
		G_SIGNAL_RUN_FIRST|G_SIGNAL_ACTION,
		0,
		NULL,
		NULL,
		g_cclosure_marshal_VOID__VOID,
		G_TYPE_NONE, 0);
		
		task_button_drag_begin_signals [TASK_BUTTON_DRAG_BEGIN_SIGNAL] = 
		g_signal_new ("task-button-drag-begin",
		G_TYPE_FROM_CLASS(klass),
		G_SIGNAL_RUN_FIRST|G_SIGNAL_ACTION,
		0,
		NULL,
		NULL,
		g_cclosure_marshal_VOID__VOID,
		G_TYPE_NONE, 0);
		
		task_button_drag_end_signals [TASK_BUTTON_DRAG_END_SIGNAL] = 
		g_signal_new ("task-button-drag-end",
		G_TYPE_FROM_CLASS(klass),
		G_SIGNAL_RUN_FIRST|G_SIGNAL_ACTION,
		0,
		NULL,
		NULL,
		g_cclosure_marshal_VOID__VOID,
		G_TYPE_NONE, 0);
	
}

GtkWidget* lightdash_window_switcher_new (void)
{
	return GTK_WIDGET(g_object_new (lightdash_windows_view_get_type (), NULL));
}

static int lightdash_windows_view_xhandler_xerror (Display *dpy, XErrorEvent *e)
{
	gchar text [64];
	
		g_print ("%s", "X11 error ");
		g_print ("%d", e->error_code);
		g_print ("%s", " - ");
		XGetErrorText (dpy, e->error_code, text, 64);
		g_print ("%s", text);
		g_print ("%s", "\n");
		
		return 0;
}

static void
my_tasklist_free_tasks (LightdashWindowsView *tasklist)
{
	GList *l;
	if (tasklist->tasks)
	{
		l = tasklist->tasks;
		
		while (l != NULL)
		{
			
			LightTask *task = LIGHT_TASK (l->data);
				
			
			l = l->next;
			
			if (task->image)
			{
				if (task->button_resized_tag)
				{
				g_signal_handler_disconnect (task->button,
							task->button_resized_tag);
				task->button_resized_tag = 0;
				}
	
			}
			
			if (task->button)
			{	
				gtk_widget_destroy (task->button);
				task->button = 0;
			}
			
			g_object_unref (task);
			
			
		}
	}

	g_list_free (tasklist->tasks);
	tasklist->tasks = NULL;
	my_tasklist_free_skipped_windows (tasklist);
	
}

static void
my_tasklist_free_skipped_windows (LightdashWindowsView *tasklist)

{	
	
	if (tasklist->skipped_windows)
	
	{
		GList *skipped_l;
		skipped_l = tasklist->skipped_windows;
	
		while (skipped_l != NULL)
		{
			skipped_window *skipped = (skipped_window *) skipped_l->data;
			g_signal_handler_disconnect (skipped->window, skipped->tag);
			g_object_unref (skipped->window);
			g_free (skipped);
			skipped_l = skipped_l->next;
		}
		
		g_list_free (tasklist->skipped_windows);
		tasklist->skipped_windows = NULL;
	

		
	}
	
}

LightTask * get_task_from_window (LightdashWindowsView *tasklist, WnckWindow *window)
{
	GList *list;
	
	for (list = tasklist->tasks; list; list = g_list_next (list))
		{
			LightTask *task = (LightTask *) list->data;
			if (task->window == window)
			{
				return task;
			}
		
	}
	return NULL;
}

skipped_window * get_skipped_window (LightdashWindowsView *tasklist, WnckWindow *window)
{
	GList *list;
	
	for (list = tasklist->skipped_windows; list; list = g_list_next (list))
		{
			skipped_window *task = (skipped_window *) list->data;
			if (task->window == window)
			{
				return task;
			}
		
	}
	return NULL;
}



static void my_tasklist_update_windows (LightdashWindowsView *tasklist)
{
	GList *window_l;
	WnckWindow *win;
	
	//Table attachment values
	tasklist->unique_id_counter = 0;
	
	lightdash_table_layout_start_from_beginning (LIGHTDASH_TABLE_LAYOUT (tasklist->table));
	
	my_tasklist_free_tasks (tasklist);
	lightdash_table_layout_resize (LIGHTDASH_TABLE_LAYOUT(tasklist->table), 
			tasklist->table_rows, tasklist->table_columns);
	
	tasklist->update_complete = FALSE;
	tasklist->window_counter = 0;
	
	for (window_l = wnck_screen_get_windows (tasklist->screen); window_l != NULL; window_l = window_l->next)
    {
		win = WNCK_WINDOW (window_l->data);
		
		if (!(wnck_window_is_skip_tasklist (win)))
		{
			LightTask *task;
			
			task = light_task_new_from_window (tasklist, win);
	
			tasklist->tasks = g_list_prepend (tasklist->tasks, task);
			
			if(wnck_window_is_on_workspace(task->window, wnck_screen_get_active_workspace(tasklist->screen)))
			{
				
				lightdash_table_layout_attach_next (task->button, LIGHTDASH_TABLE_LAYOUT (tasklist->table));
				
				tasklist->window_counter++;
				
			}
			
		}
		
		else
		{
			skipped_window *skipped = g_new0 (skipped_window, 1);
			skipped->window = g_object_ref (win);
			skipped->tag = g_signal_connect (G_OBJECT (win), "state-changed",
							G_CALLBACK (my_tasklist_window_state_changed),
							tasklist);
							
		tasklist->skipped_windows =
				g_list_prepend (tasklist->skipped_windows,
					skipped);
					
		}
		

	}
	my_tasklist_sort (tasklist);
	lightdash_windows_view_update_rows_and_columns (tasklist);
	lightdash_windows_view_reattach_widgets (tasklist);
	lightdash_table_layout_resize (LIGHTDASH_TABLE_LAYOUT (tasklist->table), 
			tasklist->table_rows, tasklist->table_columns);
	tasklist->update_complete = TRUE;
	
	
}

static void my_tasklist_on_name_changed (WnckWindow *window, GtkWidget *label) 
{
	const gchar *name;
	name = wnck_window_get_name (window);
	gtk_label_set_text (GTK_LABEL(label), name);
}

static void my_tasklist_on_window_opened 
	(WnckScreen *screen, WnckWindow *window, LightdashWindowsView *tasklist)
{
	LightTask *task;
	gint rows, columns;
	
	if (wnck_window_is_skip_tasklist (window))
	{
		skipped_window *skipped = g_new0 (skipped_window, 1);
			skipped->window = g_object_ref (window);
			skipped->tag = g_signal_connect (G_OBJECT (window), "state-changed",
							G_CALLBACK (my_tasklist_window_state_changed),
							tasklist);
							
		tasklist->skipped_windows =
				g_list_prepend (tasklist->skipped_windows,
					skipped);
		return;
	}
	
	task = light_task_new_from_window (tasklist, window);
	
	tasklist->tasks = g_list_prepend (tasklist->tasks, task);
	
	if(wnck_window_is_on_workspace(task->window, wnck_screen_get_active_workspace(tasklist->screen)))
	{
		lightdash_table_layout_attach_next (task->button, LIGHTDASH_TABLE_LAYOUT (tasklist->table));
		
		tasklist->window_counter++;
		
		rows = tasklist->table_rows;
		columns = tasklist->table_columns;
		
		lightdash_windows_view_update_rows_and_columns (tasklist);
		
		if (tasklist->table_columns != columns)
		{
			my_tasklist_sort (tasklist);
			lightdash_windows_view_reattach_widgets (tasklist);
			lightdash_table_layout_resize (LIGHTDASH_TABLE_LAYOUT (tasklist->table), 
					tasklist->table_rows, 
					tasklist->table_columns);
		}
		
		//g_print ("%d", tasklist->window_counter);
	}
						

}


static void my_tasklist_on_window_closed 
	(WnckScreen *screen, WnckWindow *window, LightdashWindowsView *tasklist)
{
	LightTask *task;
	skipped_window *skipped;

	
	if (wnck_window_is_skip_tasklist (window))
	{
		
		skipped = get_skipped_window (tasklist, window);
		
		if (skipped)
		{
			tasklist->skipped_windows = g_list_remove (tasklist->skipped_windows, 
				(gconstpointer) skipped);		
					
			//g_print ("%s", wnck_window_get_name (skipped->window));
			g_signal_handler_disconnect (skipped->window, skipped->tag);
			g_object_unref (skipped->window);
			g_free (skipped);
			skipped = NULL;
			

		
			//g_print ("%s", "free skipped window \n");
			return;
		}
		
		return;
	}
		
	task = get_task_from_window (tasklist, window);
	
	if(task)
	{
			if(wnck_window_is_on_workspace(task->window, wnck_screen_get_active_workspace(tasklist->screen)))
			tasklist->window_counter--;

			if (task->button_resized_tag)
			{
				g_signal_handler_disconnect (task->button,
							task->button_resized_tag);
				task->button_resized_tag = 0;
			}
			
			if (task->button)
			{	
				gtk_widget_destroy (task->button);
				task->button = NULL;
			}
			
			tasklist->tasks = g_list_remove (tasklist->tasks, (gconstpointer) task);
			g_object_unref (task);
			task = NULL;
		
			my_tasklist_sort (tasklist);
			
			lightdash_windows_view_update_rows_and_columns (tasklist);
			
			lightdash_windows_view_reattach_widgets (tasklist);
		
			
		lightdash_table_layout_resize (LIGHTDASH_TABLE_LAYOUT(tasklist->table), 
				tasklist->table_rows,
				tasklist->table_columns);
		gtk_widget_queue_resize (GTK_WIDGET(tasklist));
	}
	
		
	//my_tasklist_update_windows (tasklist);	
}

static void my_tasklist_active_workspace_changed
	(WnckScreen *screen, WnckWorkspace *previously_active_workspace, LightdashWindowsView *tasklist)
{
	my_tasklist_update_windows (tasklist);
}

static void my_tasklist_window_workspace_changed (WnckWindow *window, LightdashWindowsView *tasklist)
{
	my_tasklist_update_windows (tasklist);
}

static void my_tasklist_screen_composited_changed (GdkScreen *screen, LightdashWindowsView *tasklist)
{
	tasklist->composited = gdk_screen_is_composited (tasklist->gdk_screen);
}

static void my_tasklist_window_state_changed
	(WnckWindow *window, WnckWindowState changed_mask, WnckWindowState new_state, LightdashWindowsView *tasklist)
{
	if (changed_mask & WNCK_WINDOW_STATE_SKIP_TASKLIST)
	{
		my_tasklist_update_windows (tasklist);
	}
	
	else if (changed_mask & WNCK_WINDOW_STATE_MINIMIZED)
	{
		my_tasklist_update_windows (tasklist);
	}
}

static gboolean
lightdash_popup_handler (GtkWidget *widget, GdkEventButton *event, LightTask *task)
{
	if (event->button == 3)
	{
		if (!task->action_menu)
		{
			task->action_menu = wnck_action_menu_new (task->window);
			gtk_menu_attach_to_widget (GTK_MENU (task->action_menu), task->button, NULL);
			
		}
		
		GdkEventButton *event_button = (GdkEventButton *) event;
		gtk_menu_popup (GTK_MENU (task->action_menu), NULL, NULL, 
					NULL,
					task->button,
					event_button->button, event_button->time);
					
		return TRUE;
	}
	
	else
	{
		return FALSE;
		
	}			
}

static void my_tasklist_button_clicked (GtkButton *button, WnckWindow *window)
	
{
	wnck_window_activate (window, gtk_get_current_event_time());
	
}

static void my_tasklist_button_emit_click_signal (GtkButton *button, LightdashWindowsView *tasklist)
{
	g_signal_emit_by_name (tasklist, "task-button-clicked");
	
}

void lightdash_windows_view_update_preview (LightTask *task, gint width, gint height)
{
		gint dest_width, dest_height;
		gfloat factor;
		cairo_t *cr;
		
		factor = (gfloat) MIN ((gfloat)width / (gfloat)task->attr.width,
						(gfloat)height / (gfloat)task->attr.height);
				
		dest_width = task->attr.width*factor;
		dest_height = task->attr.height*factor;
		
		g_object_unref (task->gdk_pixmap);
		
		if ((gint)dest_width == 0)
			dest_width = 1;
		
		if ((gint)dest_height == 0)
			dest_height = 1;
		
		task->gdk_pixmap = gdk_pixmap_new (task->tasklist->parent_gdk_window, 
			dest_width, 
			dest_height, 
			-1);
			
		cr = gdk_cairo_create (task->gdk_pixmap);
		
		cairo_scale (cr, factor, factor);

		cairo_rectangle (cr, 0, 0, task->attr.width, task->attr.height);
			
		cairo_set_source_surface (cr, task->surface, 0, 0);
		
		cairo_pattern_set_filter (cairo_get_source (cr), CAIRO_FILTER_BILINEAR);
			
		cairo_fill (cr);
		
		gtk_image_set_from_pixmap (GTK_IMAGE (task->image), task->gdk_pixmap, NULL);
		
		cairo_destroy (cr);
				
}

void lightdash_windows_view_reattach_widgets (LightdashWindowsView *tasklist)
{
		GList *li;
		LightTask *task;
		
		lightdash_table_layout_start_from_beginning (LIGHTDASH_TABLE_LAYOUT (tasklist->table));
		
		/* Remove widgets */
		for (li = tasklist->tasks; li != NULL; li = li->next)
		{
			task = (LightTask *)li->data;

			if (wnck_window_is_on_workspace (task->window, wnck_screen_get_active_workspace (tasklist->screen)))
			{
				g_object_ref (task->button);
				gtk_container_remove (GTK_CONTAINER (tasklist->table), task->button);
			}	
		}
		
		/* Resize table */
		lightdash_table_layout_resize (LIGHTDASH_TABLE_LAYOUT (tasklist->table),
					tasklist->table_rows,
					tasklist->table_columns);
			
		/* Reattach widgets */	
		for (li = tasklist->tasks; li != NULL; li = li->next)
		{
			task = (LightTask *)li->data;

			if (wnck_window_is_on_workspace (task->window, wnck_screen_get_active_workspace (tasklist->screen)))
			{		
				lightdash_table_layout_attach_next (task->button, LIGHTDASH_TABLE_LAYOUT (tasklist->table));
				g_object_unref (task->button);
			}
		}
					
	
			
}
	
void lightdash_windows_view_button_size_changed (GtkWidget *widget,
	GdkRectangle *allocation, LightTask *task)
{
	
	gfloat total_buttons_area, table_area;
	gfloat aspect_ratio;
	gint pixmap_width;
	
	if (task->scaled)
	{
		task->scaled = FALSE;
		return;
	}
	


		if (task->image->allocation.height == task->previous_height
			&& task->image->allocation.width == task->previous_width)
		return;
		
		if (wnck_window_is_minimized (task->window))
		{
			lightdash_windows_view_draw_symbolic_window_rectangle (task, task->image->allocation.width,
				task->image->allocation.height);
			task->scaled = TRUE;
			return;
		}
		
		
		lightdash_windows_view_update_preview (task, task->image->allocation.width,
							task->image->allocation.height);
		
		gtk_widget_queue_resize (GTK_WIDGET (task->tasklist));
		task->scaled = TRUE;
			
}

gboolean lightdash_windows_view_image_expose (GtkWidget *widget, GdkEvent *event, LightTask *task)
{
		gint pixmap_width;
	
		lightdash_windows_view_update_preview (task, task->image->allocation.width,
							task->image->allocation.height);
		

		gdk_pixmap_get_size (task->gdk_pixmap, &pixmap_width, NULL);
		
		gtk_image_set_from_pixmap (GTK_IMAGE (task->image), task->gdk_pixmap, NULL);
		
		if (task->expose_tag)
		{
			g_signal_handler_disconnect (task->image, task->expose_tag);
			task->expose_tag = 0;
		}
		
		task->previous_height = task->image->allocation.height;
		task->previous_width = task->image->allocation.width;
		
		task->preview_created = TRUE;
		
		XDamageSubtract (task->tasklist->dpy, task->damage, None, None);
		
		return FALSE;
}
			

static void my_tasklist_drag_begin_handl
(GtkWidget *widget, GdkDragContext *context, LightTask *task)
{
	g_signal_emit_by_name (task->tasklist, "task-button-drag-begin");
	
	gtk_widget_hide (task->button);
	
	gtk_drag_set_icon_pixmap (context,
		gdk_drawable_get_colormap (GDK_DRAWABLE (task->gdk_pixmap)),
		task->gdk_pixmap, NULL, -2, -2);
}

static void my_tasklist_drag_end_handl
(GtkWidget *widget, GdkDragContext *context, LightTask *task)
{
	g_signal_emit_by_name (task->tasklist, "task-button-drag-end");
	gtk_widget_show (task->button);
}

static void
my_tasklist_drag_data_get_handl
(GtkWidget *widget, GdkDragContext *context, GtkSelectionData *selection_data,
        guint target_type, guint time, LightTask *task)
{
	gulong xid;
	
	g_assert (selection_data != NULL);
	
	
	
	xid = wnck_window_get_xid (task->window);
	 
	
	gtk_selection_data_set
                (
                        selection_data,         /* Allocated GdkSelectionData object */
                        gtk_selection_data_get_target(selection_data), /* target type */
                        8,                 /* number of bits per 'unit' */
                        (guchar*)&xid,/* pointer to data to be sent */
                        sizeof (gulong)   /* length of data in units */
                );
                
               
     
        
}

static gboolean
lightdash_windows_view_drag_drop_handl
(GtkWidget *widget, GdkDragContext *context, gint x, gint y, guint time)
{
	GdkAtom target;
	
	target = gtk_drag_dest_find_target (widget, context, NULL);
	
	if (target !=GDK_NONE)
	gtk_drag_get_data (widget, context, target, time);
	else
	gtk_drag_finish (context, FALSE, FALSE, time);
	
	return TRUE;
}

static void
lightdash_windows_view_drag_data_received_handl
(GtkWidget *widget, GdkDragContext *context, gint x, gint y, GtkSelectionData *selection_data, 
	guint target_type, guint time)
{	
	LightdashWindowsView *window_switcher;
	gulong xid;
	GList *tmp;
	WnckWindow *win;
	
	window_switcher = LIGHTDASH_WINDOWS_VIEW (widget);
	
	if ((gtk_selection_data_get_length (selection_data) != sizeof (gulong)) ||
		(gtk_selection_data_get_format (selection_data) != 8))
	{
		gtk_drag_finish (context, FALSE, FALSE, time);
		return;
	}
	
	xid = *((gulong *) gtk_selection_data_get_data (selection_data));
	
	for (tmp = wnck_screen_get_windows_stacked (window_switcher->screen); tmp != NULL; tmp = tmp->next)
	{
		if (wnck_window_get_xid (tmp->data) == xid)
		{
			win = tmp->data;
			wnck_window_move_to_workspace (win, wnck_screen_get_active_workspace (window_switcher->screen));
			gtk_drag_finish (context, TRUE, FALSE, time);
			return;
		}
	}
	gtk_drag_finish (context, FALSE, FALSE, time);
}

gboolean lightdash_windows_view_drag_motion (GtkWidget *widget, GdkDragContext *drag_context,
	gint x, gint y, guint time)
{
	GdkAtom target;
	LightdashWindowsView *windows_view;
	GtkWidget *source_widget, *parent_widget;
	
	windows_view = LIGHTDASH_WINDOWS_VIEW (widget);
	
	source_widget = gtk_drag_get_source_widget (drag_context);
	parent_widget = gtk_widget_get_parent (source_widget);
	
	target = gtk_drag_dest_find_target (widget, drag_context, NULL);
	if (target == GDK_NONE || parent_widget == windows_view->table)
		gdk_drag_status (drag_context, 0, time);

		
	return TRUE;
}

void lightdash_windows_view_draw_symbolic_window_rectangle (LightTask *task, gint width, gint height)
{
	
	gint dest_width, dest_height;
	gint pixbuf_width, pixbuf_height;
		gfloat factor;
		cairo_t *cr;
		
		factor = (gfloat) MIN ((gfloat)width / (gfloat)task->attr.width,
						(gfloat)height / (gfloat)task->attr.height);
				
		dest_width = task->attr.width*factor;
		dest_height = task->attr.height*factor;
		
		g_object_unref (task->gdk_pixmap);
		
		if ((gint)dest_width == 0)
			dest_width = 1;
		
		if ((gint)dest_height == 0)
			dest_height = 1;
		
		task->gdk_pixmap = gdk_pixmap_new (task->tasklist->parent_gdk_window, 
			dest_width, 
			dest_height, 
			-1);
			
		cr = gdk_cairo_create (task->gdk_pixmap);

		cairo_rectangle (cr, 0, 0, task->attr.width*factor, task->attr.height*factor);
			
		cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
			
		cairo_fill (cr);
		cairo_save (cr);
		
		gdk_cairo_set_source_pixbuf (cr, task->pixbuf,
			dest_width/2 - gdk_pixbuf_get_width (task->pixbuf)/2,
			dest_height/2 - gdk_pixbuf_get_height (task->pixbuf)/2);
		
		pixbuf_width = gdk_pixbuf_get_width (task->pixbuf);
		pixbuf_height = gdk_pixbuf_get_height (task->pixbuf);
		
		cairo_rectangle (cr, 0, 0, pixbuf_width, pixbuf_height);
		cairo_paint (cr);
		cairo_restore (cr);
		
		gtk_image_set_from_pixmap (GTK_IMAGE (task->image), task->gdk_pixmap, NULL);
		
		cairo_destroy (cr);
	
}

void lightdash_windows_view_redirect_window (LightTask *task)
{	
	XCompositeRedirectWindow (task->tasklist->dpy, task->xid,
		CompositeRedirectAutomatic);
}

cairo_surface_t *
lightdash_windows_view_get_window_picture (LightTask *task)
{
	cairo_surface_t *surface;
	XRenderPictFormat *format;
		
	format = None;
			
	format = XRenderFindVisualFormat (task->tasklist->dpy, task->attr.visual);	

	surface = cairo_xlib_surface_create_with_xrender_format (task->tasklist->dpy,
			task->xid,
			task->attr.screen,
			format,
			task->attr.width,
			task->attr.height);
			
	return surface;
}

static GdkFilterReturn lightdash_window_event (GdkXEvent *xevent, GdkEvent *event, LightTask *task)
{
	int dv, dr;
	XEvent *ev;
	XDamageNotifyEvent *e;
	XConfigureEvent *ce;
	XDamageQueryExtension (task->tasklist->dpy, &dv, &dr);
	
	ev = (XEvent*)xevent;
	e = (XDamageNotifyEvent *)ev;
	
	if (ev->type == dv + XDamageNotify && task->preview_created)
	{
		gint pixmap_width;
	
	XDamageSubtract (task->tasklist->dpy, e->damage, None, None);
	
	lightdash_windows_view_update_preview (task, task->image->allocation.width,
										task->image->allocation.height);
	
	gdk_pixmap_get_size (task->gdk_pixmap, &pixmap_width, NULL);
	
	}
	else if (ev->type == ConfigureNotify && task->preview_created)
	{
		ce = &ev->xconfigure;
		
		if (ce->height == task->attr.height && ce->width == task->attr.width)
			return GDK_FILTER_CONTINUE;
			
		task->attr.width = ce->width;
		task->attr.height = ce->height;
		cairo_surface_destroy (task->surface);
		task->surface = lightdash_windows_view_get_window_picture (task);
		lightdash_windows_view_update_preview (task, task->image->allocation.width,
							task->image->allocation.height);

	}
	
	return GDK_FILTER_CONTINUE;
}
static void light_task_create_widgets (LightTask *task)
{
	static const GtkTargetEntry targets [] = { {"application/x-wnck-window-id",0,0} };
	gfloat aspect_ratio;
	task->tasklist->unique_id_counter++;
	
	task->gdk_pixmap = NULL;
	
	task->unique_id = task->tasklist->unique_id_counter;
	
	//g_print ("%s", "id:");
	//g_print ("%d", task->unique_id);
	//g_print ("%s", "\n");
	
	task->label = gtk_label_new (wnck_window_get_name (task->window));
	
	#if GTK_CHECK_VERSION (3, 0, 0)
	task->vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	#else
	task->vbox = gtk_vbox_new (FALSE, 0);
	#endif
		
	task->pixbuf = wnck_window_get_icon (task->window);
	
	task->previous_height = 0;
	task->previous_width = 0;
	task->scaled = FALSE;
	
	XGetWindowAttributes (task->tasklist->dpy, task->xid, &task->attr);
	
	task->gdk_window = gdk_x11_window_foreign_new_for_display 
		(gdk_screen_get_display (task->tasklist->gdk_screen), task->xid);
		
	if (wnck_window_is_on_workspace (task->window,
				wnck_screen_get_active_workspace (task->tasklist->screen))) 
	
	{
	#if GTK_CHECK_VERSION (3, 0, 0)
	#else
	task->gdk_pixmap = gdk_pixmap_new (task->tasklist->parent_gdk_window, 1, 1, -1);
	#endif	
			
	if (task->tasklist->composited 
		&& !wnck_window_is_minimized (task->window)
		&& task->attr.height != 0)
		{
			
			lightdash_windows_view_redirect_window (task);
			
			task->surface = lightdash_windows_view_get_window_picture (task);

			task->image = gtk_image_new ();
			
			/* Ignore damage events on its own window */
			if (task->tasklist->parent_gdk_window && task->gdk_window != task->tasklist->parent_gdk_window)
			{
				task->damage = XDamageCreate (task->tasklist->dpy, 
									task->xid, 
									XDamageReportDeltaRectangles);
				XDamageSubtract (task->tasklist->dpy, task->damage, None, None);
				gdk_window_add_filter (task->gdk_window, (GdkFilterFunc) lightdash_window_event, task);
			}
			
	

			
		}
		
		else
		{
			task->image = gtk_image_new_from_pixbuf (task->pixbuf);
			
		}
	}
		
		else
		{
			task->image = gtk_image_new_from_pixbuf (task->pixbuf);
			
		}
	
	task->button = gtk_button_new();
	gtk_button_set_relief (GTK_BUTTON (task->button), GTK_RELIEF_NONE);
	gtk_label_set_ellipsize(GTK_LABEL(task->label),PANGO_ELLIPSIZE_END);
	gtk_container_add (GTK_CONTAINER(task->button),task->vbox);
	gtk_box_pack_start (GTK_BOX (task->vbox), task->image, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (task->vbox), task->label, FALSE, TRUE, 0);
	
	task->icon_changed_tag = g_signal_connect (task->window, "icon-changed",
					G_CALLBACK (my_tasklist_window_icon_changed), task);
					
	task->name_changed_tag = g_signal_connect (task->window, "name-changed",
					G_CALLBACK (my_tasklist_on_name_changed), task->label);
					
	task->workspace_changed_tag = g_signal_connect (task->window, "workspace-changed",
					G_CALLBACK (my_tasklist_window_workspace_changed), task->tasklist);
					
	task->state_changed_tag = g_signal_connect (task->window, "state-changed",
					G_CALLBACK (my_tasklist_window_state_changed), task->tasklist);				
					
					
	if (!wnck_window_is_minimized (task->window))
	{
		task->expose_tag = g_signal_connect (task->image, "expose-event",
							G_CALLBACK (lightdash_windows_view_image_expose),
							task);

							

	}
	
		task->button_resized_tag = g_signal_connect (task->image, "size-allocate",
						G_CALLBACK (lightdash_windows_view_button_size_changed),
						task);
							
	gtk_drag_source_set (task->button,GDK_BUTTON1_MASK,targets,1,GDK_ACTION_MOVE);
					
	g_signal_connect_object (task->button, "drag-data-get",
					G_CALLBACK (my_tasklist_drag_data_get_handl), G_OBJECT(task),
					0);
					
	g_signal_connect_object (task->button, "drag-begin",
					G_CALLBACK (my_tasklist_drag_begin_handl), G_OBJECT (task),
					0);
	
	g_signal_connect_object (task->button, "drag-end",
					G_CALLBACK (my_tasklist_drag_end_handl), G_OBJECT (task),
					0);				
					
					
	g_signal_connect_object (task->button, "clicked", 
					G_CALLBACK (my_tasklist_button_clicked), task->window,
					0);
	
	g_signal_connect_object (GTK_BUTTON(task->button), "clicked", 
					G_CALLBACK (my_tasklist_button_emit_click_signal), task->tasklist,
					0);
					
	g_signal_connect_object (task->button, "button-press-event", 
					G_CALLBACK (lightdash_popup_handler), G_OBJECT (task),
					0);
}
