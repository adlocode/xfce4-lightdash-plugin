/* Copyright (C) 2018 adlo
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

#ifndef LIGHTDASH_BUTTON_H
#define LIGHTDASH_BUTTON_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define LIGHTDASH_TYPE_BUTTON (lightdash_button_get_type())
#define LIGHTDASH_BUTTON(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGHTDASH_TYPE_BUTTON, LightdashButton))
#define MY_TASKLIST_CLASS (klass) (G_TYPE_CHECK_CLASS_CAST ((klass), LIGHTDASH_TYPE_BUTTON, LightdashButtonClass))
#define IS_LIGHTDASH_BUTTON (obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGHTDASH_TYPE_BUTTON))
#define IS_LIGHTDASH_BUTTON_CLASS(klass), (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGHTDASH_TYPE_BUTTON))

typedef struct _LightdashButton LightdashButton;
typedef struct _LightdashButtonClass LightdashButtonClass;
typedef LightdashButton LightdashButton;
typedef LightdashButtonClass LightdashButtonClass;

struct _LightdashButton
{
	GtkBin parent_instance;

};



struct _LightdashButtonClass
{
	GtkBinClass parent_class;
};

GtkWidget* lightdash_button_new (void);




G_END_DECLS

#endif /*LIGHTDASH_BUTTON_H*/
