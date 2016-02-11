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
 
 #include <cairo/cairo.h>
 #include <cairo/cairo-xlib.h>
 #include <cairo/cairo-xlib-xrender.h>
 
 #include "lightdash-composited-window.h"
 
 G_DEFINE_TYPE (LightdashCompositedWindow, lightdash_composited_window, G_TYPE_OBJECT)
 
 static void lightdash_composited_window_class_init (LightdashCompositedWindowClass *klass)
 {
	 GObjectClass *object_class = G_OBJECT_CLASS (klass);
	 
	 object_class->finalize = lightdash_composited_window_finalize;
 }
 
 static void lightdash_composited_window_init (LightdashCompositedWindow *self)
 {
	 self->surface = NULL;
	 self->compositor = NULL;
	 self->window = NULL;
	 self->xid = None;
	 self->gdk_window = NULL;
	 self->damage = None;
	 self->compositor = lightdash_compositor_get_default ();
	 
 }
 
 LightdashCompositedWindow * lightdash_composited_window_new_from_window (WnckWindow *window)
 {
	LightdashCompositedWindow *composited_window;
	 
	composited_window = LIGHTDASH_COMPOSITED_WINDOW (g_object_new (LIGHTDASH_TYPE_COMPOSITED_WINDOW, NULL));
	 
	composited_window->window = window;
	XGetWindowAttributes (composited_window->compositor->dpy, composited_window->xid, &composited_window->attr);
	
	composited_window->gdk_window = gdk_x11_window_foreign_new_for_display 
		(gdk_screen_get_display (composited_window->compositor->gdk_screen), composited_window->xid);
	
	//if (task->tasklist->composited 
		//&& task->attr.height != 0)
		//{
			XCompositeRedirectWindow (composited_window->compositor->dpy, composited_window->xid,
					CompositeRedirectAutomatic);
			
			composited_window->surface = lightdash_composited_window_get_window_picture (composited_window);
	
			/* Ignore damage events on excluded windows*/
			if (composited_window->compositor->excluded_gdk_window && 
					composited_window->gdk_window != composited_window->compositor->excluded_gdk_window)
				{
					composited_window->damage = XDamageCreate (composited_window->compositor->dpy, 
									composited_window->xid, 
									XDamageReportDeltaRectangles);
					XDamageSubtract (composited_window->compositor->dpy, composited_window->damage, None, None);				
					XSync (composited_window->compositor->dpy, False);
					gdk_window_add_filter (composited_window->gdk_window, 
								(GdkFilterFunc) lightdash_composited_window_event, 
								composited_window);
				}
		//}
}
