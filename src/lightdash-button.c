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

#include "lightdash-button.h"

G_DEFINE_TYPE (LightdashButton, lightdash_button, GTK_TYPE_EVENT_BOX);


enum
{
  CLICKED,
  LAST_SIGNAL
};

static guint button_signals[LAST_SIGNAL] = {0};

static void lightdash_button_init (LightdashButton *button)
{
  button->button_release_tag = 0;
  gtk_event_box_set_visible_window (GTK_EVENT_BOX (button), FALSE);
  gtk_widget_add_events (GTK_WIDGET (button),
                         GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);

}

static void lightdash_button_class_init (LightdashButtonClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);

  button_signals[CLICKED] =
    g_signal_new ("clicked",
                  G_OBJECT_CLASS_TYPE (gobject_class),
                  G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                  0,
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);

}

GtkWidget * lightdash_button_new ()
{
  return g_object_new (LIGHTDASH_TYPE_BUTTON, NULL);
}
