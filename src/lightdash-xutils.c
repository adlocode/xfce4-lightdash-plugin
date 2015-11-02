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

GdkPixbuf*
_lightdash_gdk_pixbuf_get_from_pixmap (GdkPixbuf   *dest,
                                  Pixmap       xpixmap,
                                  int          src_x,
                                  int          src_y,
                                  int          dest_x,
                                  int          dest_y,
                                  int          width,
                                  int          height)
{
  GdkDrawable *drawable;
  GdkPixbuf *retval;
  GdkColormap *cmap;
  
  retval = NULL;
  cmap = NULL;
  
  drawable = gdk_xid_table_lookup (xpixmap);

  if (drawable)
    g_object_ref (G_OBJECT (drawable));
  else
    drawable = gdk_pixmap_foreign_new (xpixmap);

  if (drawable)
    {
      cmap = get_cmap (drawable);

      /* GDK is supposed to do this but doesn't in GTK 2.0.2,
       * fixed in 2.0.3
       */
      if (width < 0)
        gdk_drawable_get_size (drawable, &width, NULL);
      if (height < 0)
        gdk_drawable_get_size (drawable, NULL, &height);

      retval = gdk_pixbuf_get_from_drawable (dest,
                                             drawable,
                                             cmap,
                                             src_x, src_y,
                                             dest_x, dest_y,
                                             width, height);
    }

  if (cmap)
    g_object_unref (G_OBJECT (cmap));
  if (drawable)
    g_object_unref (G_OBJECT (drawable));

  return retval;
}

static GdkColormap*
get_cmap (GdkPixmap *pixmap)
{
  GdkColormap *cmap;

  cmap = gdk_drawable_get_colormap (pixmap);
  if (cmap)
    g_object_ref (G_OBJECT (cmap));

  if (cmap == NULL)
    {
      if (gdk_drawable_get_depth (pixmap) == 1)
        {
          /* try null cmap */
          cmap = NULL;
        }
      else
        {
          /* Try system cmap */
          GdkScreen *screen = gdk_drawable_get_screen (GDK_DRAWABLE (pixmap));
          cmap = gdk_screen_get_system_colormap (screen);
          g_object_ref (G_OBJECT (cmap));
        }
    }

  /* Be sure we aren't going to blow up due to visual mismatch */
  if (cmap &&
#if GTK_CHECK_VERSION(2,21,0)
      (gdk_visual_get_depth (gdk_colormap_get_visual (cmap)) !=
       gdk_drawable_get_depth (pixmap))
#else
      (gdk_colormap_get_visual (cmap)->depth !=
       gdk_drawable_get_depth (pixmap))
#endif
     )
    {
      g_object_unref (G_OBJECT (cmap));
      cmap = NULL;
    }
  
  return cmap;
}
