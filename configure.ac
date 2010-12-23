dnl Process this file with autoconf to produce a configure script.
dnl Created by Anjuta application wizard.

AC_INIT([GNOME Media Player], [0.1.3], [https://launchpad.net/gnome-media-player], gnome-media-player)
AC_COPYRIGHT([Michael Lamothe <michael.lamothe@gmail.com>])

AM_INIT_AUTOMAKE(AC_PACKAGE_NAME, AC_PACKAGE_VERSION)
AC_CONFIG_HEADERS([config.h])
AM_MAINTAINER_MODE

AC_ISC_POSIX
AC_PROG_CXX
AM_PROG_CC_STDC
AC_HEADER_STDC

dnl ***************************************************************************
dnl Internationalization
dnl ***************************************************************************
GETTEXT_PACKAGE=gnome-media-player
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", [GETTEXT package name])
AM_GLIB_GNU_GETTEXT
IT_PROG_INTLTOOL([0.35.0])

AM_PROG_LIBTOOL

PKG_CHECK_MODULES(GNOME_MEDIA_PLAYER, [
	dbus-1 dbus-glib-1 gtkmm-2.4 >= 2.12 sigc++-2.0 unique-1.0 x11
	libvlc >= 1.0.0
	libxine >= 1.1.16
	gstreamer-base-0.10 gstreamer-interfaces-0.10 gconf-2.0])

AC_OUTPUT([
Makefile
src/Makefile
po/Makefile.in
])
