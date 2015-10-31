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
 
#ifndef LIGHTDASH_PAGER_H
#define LIGHTDASH_PAGER_H

#include <gtk/gtk.h>
#include <libwnck/libwnck.h>

G_BEGIN_DECLS

#define LIGHTDASH_PAGER_TYPE (lightdash_pager_get_type())
#define LIGHTDASH_PAGER(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGHTDASH_PAGER_TYPE, LightdashPager))
#define LIGHTDASH_PAGER_CLASS (klass) (G_TYPE_CHECK_CLASS_CAST ((klass), LIGHTDASH_PAGER_TYPE, LightdashPagerClass))
#define IS_LIGHTDASH_PAGER (obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGHTDASH_PAGER_TYPE))
#define IS_LIGHTDASH_PAGER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGHTDASH_PAGER_CLASS))

typedef struct _LightdashPager LightdashPager;
typedef struct _LightdashPagerClass LightdashPagerClass;
typedef struct _LightdashPagerPrivate LightdashPagerPrivate;

struct _LightdashPager
{
	GtkContainer parent_instance;
	
	LightdashPagerPrivate *priv;
};

struct _LightdashPagerClass
{
	GtkContainerClass parent_class;
};

GType lightdash_pager_get_type (void);

GtkWidget * lightdash_pager_new ();

G_END_DECLS

#endif /* LIGHTDASH_PAGER_H */
