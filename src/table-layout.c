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

#include <gtk/gtk.h>
#include <math.h>
#include "table-layout.h"
#include "lightdash-window-switcher.h"

static void lightdash_table_layout_class_init (LightdashTableLayoutClass *klass);

static void lightdash_table_layout_init (LightdashTableLayout *grid);


GType lightdash_table_layout_get_type (void)
{
	static GType lightdash_table_layout_type = 0;
	
	if (!lightdash_table_layout_type)
	{
		const GTypeInfo lightdash_table_layout_info =
		{
			sizeof(LightdashTableLayoutClass),
			NULL, /*base_init*/
			NULL, /*base_finalize*/
			(GClassInitFunc) lightdash_table_layout_class_init,
			NULL, /*class_finalize*/
			NULL,
			sizeof(LightdashTableLayout),
			0,
			(GInstanceInitFunc) lightdash_table_layout_init,
		};
		lightdash_table_layout_type = g_type_register_static (GTK_TYPE_TABLE, "LightdashTableLayout", &lightdash_table_layout_info, 0);
	}
	return lightdash_table_layout_type;
}

static void lightdash_table_layout_class_init (LightdashTableLayoutClass *klass)

{	
	
}
	
static void lightdash_table_layout_init (LightdashTableLayout *table_layout)
{
  
  lightdash_table_layout_reset_values (table_layout);

}

GtkWidget* lightdash_table_layout_new (guint rows, guint columns, gboolean homogeneous)
{
	LightdashTableLayout *table_layout;
	
	if (rows == 0)
		rows = 1;
		
	if (columns == 0)
		columns = 1;
		
	
	table_layout = g_object_new (lightdash_table_layout_get_type (), NULL);
	
	
	GTK_TABLE  (table_layout)->homogeneous = (homogeneous ? TRUE : FALSE);
	
	gtk_table_resize (GTK_TABLE (table_layout), rows, columns);
	
	return GTK_WIDGET (table_layout);
	
	
}

void lightdash_table_layout_reset_values (LightdashTableLayout *table_layout)
{
	table_layout->left_attach =0;	
	table_layout->right_attach=1;		
	table_layout->top_attach=0;		
	table_layout->bottom_attach=1;
}
	

void lightdash_table_layout_attach_next (GtkWidget *widget, LightdashTableLayout *table_layout)
{
	gtk_table_attach_defaults (GTK_TABLE(table_layout), widget, table_layout->left_attach, 
						table_layout->right_attach, table_layout->top_attach, table_layout->bottom_attach);
					
					gtk_widget_show_all (widget);
					
					
					if (table_layout->right_attach % GTK_TABLE (table_layout)->ncols == 0)
					{
						table_layout->top_attach++;
						table_layout->bottom_attach++;
						table_layout->left_attach=0;
						table_layout->right_attach=1;
					}
				
					else
					{
						table_layout->left_attach++;
						table_layout->right_attach++;
					}
}

void lightdash_table_layout_resize (LightdashTableLayout *table_layout, guint rows, guint columns)
{
	gtk_table_resize (GTK_TABLE (table_layout), rows, columns);
}

void lightdash_windows_view_update_rows_and_columns (LightdashWindowsView *windows_view)
{
	gint rows, columns;
	
	columns = ceil (sqrt ((double)windows_view->window_counter));
	rows = ceil ((double)windows_view->window_counter / (double)windows_view->table_columns);
	
	if (columns < 2)
		columns = 2;
	if (rows < 2)
		rows = 2;
	
	windows_view->table_columns = columns;
	windows_view->table_rows = rows;
}
