/* Copyright (C) 2015-2016 adlo
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
#include "lightdash-image.h"
#include "lightdash-composited-window.h"

#define DEFAULT_TABLE_COLUMNS 3
#define DEFAULT_TABLE_ROWS 2

/* 
 * This tasklist emits signals when various events happen. This allows
 * the parent application to perform actions based on these events.
 */

static void my_tasklist_on_window_opened (WnckScreen *screen, WnckWindow *window, LightdashWindowsView *tasklist);
static void my_tasklist_on_window_closed (WnckScreen *screen, WnckWindow *window, LightdashWindowsView *tasklist);
static void my_tasklist_active_workspace_changed (WnckScreen *screen, 
			WnckWorkspace *previously_active_workspace, LightdashWindowsView *tasklist);
static void my_tasklist_on_name_changed (WnckWindow *window, GtkWidget *label);
static void my_tasklist_window_workspace_changed (WnckWindow *window, LightdashWindowsView *tasklist);
static void my_tasklist_window_state_changed (WnckWindow *window, 
			WnckWindowState changed_mask, WnckWindowState new_state, LightdashWindowsView *tasklist);
static void lightdash_windows_view_skipped_window_state_changed (WnckWindow *window, 
			WnckWindowState changed_mask, WnckWindowState new_state, LightdashWindowsView *tasklist);
static void my_tasklist_screen_composited_changed (GdkScreen *screen, LightdashWindowsView *tasklist);
static void my_tasklist_free_skipped_windows (LightdashWindowsView *tasklist);
static void lightdash_windows_view_unrealize (GtkWidget *widget);
static gint my_tasklist_button_compare (gconstpointer a, gconstpointer b, gpointer data);
static gboolean lightdash_windows_view_drag_drop_handl (GtkWidget *widget, 
			GdkDragContext *context, gint x, gint y, guint time);
static void lightdash_windows_view_drag_data_received_handl (GtkWidget *widget, 
			GdkDragContext *context, gint x, gint y, GtkSelectionData *selection_data, 
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
	LightdashCompositedWindow *composited_window;
	Window xid;
	const GtkTargetEntry target;
	GdkPixbuf *pixbuf; /* Icon pixbuf */
	
	gboolean scaled;
	gboolean unparented;
	
	cairo_surface_t *image_surface;
	int surface_width;
	int surface_height;
	
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
	
#if GTK_CHECK_VERSION (3, 0, 0)
gboolean lightdash_windows_view_image_draw (GtkWidget *widget, cairo_t *cr, LightTask *task);
#else
gboolean lightdash_windows_view_image_expose (GtkWidget *widget, GdkEventExpose *event, LightTask *task);
#endif

LightTask * get_task_from_window (LightdashWindowsView *tasklist, WnckWindow *window);

skipped_window * get_skipped_window (LightdashWindowsView *tasklist, WnckWindow *window);

void lightdash_windows_view_render_preview_at_size (LightTask *task, gint width, gint height);

cairo_surface_t *
lightdash_windows_view_get_window_picture (LightTask *task);

static gint
lightdash_popup_handler (GtkWidget *widget, GdkEventButton *event, LightTask *task);

static void my_tasklist_button_clicked (GtkButton *button, LightTask *task);



//****************


static void light_task_class_init (LightTaskClass *klass)

{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	
	gobject_class->finalize = light_task_finalize;
}

static void light_task_init (LightTask *task)

{
	task->surface = NULL;
	task->damage = None;
	
	task->unparented = FALSE;
	
	task->button_resized_tag = 0;
	task->expose_tag = 0;
	
	task->image_surface = NULL;
	
	task->action_menu = NULL;
	
	task->surface_width = 1;
	task->surface_height = 1;
}

static void light_task_finalize (GObject *object)
{
	LightTask *task;
	task = LIGHT_TASK (object);
	gint error;
	
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
		gdk_error_trap_push ();
		XDamageDestroy (task->tasklist->dpy, task->damage);
		task->damage = None;
		error = gdk_error_trap_pop ();
	}
	
	g_object_unref (task->composited_window);
	
	
	if (task->image_surface)
	{
		cairo_surface_destroy (task->image_surface);
		task->image_surface = NULL;
	}
	
}

static LightTask *
light_task_new_from_window (LightdashWindowsView *tasklist, WnckWindow *window)
{
	LightTask *task;
	task = g_object_new (LIGHT_TASK_TYPE, NULL);
	
	task->tasklist = tasklist;
	
	task->window = g_object_ref (window);
	
	task->xid = wnck_window_get_xid (window);
	
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
	
	if (wnck_window_is_minimized (task->window))
		lightdash_windows_view_render_preview_at_size (task, task->previous_width, task->previous_height);
	
}


enum {
	TASK_BUTTON_CLICKED_SIGNAL,
	TASK_BUTTON_DRAG_BEGIN_SIGNAL,
	TASK_BUTTON_DRAG_END_SIGNAL,
	WORKSPACE_CHANGED_SIGNAL,
	LAST_SIGNAL
};

static guint task_button_clicked_signals[LAST_SIGNAL]={0};

static guint task_button_drag_begin_signals[LAST_SIGNAL]={0};

static guint task_button_drag_end_signals[LAST_SIGNAL]={0};

static guint workspace_changed_signals[LAST_SIGNAL]={0};

G_DEFINE_TYPE (LightdashWindowsView, lightdash_windows_view, GTK_TYPE_EVENT_BOX);

static void lightdash_windows_view_realize (GtkWidget *widget)
{
	LightdashWindowsView *tasklist;
	GtkWidget *parent_gtk_widget;
	
	
	tasklist = LIGHTDASH_WINDOWS_VIEW (widget);
	
	(*GTK_WIDGET_CLASS (lightdash_windows_view_parent_class)->realize) (widget);
	
	parent_gtk_widget = gtk_widget_get_toplevel (GTK_WIDGET (tasklist));
	
	if (gtk_widget_is_toplevel (parent_gtk_widget))
	{
		tasklist->parent_gdk_window = gtk_widget_get_window (parent_gtk_widget);
	}
	
	/* Get compositor instance*/
	tasklist->compositor = lightdash_compositor_get_default ();
	lightdash_compositor_set_excluded_window (tasklist->compositor, tasklist->parent_gdk_window);
	
	tasklist->screen = lightdash_compositor_get_wnck_screen (tasklist->compositor);
	tasklist->gdk_screen = tasklist->compositor->gdk_screen;
	tasklist->dpy = tasklist->compositor->dpy;
	
	tasklist->composited = gdk_screen_is_composited (tasklist->gdk_screen);
	
	
	
	GList *window_l;
	WnckWindow *win;
	
	tasklist->unique_id_counter = 0;
	
	lightdash_table_layout_start_from_beginning (LIGHTDASH_TABLE_LAYOUT (tasklist->table));
	
	lightdash_table_layout_resize (LIGHTDASH_TABLE_LAYOUT(tasklist->table), 
			tasklist->table_rows, tasklist->table_columns);
	
	tasklist->update_complete = FALSE;
	
	for (window_l = wnck_screen_get_windows (tasklist->screen); window_l != NULL; window_l = window_l->next)
    {
		win = WNCK_WINDOW (window_l->data);
		my_tasklist_on_window_opened (tasklist->screen, win, tasklist);
	}
	
	my_tasklist_sort (tasklist);
	lightdash_table_layout_update_rows_and_columns (LIGHTDASH_TABLE_LAYOUT (tasklist->table), 
									&tasklist->table_rows, 
									&tasklist->table_columns);
	lightdash_windows_view_reattach_widgets (tasklist);
	lightdash_table_layout_resize (LIGHTDASH_TABLE_LAYOUT (tasklist->table), 
			tasklist->table_rows, tasklist->table_columns);
	
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
	
	tasklist->unique_id_counter = 0;
	
	
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
	
	widget_class->realize = lightdash_windows_view_realize;
	widget_class->drag_motion = lightdash_windows_view_drag_motion;
	widget_class->unrealize = lightdash_windows_view_unrealize;
	
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
		
		workspace_changed_signals [WORKSPACE_CHANGED_SIGNAL] = 
		g_signal_new ("workspace-changed",
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

static void lightdash_windows_view_unrealize (GtkWidget *widget)
{
	LightdashWindowsView *tasklist = LIGHTDASH_WINDOWS_VIEW (widget);
	
	my_tasklist_free_tasks (tasklist);
	
	g_object_unref (tasklist->compositor);
	tasklist->compositor = NULL;
	
	(*GTK_WIDGET_CLASS (lightdash_windows_view_parent_class)->unrealize) (widget);
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
							G_CALLBACK (lightdash_windows_view_skipped_window_state_changed),
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
		
		rows = tasklist->table_rows;
		columns = tasklist->table_columns;
		
		lightdash_table_layout_update_rows_and_columns (LIGHTDASH_TABLE_LAYOUT (tasklist->table),
											&tasklist->table_rows, 
											&tasklist->table_columns);
		
		if (tasklist->table_columns != columns)
		{
			my_tasklist_sort (tasklist);
			lightdash_windows_view_reattach_widgets (tasklist);
			lightdash_table_layout_resize (LIGHTDASH_TABLE_LAYOUT (tasklist->table), 
					tasklist->table_rows, 
					tasklist->table_columns);
		}
		
	}
						

}


static void my_tasklist_on_window_closed 
	(WnckScreen *screen, WnckWindow *window, LightdashWindowsView *tasklist)
{
	LightTask *task;
	skipped_window *skipped;

	
	if (wnck_window_is_skip_tasklist (window) 
		&& (skipped = get_skipped_window (tasklist, window)) != NULL)
	{

		tasklist->skipped_windows = g_list_remove (tasklist->skipped_windows, 
			(gconstpointer) skipped);		
					
		g_signal_handler_disconnect (skipped->window, skipped->tag);
		g_object_unref (skipped->window);
		g_free (skipped);
		skipped = NULL;

		
		return;
	}
		
	task = get_task_from_window (tasklist, window);
	
	if(task)
	{

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
			
			lightdash_table_layout_update_rows_and_columns (LIGHTDASH_TABLE_LAYOUT (tasklist->table),
											&tasklist->table_rows, 
											&tasklist->table_columns);
			
			lightdash_windows_view_reattach_widgets (tasklist);
		
			
		lightdash_table_layout_resize (LIGHTDASH_TABLE_LAYOUT(tasklist->table), 
				tasklist->table_rows,
				tasklist->table_columns);
		gtk_widget_queue_resize (GTK_WIDGET(tasklist));
	}
		
}

static void my_tasklist_active_workspace_changed (WnckScreen *screen, 
				WnckWorkspace *previously_active_workspace, LightdashWindowsView *tasklist)
{
	GList *li;
	LightTask *task;
	
	g_signal_emit_by_name (tasklist, "workspace-changed");
	
	lightdash_table_layout_start_from_beginning (LIGHTDASH_TABLE_LAYOUT (tasklist->table));
	
	for (li = tasklist->tasks; li != NULL; li = li->next)
	{
		task = (LightTask *)li->data;
		if (gtk_widget_get_parent (task->button) == tasklist->table)
		{
			g_object_ref (task->button);
			gtk_container_remove (GTK_CONTAINER (tasklist->table), task->button);
			task->unparented = TRUE;
		}
	}
		
		/* Resize table */
		lightdash_table_layout_resize (LIGHTDASH_TABLE_LAYOUT (tasklist->table),
					tasklist->table_rows,
					tasklist->table_columns);
					
	for (li = tasklist->tasks; li != NULL; li = li->next)
	{
		task = (LightTask *)li->data;
		if (wnck_window_is_on_workspace (task->window, 
						wnck_screen_get_active_workspace (tasklist->screen)))
		{
			lightdash_table_layout_attach_next (task->button, LIGHTDASH_TABLE_LAYOUT (tasklist->table));
			
			if (task->unparented)
			{
				g_object_unref (task->button);
				task->unparented = FALSE;
			}
			
			if (task->expose_tag == 0)
			{
				#if GTK_CHECK_VERSION (3, 0, 0)
				task->expose_tag = g_signal_connect (task->image, "draw",
								G_CALLBACK (lightdash_windows_view_image_draw),
								task);
				#else
				task->expose_tag = g_signal_connect (task->image, "expose-event",
								G_CALLBACK (lightdash_windows_view_image_expose),
								task);
				#endif
			}
			
			gtk_widget_queue_draw (task->image);
		}
	}			
				my_tasklist_sort (tasklist);
				lightdash_table_layout_update_rows_and_columns (LIGHTDASH_TABLE_LAYOUT (tasklist->table),
										&tasklist->table_rows, &tasklist->table_columns);
				lightdash_table_layout_resize (LIGHTDASH_TABLE_LAYOUT (tasklist->table),
								tasklist->table_rows, tasklist->table_columns);
				lightdash_windows_view_reattach_widgets (tasklist);
		
}

static void my_tasklist_window_workspace_changed (WnckWindow *window, LightdashWindowsView *tasklist)
{
	my_tasklist_active_workspace_changed (NULL, NULL, tasklist);
}

static void my_tasklist_screen_composited_changed (GdkScreen *screen, LightdashWindowsView *tasklist)
{
	tasklist->composited = gdk_screen_is_composited (tasklist->gdk_screen);
}

static void my_tasklist_window_state_changed (WnckWindow *window, 
		WnckWindowState changed_mask, WnckWindowState new_state, LightdashWindowsView *tasklist)
{
	if (changed_mask & WNCK_WINDOW_STATE_SKIP_TASKLIST)
	{
		my_tasklist_on_window_closed (tasklist->screen, window, tasklist);
		my_tasklist_on_window_opened (tasklist->screen, window, tasklist);
	}
	
	else if (changed_mask & WNCK_WINDOW_STATE_MINIMIZED)
	{
		LightTask *task;
		task = get_task_from_window (tasklist, window);
		gtk_widget_queue_draw (task->image);
	}
}

static void lightdash_windows_view_skipped_window_state_changed (WnckWindow *window, 
		WnckWindowState changed_mask, WnckWindowState new_state, LightdashWindowsView *tasklist)
{
	skipped_window *skipped;
	
	if (changed_mask & WNCK_WINDOW_STATE_SKIP_TASKLIST)
	{
		skipped = get_skipped_window (tasklist, window);
		tasklist->skipped_windows = g_list_remove (tasklist->skipped_windows, skipped);
		g_signal_handler_disconnect (skipped->window, skipped->tag);
		g_object_unref (skipped->window);
		g_free (skipped);
		
		my_tasklist_on_window_opened (tasklist->screen, window, tasklist);
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

static void my_tasklist_button_clicked (GtkButton *button, LightTask *task)
	
{
	wnck_window_activate (task->window, gtk_get_current_event_time());
	g_signal_emit_by_name (task->tasklist, "task-button-clicked");
	
}

void lightdash_windows_view_render_preview_at_size (LightTask *task, gint width, gint height)
{
		gint src_width, src_height;
		gint dest_width, dest_height;
		gfloat factor;
		cairo_t *cr;
		gint pixbuf_width, pixbuf_height;
		
		lightdash_composited_window_get_size (task->composited_window, &src_width, &src_height);
		
		factor = (gfloat) MIN ((gfloat)width / (gfloat)src_width,
						(gfloat)height / (gfloat)src_height);
				
		dest_width = src_width*factor;
		dest_height = src_height*factor;
		
		cairo_surface_destroy (task->image_surface);
		
		if ((gint)dest_width == 0)
			dest_width = 1;
		
		if ((gint)dest_height == 0)
			dest_height = 1;
		
		if (dest_width > src_width && dest_height > src_height)
		{
			factor = 1.0;
			dest_width = src_width;
			dest_height = src_height;
		}
		
		task->surface_width = dest_width;
		task->surface_height = dest_height;
			
		task->image_surface = gdk_window_create_similar_surface (task->tasklist->parent_gdk_window,
								CAIRO_CONTENT_COLOR,
								dest_width, dest_height);
								
		cr = cairo_create (task->image_surface);

		
		if (wnck_window_is_minimized (task->window) || task->tasklist->composited == FALSE)
		{
			cairo_rectangle (cr, 0, 0, src_width*factor, src_height*factor);
			
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
		}
		
		else
		{
			cairo_scale (cr, factor, factor);

			cairo_rectangle (cr, 0, 0, src_width, src_height);
			
			cairo_set_source_surface (cr, task->composited_window->surface, 0, 0);
		
			cairo_pattern_set_filter (cairo_get_source (cr), CAIRO_FILTER_BILINEAR);
			
			cairo_fill (cr);
		}
				
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

#if GTK_CHECK_VERSION (3, 0, 0)
gboolean lightdash_windows_view_image_draw (GtkWidget *widget, cairo_t *cr, LightTask *task)
#else
gboolean lightdash_windows_view_image_expose (GtkWidget *widget, GdkEventExpose *event, LightTask *task)
#endif
{
		#if GTK_CHECK_VERSION (3, 0, 0)
		#else
		cairo_t *cr;
		#endif
		
		int width, height;
		
		gfloat aspect_ratio;
	
		#if GTK_CHECK_VERSION (3, 0, 0)
		width = gtk_widget_get_allocated_width (task->image);
		height = gtk_widget_get_allocated_height (task->image);
		#else
		width = task->image->allocation.width;
		height = task->image->allocation.height;
		#endif
		
		lightdash_windows_view_render_preview_at_size (task, width, height);
		
		#if GTK_CHECK_VERSION (3, 0, 0)
		task->previous_width = gtk_widget_get_allocated_width (task->image);
		task->previous_height = gtk_widget_get_allocated_height (task->image);
		#else
		task->previous_width = task->image->allocation.width;
		task->previous_height = task->image->allocation.height;
		#endif
		
		task->scaled = TRUE;
			
		#if GTK_CHECK_VERSION (3, 0, 0)
		cairo_set_source_surface (cr, task->image_surface, 
				width/2 - task->surface_width/2, 
				height/2 - task->surface_height/2);
				
		cairo_paint (cr);
		
		task->previous_width = gtk_widget_get_allocated_width (task->image);
		task->previous_height = gtk_widget_get_allocated_height (task->image);
	
		#else

		cr = gdk_cairo_create (widget->window);
		
		cairo_set_source_surface (cr, task->image_surface, 
				widget->allocation.x + width/2 - task->surface_width/2,
				widget->allocation.y + height/2 - task->surface_height/2);
		
		cairo_paint (cr);
		
		task->previous_width = task->image->allocation.width;
		task->previous_height = task->image->allocation.height;
		
		cairo_destroy (cr);	
		
		#endif

		return FALSE;
}
			

static void my_tasklist_drag_begin_handl
(GtkWidget *widget, GdkDragContext *context, LightTask *task)
{
	gint src_width, src_height;
	gfloat size, factor, surface_width, surface_height;
	cairo_t *cr;
	
	g_signal_emit_by_name (task->tasklist, "task-button-drag-begin");
	
	gtk_widget_hide (task->button);
	
	lightdash_composited_window_get_size (task->composited_window, &src_width, &src_height);
	
	if (task->image_surface)
	{
	
	#if GTK_CHECK_VERSION (3, 0, 0)
	cairo_surface_t *dnd_pixmap;
	#else
	GdkPixmap *dnd_pixmap;
	#endif
	surface_width = (gfloat) task->surface_width;
	surface_height = (gfloat) task->surface_height;
	
	size = (gfloat) MAX (surface_width, surface_height);
	factor = 256 / size;
	if (surface_width * factor > surface_width || surface_height * factor > surface_height)
		factor = 1.0;
	
	#if GTK_CHECK_VERSION (3, 0, 0)
	dnd_pixmap = gdk_window_create_similar_surface (task->tasklist->parent_gdk_window,
							CAIRO_CONTENT_COLOR,
							task->surface_width * factor,
							task->surface_height * factor);
	cr = cairo_create (dnd_pixmap);
	#else
	dnd_pixmap = gdk_pixmap_new (task->tasklist->parent_gdk_window,
						task->surface_width * factor,
						task->surface_height * factor,
						-1);
	cr = gdk_cairo_create (dnd_pixmap);					
	#endif
	
	cairo_scale (cr, factor, factor);
	cairo_rectangle (cr, 0, 0, src_width, src_height);	
	cairo_set_source_surface (cr, task->image_surface, 0, 0);
	cairo_pattern_set_filter (cairo_get_source (cr), CAIRO_FILTER_BILINEAR);	
	cairo_fill (cr);		
	cairo_destroy (cr);
	
	#if GTK_CHECK_VERSION (3, 0, 0)
	gtk_drag_set_icon_surface (context, dnd_pixmap);
	cairo_surface_destroy (dnd_pixmap);
	#else
	gtk_drag_set_icon_pixmap (context,
		gdk_drawable_get_colormap (GDK_DRAWABLE (dnd_pixmap)),
		dnd_pixmap, NULL, -2, -2);
		
	g_object_unref (dnd_pixmap);
	#endif
	}
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

static void lightdash_windows_view_window_event (LightdashCompositedWindow *composited_window, LightTask *task)
{
	GtkAllocation allocation;
	
	gtk_widget_get_allocation (task->image, &allocation);
	
	lightdash_windows_view_render_preview_at_size (task, allocation.width, allocation.height);
	
	gtk_widget_queue_draw (task->image);
	
}
	
static void light_task_create_widgets (LightTask *task)
{
	static const GtkTargetEntry targets [] = { {"application/x-wnck-window-id",0,0} };
	gfloat aspect_ratio;
	
	/* avoid integer overflows */
	if (G_UNLIKELY (task->tasklist->unique_id_counter >= G_MAXUINT))
		task->tasklist->unique_id_counter = 0;
			
	task->tasklist->unique_id_counter++;
	
	task->image_surface = NULL;

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
	
	
	/* Create composited window */
	task->composited_window = lightdash_composited_window_new_from_window (task->window);
	
	
	task->image = lightdash_image_new ();
	
	if (wnck_window_is_on_workspace (task->window,
			wnck_screen_get_active_workspace (task->tasklist->screen))) 
	{
		task->image_surface = gdk_window_create_similar_surface (task->tasklist->parent_gdk_window,
								CAIRO_CONTENT_COLOR,
								1, 1);
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
	
	if (wnck_window_is_on_workspace (task->window,
		wnck_screen_get_active_workspace (task->tasklist->screen))) 
	{
		#if GTK_CHECK_VERSION (3, 0, 0)
		task->expose_tag = g_signal_connect (task->image, "draw",
							G_CALLBACK (lightdash_windows_view_image_draw),
							task);
		#else
		task->expose_tag = g_signal_connect (task->image, "expose-event",
							G_CALLBACK (lightdash_windows_view_image_expose),
							task);
		#endif
	}
	
	g_signal_connect (task->composited_window, "damage-event",
					G_CALLBACK (lightdash_windows_view_window_event), task);
							
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
					G_CALLBACK (my_tasklist_button_clicked), task,
					0);
					
	g_signal_connect_object (task->button, "button-press-event", 
					G_CALLBACK (lightdash_popup_handler), G_OBJECT (task),
					0);
}
