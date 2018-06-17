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

static void lightdash_button_realize (GtkWidget *widget);
static void lightdash_button_finalize (GObject *object);
gboolean lightdash_button_draw (GtkWidget *widget,
                                cairo_t   *cr);
static gboolean lightdash_button_key_release (GtkWidget *widget,
                                              GdkEventKey *event);


enum
{
  CLICKED,
  LAST_SIGNAL
};

static guint button_signals[LAST_SIGNAL] = {0};

static void lightdash_button_init (LightdashButton *button)
{
  button->button_release_tag = 0;
  gtk_widget_set_can_focus (GTK_WIDGET (button), TRUE);
  gtk_widget_set_receives_default (GTK_WIDGET (button), TRUE);
  gtk_event_box_set_visible_window (GTK_EVENT_BOX (button), FALSE);

  g_signal_connect (GTK_WIDGET (button), "realize",
                    G_CALLBACK (lightdash_button_realize), NULL);

  g_signal_connect (GTK_WIDGET (button), "draw",
                    G_CALLBACK (lightdash_button_draw), NULL);


}

static void lightdash_button_class_init (LightdashButtonClass *klass)
{
  GObjectClass *gobject_class;
  GtkWidgetClass *widget_class;

  gobject_class = G_OBJECT_CLASS (klass);
  widget_class = GTK_WIDGET_CLASS (klass);

  gobject_class->finalize = lightdash_button_finalize;

  widget_class->key_release_event = lightdash_button_key_release;

  button_signals[CLICKED] =
    g_signal_new ("clicked",
                  G_OBJECT_CLASS_TYPE (gobject_class),
                  G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                  0,
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);

}

static void lightdash_button_realize (GtkWidget *widget)
{

  gtk_widget_add_events (widget,
                         GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK
                         | GDK_KEY_RELEASE_MASK);


}

static void lightdash_button_finalize (GObject *object)
{
  LightdashButton *button;

  button = LIGHTDASH_BUTTON (object);

  if (button->button_release_tag)
    {
      g_signal_handler_disconnect (GTK_WIDGET (button), button->button_release_tag);
      button->button_release_tag = 0;
    }
}

gboolean lightdash_button_draw (GtkWidget *widget,
                                cairo_t   *cr)
{
  GtkStyleContext *context;
  GtkAllocation allocation;

  context = gtk_widget_get_style_context (widget);
  gtk_widget_get_allocation (widget, &allocation);

  if (gtk_widget_has_focus (widget))
    {
  gtk_render_focus (context,
                    cr,
                    allocation.x,
                    allocation.y,
                    allocation.width,
                    allocation.height);
    }

  return FALSE;


}

GtkWidget * lightdash_button_new ()
{
  return g_object_new (LIGHTDASH_TYPE_BUTTON, NULL);
}

static gboolean lightdash_button_key_release (GtkWidget *widget,
                                              GdkEventKey *event)
{
  if (event->keyval == GDK_KEY_Return)
    {
  g_signal_emit (LIGHTDASH_BUTTON (widget), button_signals[CLICKED], 0);

  if (GTK_WIDGET_CLASS (lightdash_button_parent_class)->key_release_event)
      return GTK_WIDGET_CLASS (lightdash_button_parent_class)->key_release_event (widget, event);
  return FALSE;
    }
  return FALSE;
}
