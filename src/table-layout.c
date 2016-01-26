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

static void lightdash_table_layout_class_init (LightdashTableLayoutClass *klass);
static void lightdash_table_layout_init (LightdashTableLayout *grid);
static void lightdash_table_layout_remove (GtkContainer *container, GtkWidget *widget);

#if GTK_CHECK_VERSION (3, 0, 0)
G_DEFINE_TYPE (LightdashTableLayout, lightdash_table_layout, GTK_TYPE_GRID);
#else
G_DEFINE_TYPE (LightdashTableLayout, lightdash_table_layout, GTK_TYPE_TABLE);
#endif


static void lightdash_table_layout_class_init (LightdashTableLayoutClass *klass)

{
	
}
	
static void lightdash_table_layout_init (LightdashTableLayout *table_layout)
{
  
  table_layout->number_children = 0;
  lightdash_table_layout_start_from_beginning (table_layout);
  
  g_signal_connect (G_OBJECT (table_layout), "remove",
			G_CALLBACK (lightdash_table_layout_remove), NULL);

}

GtkWidget* lightdash_table_layout_new (guint rows, guint columns, gboolean homogeneous)
{
	LightdashTableLayout *table_layout;
	
	if (rows == 0)
		rows = 1;
		
	if (columns == 0)
		columns = 1;
		
	
	table_layout = g_object_new (lightdash_table_layout_get_type (), NULL);
	
	#if GTK_CHECK_VERSION (3, 0, 0)
	gtk_grid_set_row_homogeneous (GTK_GRID (table_layout), homogeneous);
	gtk_grid_set_column_homogeneous (GTK_GRID (table_layout), homogeneous);
	#else
	GTK_TABLE  (table_layout)->homogeneous = (homogeneous ? TRUE : FALSE);
	#endif
	
	lightdash_table_layout_resize (LIGHTDASH_TABLE_LAYOUT (table_layout), rows, columns);
	
	return GTK_WIDGET (table_layout);
	
	
}

void lightdash_table_layout_start_from_beginning (LightdashTableLayout *table_layout)
{
	table_layout->left_attach =0;	
	table_layout->right_attach=1;		
	table_layout->top_attach=0;		
	table_layout->bottom_attach=1;
}
	

void lightdash_table_layout_attach_next (GtkWidget *widget, LightdashTableLayout *table_layout)
{
	#if GTK_CHECK_VERSION (3, 0, 0)
	gtk_grid_attach (GTK_GRID (table_layout), widget, table_layout->left_attach, 
				table_layout->top_attach, 1, 1);
	#else
	gtk_table_attach_defaults (GTK_TABLE(table_layout), widget, table_layout->left_attach, 
						table_layout->right_attach, table_layout->top_attach, table_layout->bottom_attach);
	#endif
					
					gtk_widget_show_all (widget);
					
					
					if (table_layout->right_attach % table_layout->ncols == 0)
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
	table_layout->number_children++;
}

void lightdash_table_layout_resize (LightdashTableLayout *table_layout, guint rows, guint columns)
{
	if (rows == 0)
		rows = 1;
	if (columns == 0)
		columns = 1;
	
	#if GTK_CHECK_VERSION (3, 0, 0)
	#else	
	gtk_table_resize (GTK_TABLE (table_layout), rows, columns);
	#endif
	
	table_layout->nrows = rows;
	table_layout->ncols = columns;
}

static void lightdash_table_layout_remove (GtkContainer *container, GtkWidget *widget)
{
	LightdashTableLayout *table_layout = LIGHTDASH_TABLE_LAYOUT (container);
	table_layout->number_children--;
}
	

void lightdash_table_layout_update_rows_and_columns (LightdashTableLayout *table_layout, guint *table_rows, guint *table_columns)
{
	gint rows, columns;
	
	columns = ceil (sqrt ((double)table_layout->number_children));
	rows = ceil ((double)table_layout->number_children / (double)*table_columns);
	
	if (columns < 2)
		columns = 2;
	if (rows < 2)
		rows = 2;
	
	*table_columns = columns;
	*table_rows = rows;
}
