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
 
#include "lightdash-window-switcher.h"

G_BEGIN_DECLS
 
void lightdash_table_layout_attach_widget (GtkWidget *widget, LightdashWindowsView *tasklist);
void lightdash_windows_view_update_rows_and_columns (LightdashWindowsView *tasklist);
 
G_END_DECLS
 
#endif
