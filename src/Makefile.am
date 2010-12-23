## Process this file with automake to produce Makefile.in

## Created by Anjuta

uidir = $(datadir)/gnome-media-player/ui
ui_DATA = gnome-media-player.ui

AM_CPPFLAGS = \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	-DPACKAGE_SRC_DIR=\""$(srcdir)"\" \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	$(GNOME_MEDIA_PLAYER_CFLAGS)

AM_CFLAGS =\
	 -Wall

bin_PROGRAMS = gnome-media-player

gnome_media_player_SOURCES = \
	main_window.cc    \
	main_window.h    \
	engine.cc    \
	engine.h    \
	exception.cc    \
	exception.h    \
	gnome-media-player.cc    \
	gnome-media-player.h    \
	xine_engine.cc    \
	xine_engine.h    \
	gstreamer_engine.cc    \
	gstreamer_engine.h    \
	vlc_engine.cc    \
	vlc_engine.h

gnome_media_player_LDFLAGS = 

gnome_media_player_LDADD = $(GNOME_MEDIA_PLAYER_LIBS)

EXTRA_DIST = $(ui_DATA)


