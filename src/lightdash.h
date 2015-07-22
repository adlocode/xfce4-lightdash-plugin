#ifndef __LIGHTDASH_H__
#define __LIGHTDASH_H__

G_BEGIN_DECLS

typedef struct
{
	XfcePanelPlugin *plugin;
	
	GtkWidget *ebox;
	GtkWidget *button;
	GtkWidget *button_label;
	GtkWidget *lightdash_window;
	
}LightdashPlugin;

G_END_DECLS

#endif //__LIGHTDASH_H__
