/* Copyright (C) 2019 adlo
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

#ifndef LIGHTDASH_WINDOW_TRACKER_WINDOW_H
#define LIGHTDASH_WINDOW_TRACKER_WINDOW_H

#include "lightdash-compositor.h"

G_BEGIN_DECLS

#define LIGHTDASH_TYPE_WINDOW_TRACKER_WINDOW (lightdash_window_tracker_window_get_type())
#define LIGHTDASH_WINDOW_TRACKER_WINDOW(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGHTDASH_TYPE_WINDOW_TRACKER_WINDOW, LightdashWindowTrackerWindow))
#define LIGHTDASH_WINDOW_TRACKER_WINDOW_CLASS (klass) (G_TYPE_CHECK_CLASS_CAST ((klass), LIGHTDASH_TYPE_WINDOW_TRACKER_WINDOW, LightdashWindowTrackerWindowClass))
#define IS_LIGHTDASH_WINDOW_TRACKER_WINDOW (obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGHTDASH_TYPE_WINDOW_TRACKER_WINDOW))
#define IS_LIGHTDASH_WINDOW_TRACKER_WINDOW_CLASS(klass), (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGHTDASH_TYPE_WINDOW_TRACKER_WINDOW))

typedef struct _LightdashWindowTrackerWindow LightdashWindowTrackerWindow;
typedef struct _LightdashWindowTrackerWindowClass LightdashWindowTrackerWindowClass;

struct _LightdashWindowTrackerWindow
{
	GObject parent_instance;

  WnckWindow *window;
};

struct _LightdashWindowTrackerWindowClass
{
	GObjectClass parent_class;
};

LightdashWindowTrackerWindow * lightdash_window_tracker_window_new_from_window (WnckWindow *window);
void lightdash_window_tracker_window_get_size (LightdashWindowTrackerWindow *self, gint *width, gint *height);

G_END_DECLS

#endif /* LIGHTDASH_WINDOW_TRACKER_WINDOW_H */

