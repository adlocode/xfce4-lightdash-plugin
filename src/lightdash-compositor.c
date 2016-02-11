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
 
 #include "lightdash-compositor.h"
 
 G_DEFINE_TYPE (LightdashCompositor, lightdash_compositor, G_TYPE_OBJECT);
 
 static int lightdash_compositor_xhandler_xerror (Display *dpy, XErrorEvent *e);
 
 static LightdashCompositor *_lightdash_compositor_singleton = NULL;
 
 static void lightdash_compositor_class_init (LightdashCompositorClass *klass)
 {
	 GObjectClass *object_class = G_OBJECT_CLASS (klass);
	 
 }
 
 static void lightdash_compositor_init (LightdashCompositor *compositor)
 {
	int dv, dr;
	
	compositor->excluded_gdk_window = NULL;
	compositor->screen = wnck_screen_get_default();
	compositor->gdk_screen = gdk_screen_get_default ();
	compositor->dpy = gdk_x11_get_default_xdisplay ();
	
	XSetErrorHandler (lightdash_compositor_xhandler_xerror);
	
	wnck_screen_force_update (compositor->screen);
	
	XDamageQueryExtension (compositor->dpy, &dv, &dr);
	gdk_x11_register_standard_event_type (gdk_screen_get_display (compositor->gdk_screen),
		dv, dv + XDamageNotify);
 }
 
 WnckScreen * lightdash_compositor_get_wnck_screen (LightdashCompositor *compositor)
 {
	 return compositor->screen;
 }
 
 static int lightdash_compositor_xhandler_xerror (Display *dpy, XErrorEvent *e)
{
	gchar text [64];
	
		g_print ("%s", "X11 error ");
		g_print ("%d", e->error_code);
		g_print ("%s", " - ");
		XGetErrorText (dpy, e->error_code, text, 64);
		g_print ("%s", text);
		g_print ("%s", "\n");
		
		return 0;
}

void lightdash_compositor_set_excluded_window (LightdashCompositor *compositor, GdkWindow *gdk_window)
{
	compositor->excluded_gdk_window = gdk_window;
}
 
 LightdashCompositor * lightdash_compositor_get_default ()
 {
	if (_lightdash_compositor_singleton == NULL)
	{
		_lightdash_compositor_singleton =
			LIGHTDASH_COMPOSITOR (g_object_new (LIGHTDASH_TYPE_COMPOSITOR, NULL));
	}
	else g_object_ref (_lightdash_compositor_singleton);
	
	return _lightdash_compositor_singleton;
}
