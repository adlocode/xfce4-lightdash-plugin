/*
 * Copyright (C) 2015 adlo
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this library; if not, see <http://www.gnu.org/licenses/>
 */
 
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif


#include <gtk/gtk.h>
#include <libxfce4util/libxfce4util.h>
#include <libxfce4panel/xfce-panel-plugin.h>
#include <libxfce4panel/xfce-hvbox.h>
#include <stdlib.h>

#include "lightdash.h"
#include "lightdash-dialogs.h"
#include "appfinder-window.h"

#include <garcon/garcon.h>
#include <xfconf/xfconf.h>

#include <src/appfinder-window.h>
#include <src/appfinder-private.h>
#include <src/appfinder-model.h>

#define BUTTON_TITLE_DEFAULT _("Activities")
#define DEFAULT_OPACITY 94

static gboolean            opt_collapsed = FALSE;



#ifdef DEBUG
static GHashTable         *objects_table = NULL;
static guint               objects_table_count = 0;
#endif





#ifdef DEBUG
static void
appfinder_refcount_debug_weak_notify (gpointer  data,
                                      GObject  *where_the_object_was)
{
  /* remove the unreffed object pixbuf from the table */
  if (!g_hash_table_remove (objects_table, where_the_object_was))
    appfinder_assert_not_reached ();
}







void
appfinder_refcount_debug_add (GObject     *object,
                              const gchar *description)
{
  /* silently ignore objects that are already registered */
  if (object != NULL
      && g_hash_table_lookup (objects_table, object) == NULL)
    {
      objects_table_count++;
      g_object_weak_ref (G_OBJECT (object), appfinder_refcount_debug_weak_notify, NULL);
      g_hash_table_insert (objects_table, object, g_strdup (description));
    }
}





#endif


static void
lightdash_construct (XfcePanelPlugin *plugin);

XFCE_PANEL_PLUGIN_REGISTER (lightdash_construct);

void lightdash_save (XfcePanelPlugin *plugin, LightdashPlugin *lightdash)
{
	XfceRc *rc;
	gchar *file;
	
	file = xfce_panel_plugin_save_location (plugin, TRUE);
	
	if (G_UNLIKELY (file == NULL))
	{
		return;
	}
	
	rc = xfce_rc_simple_open (file, FALSE);
	g_free (file);
	
	if (G_LIKELY (rc != NULL))
	{
		if (lightdash->button_title)
			xfce_rc_write_entry (rc, "button-title", lightdash->button_title);
				
			xfce_rc_write_int_entry (rc, "opacity", lightdash->opacity);
			
			xfce_rc_close (rc);
		
	}
}

static void lightdash_read (LightdashPlugin *lightdash)
{
	XfceRc *rc;
	gchar *file;
	const gchar *value;
	
	file = xfce_panel_plugin_save_location (lightdash->plugin, TRUE);
	
	if (G_LIKELY (file != NULL))
	{
		rc = xfce_rc_simple_open (file, TRUE);
		
		g_free (file);
		
		if (G_LIKELY (rc != NULL))
		{
			value = xfce_rc_read_entry (rc, "button-title", BUTTON_TITLE_DEFAULT);
			lightdash->button_title = g_strdup (value);
			lightdash->opacity = xfce_rc_read_int_entry (rc, "opacity", DEFAULT_OPACITY);
			
			xfce_rc_close (rc);
			
			return;
		}
		
	}
	
	lightdash->button_title = BUTTON_TITLE_DEFAULT;
	lightdash->opacity = DEFAULT_OPACITY;
}

static LightdashPlugin *
lightdash_new (XfcePanelPlugin *plugin)
{
	LightdashPlugin *lightdash;
	
	lightdash = panel_slice_new0 (LightdashPlugin);
	
	lightdash->plugin = plugin;
	
	lightdash->button_title = NULL;
	
	lightdash_read (lightdash);
		
	lightdash->button = xfce_panel_create_toggle_button ();
	lightdash->button_label = gtk_label_new (lightdash->button_title);
	
	lightdash->opacity = DEFAULT_OPACITY;
	
	gtk_container_add (GTK_CONTAINER (lightdash->button), (lightdash->button_label));
	gtk_widget_show (lightdash->button_label);
	gtk_container_add (GTK_CONTAINER (plugin), lightdash->button);
	
	gtk_widget_show_all (lightdash->button);
	
	return lightdash;
}

static void
lightdash_free (XfcePanelPlugin *plugin,
				LightdashPlugin *lightdash)
{
	if (G_LIKELY (lightdash->button_title != NULL))
		g_free (lightdash->button_title);
	
	panel_slice_free (LightdashPlugin, lightdash);
}

static gboolean
lightdash_size_changed (XfcePanelPlugin *plugin,
						gint size,
						LightdashPlugin *lightdash)
{
	GtkOrientation orientation;
	
	orientation = xfce_panel_plugin_get_orientation (plugin);
	
	if (orientation == GTK_ORIENTATION_HORIZONTAL)
		gtk_widget_set_size_request (GTK_WIDGET (plugin), -1, size);
	else
		gtk_widget_set_size_request (GTK_WIDGET (plugin), size, -1);
		
	return TRUE;
}

static void
lightdash_button_clicked (GtkToggleButton *button, LightdashPlugin *lightdash)
{
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (lightdash->button)) == FALSE)
        {
			gtk_widget_hide (GTK_WIDGET (lightdash->lightdash_window));
		}
	else
	{
		gtk_widget_show (GTK_WIDGET (lightdash->lightdash_window));
	}
}

static void
lightdash_window_unmap (GtkWidget *widget, LightdashPlugin *lightdash)
{
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (lightdash->button), FALSE);
}

static gboolean lightdash_remote_event 
(XfcePanelPlugin *plugin, gchar *name, GValue *value, LightdashPlugin *lightdash)
{
	if (strcmp (name, "popup"))
	{
		return FALSE;
	}
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (lightdash->button)))
	{
		gtk_widget_hide (GTK_WIDGET (lightdash->lightdash_window));
	}
	else
	{
		gtk_widget_show (GTK_WIDGET (lightdash->lightdash_window));
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (lightdash->button), TRUE);
	}
	
	return TRUE;
}

static void
lightdash_construct (XfcePanelPlugin *plugin)
{
	LightdashPlugin *lightdash;
  
	xfce_textdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");
	
	lightdash = lightdash_new (plugin);
	
	

  /* set translation domain */
 // xfce_textdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");


  /* create initial window */
  lightdash->lightdash_window = lightdash_window_new (NULL, !opt_collapsed, lightdash);
	
	xfce_panel_plugin_add_action_widget (plugin, lightdash->button);
	
	xfce_panel_plugin_menu_show_configure (plugin);
	
	g_signal_connect (G_OBJECT (plugin), "free-data",
					G_CALLBACK (lightdash_free), lightdash);
						
	g_signal_connect (G_OBJECT (plugin), "save",
					G_CALLBACK (lightdash_save), lightdash);
						
	g_signal_connect (G_OBJECT (lightdash->button), "toggled",
					G_CALLBACK (lightdash_button_clicked), lightdash);
						
	g_signal_connect (G_OBJECT (plugin), "size-changed",
                    G_CALLBACK (lightdash_size_changed), lightdash);
                    
    g_signal_connect (G_OBJECT (lightdash->lightdash_window), "unmap",
                    G_CALLBACK (lightdash_window_unmap), lightdash);
                    
    g_signal_connect (G_OBJECT (plugin), "configure-plugin",
					G_CALLBACK (lightdash_configure), lightdash);
                    
    g_signal_connect (G_OBJECT (plugin), "remote-event",
                    G_CALLBACK (lightdash_remote_event), lightdash);
	
}
	
	
