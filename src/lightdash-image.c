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

#include "lightdash-image.h"

G_DEFINE_TYPE (LightdashImage, lightdash_image, GTK_TYPE_DRAWING_AREA);

static void lightdash_image_init (LightdashImage *image)
{
	#if GTK_CHECK_VERSION (3, 0, 0)

	#else
	GtkDrawingArea *darea;
	darea = GTK_DRAWING_AREA (image);
	darea->draw_data = NULL;
	#endif
	
	gtk_widget_set_has_window (GTK_WIDGET (image), FALSE);
}

static void lightdash_image_class_init (LightdashImageClass *klass)
{
	
}

GtkWidget * lightdash_image_new ()
{
	return g_object_new (LIGHTDASH_TYPE_IMAGE, NULL);
}
