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
 
 #include "lightdash-xutils.h"

static cairo_surface_t *
_lightdash_cairo_surface_get_from_pixmap (Screen *screen,
                                     Pixmap  xpixmap)
{
  cairo_surface_t *surface;
  Display *display;
  Window root_return;
  int x_ret, y_ret;
  unsigned int w_ret, h_ret, bw_ret, depth_ret;
  XWindowAttributes attrs;

  surface = NULL;
  display = DisplayOfScreen (screen);

  gdk_x11_display_error_trap_push (gdk_display_get_default ());

  if (!XGetGeometry (display, xpixmap, &root_return,
                     &x_ret, &y_ret, &w_ret, &h_ret, &bw_ret, &depth_ret))
    goto TRAP_POP;

  if (depth_ret == 1)
    {
      surface = cairo_xlib_surface_create_for_bitmap (display,
                                                      xpixmap,
                                                      screen,
                                                      w_ret,
                                                      h_ret);
    }
  else
    {
      if (!XGetWindowAttributes (display, root_return, &attrs))
        goto TRAP_POP;

      if (depth_ret == attrs.depth)
	{
	  surface = cairo_xlib_surface_create (display,
					       xpixmap,
					       attrs.visual,
					       w_ret, h_ret);
	}
      else
	{
#if HAVE_CAIRO_XLIB_XRENDER
	  int std;

	  switch (depth_ret) {
	  case 1: std = PictStandardA1; break;
	  case 4: std = PictStandardA4; break;
	  case 8: std = PictStandardA8; break;
	  case 24: std = PictStandardRGB24; break;
	  case 32: std = PictStandardARGB32; break;
	  default: goto TRAP_POP;
	  }

	  surface = cairo_xlib_surface_create_with_xrender_format (display,
								   xpixmap,
								   attrs.screen,
								   XRenderFindStandardFormat (display, std),
								   w_ret, h_ret);
#endif
	}
    }

TRAP_POP:
  gdk_x11_display_error_trap_pop (gdk_display_get_default ());

  return surface;
}

GdkPixbuf*
_lightdash_gdk_pixbuf_get_from_pixmap (Screen *screen,
                                  Pixmap  xpixmap)
{
  cairo_surface_t *surface;
  GdkPixbuf *retval;

  surface = _lightdash_cairo_surface_get_from_pixmap (screen, xpixmap);

  if (surface == NULL)
    return NULL;

  retval = gdk_pixbuf_get_from_surface (surface,
                                        0,
                                        0,
                                        cairo_xlib_surface_get_width (surface),
                                        cairo_xlib_surface_get_height (surface));
  cairo_surface_destroy (surface);

  return retval;
}
