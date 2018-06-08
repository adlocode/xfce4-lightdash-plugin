/* Xlib utils */
/* vim: set sw=2 et: */

/*
 * Copyright (C) 2001 Havoc Pennington
 * Copyright (C) 2005-2007 Vincent Untz
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
 
 #ifndef LIGHTDASH_XUTILS_H
 #define LIGHTDASH_XUTILS_H
 
 #include <gtk/gtk.h>
 #include <libwnck/libwnck.h>
 #include <X11/Xlib.h>
 #include <gdk/gdk.h>
 #include <gdk/gdkx.h>
 #include <cairo/cairo.h>
 #include <cairo/cairo-xlib.h>

GdkPixbuf*
_lightdash_gdk_pixbuf_get_from_pixmap (Screen *screen,
                                  Pixmap  xpixmap);

#endif /* LIGHTDASH_XUTILS_H */
