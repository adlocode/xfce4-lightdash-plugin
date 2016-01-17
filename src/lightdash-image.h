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

#ifndef _LIGHTDASH_IMAGE_H_
#define _LIGHTDASH_IMAGE_H_

#include <gtk/gtk.h>
#include <cairo/cairo.h>

#define LIGHTDASH_TYPE_IMAGE lightdash_image_get_type ()

G_BEGIN_DECLS

#define LIGHTDASH_IMAGE_TYPE (lightdash_image_get_type())
#define LIGHTDASH_IMAGE(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGHTDASH_IMAGE_TYPE, LightdashImage))
#define LIGHTDASH_IMAGE_CLASS (klass) (G_TYPE_CHECK_CLASS_CAST ((klass), LIGHTDASH_IMAGE_TYPE, LightdashImageClass))
#define IS_LIGHTDASH_IMAGE (obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGHTDASH_IMAGE_TYPE))
#define IS_LIGHTDASH_IMAGE_CLASS(klass), (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGHTDASH_IMAGE_TYPE))

typedef struct _LightdashImage LightdashImage;
typedef struct _LightdashImageClass LightdashImageClass;

struct _LightdashImage
{
	GtkDrawingArea parent_instance;
	
	
};

struct _LightdashImageClass
{
	GtkDrawingAreaClass parent_class;
};

GtkWidget * lightdash_image_new ();

G_END_DECLS

#endif /* _LIGHTDASH_IMAGE_H_ */
