/*
 * Copyright (C) 2015 adlo
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
 * You should have received a copy of the GNU General Public License
 * along with this library; if not, see <http://www.gnu.org/licenses/>
 */

#ifndef __LIGHTDASH_H__
#define __LIGHTDASH_H__

#include <libxfce4util/libxfce4util.h>
#include <libxfce4panel/xfce-panel-plugin.h>
#include <libxfce4panel/xfce-hvbox.h>

G_BEGIN_DECLS

typedef struct
{
	XfcePanelPlugin *plugin;
	
	GtkWidget *button;
	GtkWidget *button_label;
	GtkWidget *lightdash_window;
	
}LightdashPlugin;

G_END_DECLS

#endif //__LIGHTDASH_H__
