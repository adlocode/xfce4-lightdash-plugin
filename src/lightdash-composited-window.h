/* Copyright (C) 2016 adlo
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
 
#ifndef LIGHTDASH_COMPOSITED_WINDOW_H
#define LIGHTDASH_COMPOSITED_WINDOW_H

#include "lightdash-compositor.h"

G_BEGIN_DECLS

#define LIGHTDASH_TYPE_COMPOSITED_WINDOW (lightdash_composited_window_get_type())
#define LIGHTDASH_COMPOSITED_WINDOW(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGHTDASH_TYPE_COMPOSITED_WINDOW, LightdashCompositedWindow))
#define LIGHTDASH_COMPOSITED_WINDOW_CLASS (klass) (G_TYPE_CHECK_CLASS_CAST ((klass), LIGHTDASH_TYPE_COMPOSITED_WINDOW, LightdashCompositedWindowClass))
#define IS_LIGHTDASH_COMPOSITED_WINDOW (obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGHTDASH_TYPE_COMPOSITED_WINDOW))
#define IS_LIGHTDASH_COMPOSITED_WINDOW_CLASS(klass), (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGHTDASH_TYPE_COMPOSITED_WINDOW))

typedef struct _LightdashCompositedWindow LightdashCompositedWindow;
typedef struct _LightdashCompositedWindowClass LightdashCompositedWindowClass;

struct _LightdashCompositedWindow
{
	GObject parent_instance;
	
	cairo_surface_t *surface;
	
	LightdashCompositor *compositor;
	
	WnckWindow *window;
	Window xid;
	GdkWindow *gdk_window;
	XWindowAttributes attr;
	Damage damage;
};

struct _LightdashCompositedWindowClass
{
	GObjectClass parent_class;
};

LightdashCompositedWindow * lightdash_composited_window_new_from_window (WnckWindow *window);

G_END_DECLS

#endif /* LIGHTDASH_COMPOSITED_WINDOW_H */

