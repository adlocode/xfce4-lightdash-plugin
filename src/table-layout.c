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

#include <math.h>
#include "lightdash-window-switcher.h"

void lightdash_table_layout_attach_widget (GtkWidget *widget, LightdashWindowsView *tasklist)
{
	gtk_table_attach_defaults (GTK_TABLE(tasklist->table), widget, tasklist->left_attach, 
						tasklist->right_attach, tasklist->top_attach, tasklist->bottom_attach);
					
					gtk_widget_show_all (widget);
					
					
					if (tasklist->right_attach % tasklist->table_columns == 0)
					{
						tasklist->top_attach++;
						tasklist->bottom_attach++;
						tasklist->left_attach=0;
						tasklist->right_attach=1;
					}
				
					else
					{
						tasklist->left_attach++;
						tasklist->right_attach++;
					}
}

void lightdash_windows_view_update_rows_and_columns (LightdashWindowsView *tasklist)
{
	gint rows, columns;
	
	columns = ceil (sqrt ((double)tasklist->window_counter));
	rows = ceil ((double)tasklist->window_counter / (double)tasklist->table_columns);
	
	if (columns < 2)
		columns = 2;
	if (rows < 2)
		rows = 2;
	
	tasklist->table_columns = columns;
	tasklist->table_rows = rows;
}
