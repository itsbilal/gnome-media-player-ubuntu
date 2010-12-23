/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * Copyright (C) Michael Lamothe 2010 <michael.lamothe@gmail.com>
 * 
 * gnome-media-player is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * gnome-media-player is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __GNOME_MEDIA_PLAYER_H__
#define __GNOME_MEDIA_PLAYER_H__

#include "../config.h"
#include <glibmm/i18n.h>
#include <gtkmm.h>
#include <gconf/gconf-client.h>
#include <vector>
#include <list>

typedef Glib::ustring String;
typedef std::list<String> StringList;
typedef std::vector<String> StringArray;

extern Glib::RefPtr<Gtk::Builder>		builder;
extern Glib::RefPtr<Gtk::UIManager>		ui_manager;
extern Glib::RefPtr<Gtk::ActionGroup>	action_group;

extern Glib::RefPtr<Gtk::Action>		action_about;
extern Glib::RefPtr<Gtk::ToggleAction>	action_controls;
extern Glib::RefPtr<Gtk::ToggleAction>	action_fullscreen;
extern Glib::RefPtr<Gtk::Action>		action_backward;
extern Glib::RefPtr<Gtk::Action>		action_forward;
extern Glib::RefPtr<Gtk::Action>		action_next;
extern Glib::RefPtr<Gtk::Action>		action_open;
extern Glib::RefPtr<Gtk::Action>		action_play;
extern Glib::RefPtr<Gtk::Action>		action_stop;
extern Glib::RefPtr<Gtk::ToggleAction>	action_pause;
extern Glib::RefPtr<Gtk::ToggleAction>	action_playlist;
extern Glib::RefPtr<Gtk::Action>		action_playlist_remove;
extern Glib::RefPtr<Gtk::Action>		action_previous;
extern Glib::RefPtr<Gtk::Action>		action_quit;
extern Glib::RefPtr<Gtk::Action>		action_stop;
extern Glib::RefPtr<Gtk::ToggleAction>	action_deinterlace;
extern Glib::RefPtr<Gtk::RadioAction>	action_engine_auto;
extern Glib::RefPtr<Gtk::RadioAction>	action_engine_vlc;
extern Glib::RefPtr<Gtk::RadioAction>	action_engine_xine;
extern Glib::RefPtr<Gtk::RadioAction>	action_engine_gstreamer;
extern Glib::RefPtr<Gtk::Action>		action_restart_engine;
extern Glib::RefPtr<Gtk::ToggleAction>	action_show_drawable;

extern bool	next;
extern bool	deinterlace;
extern bool hide_volume_controls;
extern bool on_top;
extern GConfClient*	gconf_client;

#endif
