## Process this file with automake to produce Makefile.in
## Created by Anjuta

SUBDIRS = src po

PACKAGE = gnome-media-player

desktopdir = $(datadir)/applications
desktop_in_files = gnome-media-player.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

pixmapdir = $(datadir)/pixmaps
pixmap_DATA = gnome-media-player-large.png

icondir = $(datadir)/gnome-media-player/icons
icon_DATA = gnome-media-player-large.png gnome-media-player-small.png

man1_MANS = gnome-media-player.1

gnome_media_playerdocdir = ${datadir}/doc/gnome-media-player
gnome_media_playerdoc_DATA = \
	README\
	COPYING\
	AUTHORS\
	ChangeLog\
	INSTALL\
	NEWS

INTLTOOL_FILES = intltool-extract.in \
	intltool-merge.in \
	intltool-update.in

EXTRA_DIST = $(gnome_media_playerdoc_DATA) \
	$(INTLTOOL_FILES) \
	$(desktop_in_files) \
	gnome-media-player.1 \
	gnome-media-player-small.png \
	gnome-media-player-large.png

DISTCLEANFILES = intltool-extract \
	intltool-merge \
	intltool-update \
	po/.intltool-merge-cache

CLEANFILES = $(desktop_DATA)

# Remove doc directory on uninstall
uninstall-local:
	-rm -r $(gnome_media_playerdocdir)
