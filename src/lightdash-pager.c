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
#include "lightdash-pager.h"

struct _LightdashPagerPrivate
{
	WnckScreen *screen;
};

G_DEFINE_TYPE (LightdashPager, lightdash_pager, GTK_TYPE_WIDGET);

#define LIGHTDASH_PAGER_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), LIGHTDASH_PAGER_TYPE, LightdashPagerPrivate))

static void lightdash_pager_realize (GtkWidget *widget);

static void lightdash_pager_init (LightdashPager *pager)
{
	static const GtkTargetEntry targets [] = {
			{"application/x-wnck-window-id", 0, 0}
		};
		
	pager->priv = LIGHTDASH_PAGER_GET_PRIVATE (pager);
	
	pager->priv->screen = NULL;
}

static void lightdash_pager_class_init (LightdashPagerClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
	
	g_type_class_add_private (klass, sizeof (LightdashPagerPrivate));
	
	widget_class->realize = lightdash_pager_realize;
	
}

static void lightdash_pager_realize (GtkWidget *widget)
{
	GdkWindowAttr attributes;
	gint attributes_mask;
	LightdashPager *pager;
	GtkAllocation allocation;
	GdkWindow *window;
	GtkStyle *style;
	GtkStyle *new_style;
	
	pager = LIGHTDASH_PAGER (widget);
	
	gtk_widget_set_realized (widget, TRUE);
	
	gtk_widget_get_allocation (widget, &allocation);
	
	attributes.window_type = GDK_WINDOW_CHILD;
	attributes.x = allocation.x;
	attributes.y = allocation.y;
	attributes.width = allocation.width;
	attributes.height = allocation.height;
	attributes.wclass = GDK_INPUT_OUTPUT;
	attributes.visual = gtk_widget_get_visual (widget);
	#if GTK_CHECK_VERSION (3, 0, 0)
	#else
	attributes.colormap = gtk_widget_get_colormap (widget);
	#endif
	attributes.event_mask = gtk_widget_get_events (widget) |GDK_EXPOSURE_MASK |
							GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK |
							GDK_LEAVE_NOTIFY_MASK | GDK_POINTER_MOTION_MASK |
							GDK_POINTER_MOTION_HINT_MASK;
	#if GTK_CHECK_VERSION (3, 0, 0)
	attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL;
	#else
	attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;
	#endif
	
	window = gdk_window_new (gtk_widget_get_parent_window (widget), &attributes, attributes_mask);
	gtk_widget_set_window (widget, window);
	gdk_window_set_user_data (window, widget);
	
	style = gtk_widget_get_style (widget);
	
	new_style = gtk_style_attach (style, window);
	if (new_style != style)
	{
		gtk_widget_set_style (widget, style);
		style = new_style;
	}
	
	gtk_style_set_background (style, window, GTK_STATE_NORMAL);
	
	if (pager->priv->screen == NULL)
		pager->priv->screen = wnck_screen_get_default ();
	
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
