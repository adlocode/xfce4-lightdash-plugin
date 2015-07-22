
appsdir = $(datadir)/applications
apps_in_files = \
	lightdash.desktop.in \
	xfce4-lightdash.desktop.in
	
apps_DATA = $(apps_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

#
# .appdata
#
@INTLTOOL_XML_RULE@
appdatadir = $(datadir)/appdata
appdata_DATA = $(appdata_in_files:.xml.in=.xml)
appdata_in_files = xfce4-lightdash.appdata.xml.in

EXTRA_DIST = \
	$(apps_in_files)	\
	$(lightdash_in_files)

CLEANFILES = $(lightdash_DATA)

DISTCLEANFILES = \
	$(apps_DATA)

# vi:set ts=8 sw=8 noet ai nocindent:

