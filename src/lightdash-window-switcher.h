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
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this library; if not, see <http://www.gnu.org/licenses/>
 */
 
#ifndef MY_TASKLIST_H
#define MY_TASKLIST_H

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xcomposite.h>
#include <X11/extensions/Xrender.h>
#include <libwnck/libwnck.h>

G_BEGIN_DECLS

#define MY_TASKLIST_TYPE (my_tasklist_get_type())
#define MY_TASKLIST(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), MY_TASKLIST_TYPE, MyTasklist))
#define MY_TASKLIST_CLASS (klass) (G_TYPE_CHECK_CLASS_CAST ((klass), MY_TASKLIST_TYPE, MyTasklistClass))
#define IS_MY_TASKLIST (obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MY_TASKLIST_TYPE))
#define IS_MY_TASKLIST_CLASS(klass), (G_TYPE_CHECK_CLASS_TYPE ((klass), MY_TASKLIST_TYPE))

typedef struct _MyTasklist LightdashWindowsView;
typedef struct _MyTasklistClass MyTasklistClass;
typedef LightdashWindowsView MyTasklist;

struct _MyTasklist
{
	GtkEventBox event_box;
	
	GtkWidget *table;
	
	WnckScreen *screen;
	GdkScreen *gdk_screen;
	Display *dpy;
	
	GdkWindow *parent_gdk_window;
	
	gboolean composited;
	
	gboolean adjusted;
	
	gboolean update_complete;
	
	GList *tasks;
	GList *skipped_windows;
	
	guint window_counter;
	guint unique_id_counter;
	
	guint total_buttons_area;
	guint table_area;
	
	guint table_columns;
	guint table_rows;
	
	guint left_attach;	
	guint right_attach;		
	guint top_attach;		
	guint bottom_attach;
	
	GHashTable *win_hash;
	
	
	
	
};



struct _MyTasklistClass
{
	GtkEventBoxClass parent_class;
};

GType my_tasklist_get_type (void);
GtkWidget* lightdash_window_switcher_new (void);




G_END_DECLS

#endif /*__MY_TASKLIST_H__*/
