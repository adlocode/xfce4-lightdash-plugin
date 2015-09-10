# vi:set ts=8 sw=8 noet ai nocindent syntax=automake:

AM_CPPFLAGS = \
	-I$(top_srcdir) \
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\" \
	-DG_LOG_DOMAIN=\"xfce4-lightdash\" \
	$(PLATFORM_CPPFLAGS)

plugin_LTLIBRARIES = \
	liblightdash.la

plugindir = \
	$(libdir)/xfce4/panel/plugins

xfce4_lightdash_built_sources = \
	appfinder-preferences-ui.h

liblightdash_la_SOURCES = \
	$(xfce4_lightdash_built_sources) \
	appfinder-actions.c \
	appfinder-actions.h \
	appfinder-category-model.c \
	appfinder-category-model.h \
	appfinder-model.c \
	appfinder-model.h \
	appfinder-preferences.c \
	appfinder-preferences.h \
	appfinder-private.h \
	appfinder-window.c \
	appfinder-window.h \
	lightdash-window-switcher.c \
	lightdash-window-switcher.h \
	lightdash.c \
	lightdash.h \
	lightdash-dialogs.c \
	lightdash-dialogs.h

liblightdash_la_CFLAGS = \
	-DWNCK_I_KNOW_THIS_IS_UNSTABLE \
	$(GLIB_CFLAGS) \
	$(GTHREAD_CFLAGS) \
	$(GTK_CFLAGS) \
	$(LIBXFCE4UTIL_CFLAGS) \
	$(LIBXFCE4UI_CFLAGS) \
	$(GARCON_CFLAGS) \
	$(GIO_CFLAGS) \
	$(XFCONF_CFLAGS) \
	$(LIBXFCE4PANEL_CFLAGS) \
	$(LIBWNCK_CFLAGS) \
	$(EXO_CFLAGS) \
	$(X11_CFLAGS) \
	$(XCOMPOSITE_CFLAGS) \
	$(XRENDER_CFLAGS) \
	$(XDAMAGE_CFLAGS) \
	$(CAIRO_XLIB_XRENDER_CFLAGS) \
	$(PLATFORM_CFLAGS)

liblightdash_la_LIBADD = \
	$(GLIB_LIBS) \
	$(GTHREAD_LIBS) \
	$(GTK_LIBS) \
	$(LIBXFCE4UTIL_LIBS) \
	$(LIBXFCE4UI_LIBS) \
	$(GARCON_LIBS) \
	$(XFCONF_LIBS) \
	$(GIO_LIBS) \
	$(LIBWNCK_LIBS) \
	$(EXO_LIBS) \
	$(LIBXFCE4PANEL_LIBS) \
	$(X11_LIBS) \
	$(XCOMPOSITE_LIBS) \
	$(XRENDER_LIBS) \
	$(XDAMAGE_LIBS) \
	$(LIBM) \
	$(CAIRO_XLIB_XRENDER_LIBS)

liblightdash_la_LDFLAGS = \
	-no-undefined \
	$(PLATFORM_LDFLAGS)


if MAINTAINER_MODE

appfinder-preferences-ui.h: $(srcdir)/lightdash-preferences.glade Makefile
	$(AM_V_GEN) exo-csource --static --strip-comments --strip-content --name=lightdash_preferences_ui $< >$@

DISTCLEANFILES = \
	$(xfce4_lightdash_built_sources)

BUILT_SOURCES = \
	$(xfce4_lightdash_built_sources)

endif

desktopdir =								\
	$(datadir)/xfce4/panel/plugins

desktop_DATA =								\
	lightdash.desktop

@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST =
	lightdash.desktop.in \
	lightdash-preferences.glade

CLEANFILES =								\
	$(desktop_DATA)