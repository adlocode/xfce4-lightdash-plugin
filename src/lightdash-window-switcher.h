/* Copyright (C) 2015 adlo
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>
 */
 
#ifndef MY_TASKLIST_H
#define MY_TASKLIST_H

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include <libwnck/libwnck.h>

G_BEGIN_DECLS

#define MY_TASKLIST_TYPE (my_tasklist_get_type())
#define MY_TASKLIST(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), MY_TASKLIST_TYPE, MyTasklist))
#define MY_TASKLIST_CLASS (klass) (G_TYPE_CHECK_CLASS_CAST ((klass), MY_TASKLIST_TYPE, MyTasklistClass))
#define IS_MY_TASKLIST (obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MY_TASKLIST_TYPE))
#define IS_MY_TASKLIST_CLASS(klass), (G_TYPE_CHECK_CLASS_TYPE ((klass), MY_TASKLIST_TYPE))

typedef struct _MyTasklist MyTasklist;
typedef struct _MyTasklistClass MyTasklistClass;

struct _MyTasklist
{
	GtkEventBox event_box;
	
	GtkWidget *table;
	WnckScreen *screen;
	
	GList *tasks;
	GList *skipped_windows;
	
	gboolean my_tasklist_button_click_action_is_set;
	
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
