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
 
#include <libxfce4util/libxfce4util.h>
#include <libxfce4panel/xfce-panel-plugin.h>
#include <libxfce4ui/libxfce4ui.h>
#include "lightdash.h"
#include "lightdash-dialogs.h"
 
static void lightdash_configure_response (GtkWidget *dialog,
											gint response,
											LightdashPlugin *lightdash)
{
	/* remove the dialog data from the plugin */
      g_object_set_data (G_OBJECT (lightdash->plugin), "dialog", NULL);

      /* unlock the panel menu */
      xfce_panel_plugin_unblock_menu (lightdash->plugin);

      /* destroy the properties dialog */
      gtk_widget_destroy (dialog);
}
				
void lightdash_configure (XfcePanelPlugin *plugin,
							LightdashPlugin *lightdash)
{
	GtkWidget *dialog;
	
	xfce_panel_plugin_block_menu (plugin);
	
	dialog = xfce_titled_dialog_new_with_buttons (_("Lightdash"),
						GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (plugin))),
						GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_NO_SEPARATOR,
						GTK_STOCK_HELP, GTK_RESPONSE_HELP,
						GTK_STOCK_CLOSE, GTK_RESPONSE_OK,
						NULL);
						
	g_object_set_data (G_OBJECT (plugin), "dialog", dialog);
	
	g_signal_connect (G_OBJECT (dialog), "response",
			G_CALLBACK (lightdash_configure_response), lightdash);
						
	gtk_widget_show (dialog);
}
