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
static gboolean lightdash_button_enter_notify (GtkWidget        *widget,
                                               GdkEventCrossing *event);
static gboolean lightdash_button_leave_notify (GtkWidget        *widget,
                                               GdkEventCrossing *event);



enum
{
  CLICKED,
  LAST_SIGNAL
};

static guint button_signals[LAST_SIGNAL] = {0};

static void lightdash_button_init (LightdashButton *button)
{
  GtkStyleProvider *provider;
  GtkStyleContext *context;

  context = gtk_widget_get_style_context (GTK_WIDGET (button));

  provider = (GtkStyleProvider *)gtk_css_provider_new ();
  gtk_css_provider_load_from_data (GTK_CSS_PROVIDER (provider),
                                   "#lightdash-window-button .frame1 {\n"
                                   "  border-style: solid;\n"
                                     "  border-color: rgb (33,93,156);\n"
                                    " border-width: 3px;\n"
                                     "  border-radius: 0px;\n"
                                   "}\n", -1, NULL);
  gtk_style_context_add_provider (context, provider, GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

  button->button_release_tag = 0;
  gtk_widget_set_can_focus (GTK_WIDGET (button), TRUE);
  gtk_widget_set_receives_default (GTK_WIDGET (button), TRUE);
  gtk_event_box_set_visible_window (GTK_EVENT_BOX (button), FALSE);
  gtk_widget_set_name (GTK_WIDGET (button), "lightdash-window-button");

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
  widget_class->enter_notify_event = lightdash_button_enter_notify;
  widget_class->leave_notify_event = lightdash_button_leave_notify;

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
  GtkStateFlags state;

  context = gtk_widget_get_style_context (widget);
  gtk_style_context_save (context);
  state = gtk_widget_get_state_flags (widget);
  gtk_widget_get_allocation (widget, &allocation);
  gtk_style_context_add_class (context, "frame1");

  if (gtk_widget_has_focus (widget) || state & GTK_STATE_FLAG_PRELIGHT)
    {
  gtk_render_frame (context,
                    cr,
                    0,
                    0,
                    allocation.width,
                    allocation.height);
    }

  gtk_style_context_remove_class (context, "frame1");
  gtk_style_context_restore (context);

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

static gboolean lightdash_button_enter_notify (GtkWidget        *widget,
                                               GdkEventCrossing *event)
{
  GtkStateFlags new_state;

  new_state = gtk_widget_get_state_flags (widget) &
  ~(GTK_STATE_FLAG_PRELIGHT | GTK_STATE_FLAG_ACTIVE);

  new_state |= GTK_STATE_FLAG_PRELIGHT;

  gtk_widget_set_state_flags (widget, new_state, TRUE);

  gtk_widget_queue_draw (widget);


}

static gboolean lightdash_button_leave_notify (GtkWidget        *widget,
                                               GdkEventCrossing *event)
{
  GtkStateFlags new_state;

  new_state = gtk_widget_get_state_flags (widget) &
  ~(GTK_STATE_FLAG_PRELIGHT | GTK_STATE_FLAG_ACTIVE);

  gtk_widget_set_state_flags (widget, new_state, TRUE);

  gtk_widget_queue_draw (widget);


}
