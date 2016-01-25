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
 
#ifndef LIGHTDASH_TABLE_LAYOUT_H
#define LIGHTDASH_TABLE_LAYOUT_H

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include "lightdash-window-switcher.h"

G_BEGIN_DECLS

#define LIGHTDASH_TABLE_LAYOUT_TYPE (lightdash_table_layout_get_type())
#define LIGHTDASH_TABLE_LAYOUT(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGHTDASH_TABLE_LAYOUT_TYPE, LightdashTableLayout))
#define LIGHTDASH_TABLE_LAYOUT_CLASS (klass) (G_TYPE_CHECK_CLASS_CAST ((klass), LIGHTDASH_TABLE_LAYOUT_TYPE, LightdashTableLayoutClass))
#define IS_LIGHTDASH_TABLE_LAYOUT (obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGHTDASH_TABLE_LAYOUT_TYPE))
#define IS_LIGHTDASH_TABLE_LAYOUT_CLASS(klass), (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGHTDASH_TABLE_LAYOUT_TYPE))

typedef struct _LightdashTableLayout LightdashTableLayout;
typedef struct _LightdashTableLayoutClass LightdashTableLayoutClass;

struct _LightdashTableLayout
{
	#if GTK_CHECK_VERSION (3, 0, 0)
	GtkGrid parent;
	#else
	GtkTable parent;
	#endif
	
	guint nrows;
	guint ncols;
	
	guint number_children;
	
	guint left_attach;
	guint right_attach;
	guint top_attach;
	guint bottom_attach;
};

struct _LightdashTableLayoutClass
{
	#if GTK_CHECK_VERSION (3, 0, 0)
	GtkGridClass parent_class;
	#else
	GtkTableClass parent_class;
	#endif
};

GType lightdash_table_layout_get_type (void);
GtkWidget* lightdash_table_layout_new (guint rows, guint columns, gboolean homogeneous);
 
void lightdash_table_layout_attach_next (GtkWidget *widget, LightdashTableLayout *table_layout);
void lightdash_table_layout_resize (LightdashTableLayout *table_layout, guint rows, guint columns);
void lightdash_windows_view_update_rows_and_columns (LightdashWindowsView *tasklist, LightdashTableLayout *table_layout);
void lightdash_table_layout_start_from_beginning (LightdashTableLayout *table_layout);
 
G_END_DECLS
 
#endif
