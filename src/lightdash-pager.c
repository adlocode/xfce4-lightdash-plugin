/* Copyright (C) 2015 adlo
 * 
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, see <http://www.gnu.org/licenses/>
 */
 
#include <gtk/gtk.h>
#include <libwnck/libwnck.h>
#include <cairo/cairo.h>

struct _LightdashPagerPrivate
{
	WnckScreen *screen;
};

#define LIGHTDASH_PAGER_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), LIGHTDASH_PAGER_TYPE, LightdashPagerPrivate))

static void lightdash_pager_init (LightdashPager *pager)
{
	static const GtkTargetEntry targets [] = {
			{"application/x-wnck-window-id", 0, 0}
		};
		
	pager->priv = LIGHTDASH_PAGER_GET_PRIVATE (pager);
}

static void lightdash_pager_class_init (LightdashPagerClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
	
	g_type_class_add_private (klass, sizeof (LightdashPagerPrivate));
	
}

#if GTK_CHECK_VERSION (3, 0, 0)
static gboolean lightdash_pager_draw (GtkWidget *widget, cairo_t *cr)
#else
static gboolean lightdash_pager_expose_event (GtkWidget *widget, GdkEventExpose *event)
#endif
{
	
}

GtkWidget * lightdash_pager_new ()
{
	return GTK_WIDGET(g_object_new (lightdash_pager_get_type (), NULL));
}
