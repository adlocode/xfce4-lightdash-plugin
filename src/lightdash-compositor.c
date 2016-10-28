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
	
	/* Trap all X errors throughout the lifetime of this object */
	gdk_error_trap_push ();
	
	wnck_screen_force_update (compositor->screen);

	for (int i = 0; i < ScreenCount (compositor->dpy); i++)
		XCompositeRedirectSubwindows (compositor->dpy, RootWindow (compositor->dpy, i),
			CompositeRedirectAutomatic);

	XDamageQueryExtension (compositor->dpy, &dv, &dr);
	gdk_x11_register_standard_event_type (gdk_screen_get_display (compositor->gdk_screen),
		dv, dv + XDamageNotify);
 }
 
 WnckScreen * lightdash_compositor_get_wnck_screen (LightdashCompositor *compositor)
 {
	 return compositor->screen;
 }

void lightdash_compositor_set_excluded_window (LightdashCompositor *compositor, GdkWindow *gdk_window)
{
	compositor->excluded_gdk_window = gdk_window;
}

 WnckWindow * lightdash_compositor_get_root_window (LightdashCompositor *compositor)
 {
	 GList *li;
	 
	 for (li = wnck_screen_get_windows (compositor->screen); li != NULL; li = li->next)
	 {
		 WnckWindow *win = WNCK_WINDOW (li->data);
		 
		 if (wnck_window_get_window_type (win) == WNCK_WINDOW_DESKTOP)
		 {
			 return win;
		 }
	 }
	 
	 return NULL;
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
