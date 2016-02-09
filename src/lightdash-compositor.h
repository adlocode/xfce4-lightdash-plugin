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
 
#ifndef LIGHTDASH_COMPOSITOR_H
#define LIGHTDASH_COMPOSITOR_H

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xcomposite.h>
#include <X11/extensions/Xrender.h>
#include <X11/extensions/Xdamage.h>
#include <gdk/gdkx.h>
#include <libwnck/libwnck.h>

G_BEGIN_DECLS

#define LIGHTDASH_TYPE_COMPOSITOR (lightdash_compositor_get_type())
#define LIGHTDASH_COMPOSITOR(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGHTDASH_TYPE_COMPOSITOR, LightdashCompositor))
#define LIGHTDASH_COMPOSITOR_CLASS (klass) (G_TYPE_CHECK_CLASS_CAST ((klass), LIGHTDASH_TYPE_COMPOSITOR, LightdashCompositorClass))
#define IS_LIGHTDASH_COMPOSITOR (obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGHTDASH_TYPE_COMPOSITOR))
#define IS_LIGHTDASH_COMPOSITOR_CLASS(klass), (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGHTDASH_TYPE_COMPOSITOR))

typedef struct _LightdashCompositor LightdashCompositor;
typedef struct _LightdashCompositorClass LightdashCompositorClass;

struct _LightdashCompositor
{
	GObject parent_instance;
	
	WnckScreen *screen;
	GdkScreen *gdk_screen;
	Display *dpy;
};

struct _LightdashCompositorClass
{
	GObjectClass parent_class;
};

LightdashCompositor * lightdash_compositor_get_default ();
WnckScreen * lightdash_compositor_get_wnck_screen (LightdashCompositor *compositor);

G_END_DECLS

#endif /* LIGHTDASH_COMPOSITOR_H */
