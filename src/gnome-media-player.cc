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

#include "gnome-media-player.h"
#include "exception.h"
#include "main_window.h"
#include "config.h"
#include <unique/unique.h>
#include <gconf/gconf-client.h>
#include <gconf/gconf-value.h>
#include <gst/gst.h>

#ifdef ENABLE_NLS
#  include <libintl.h>
#  define _(String) gettext (String)
#endif

#define UI_FILE PACKAGE_DATA_DIR"/gnome-media-player/ui/gnome-media-player.ui"
#define WINDOWICON PACKAGE_DATA_DIR"/gnome-media-player/icons/gnome-media-player-small.png"

Glib::RefPtr<Gtk::Builder>		builder;
Glib::RefPtr<Gtk::UIManager>	ui_manager;
Glib::RefPtr<Gtk::ActionGroup>	action_group;

Glib::RefPtr<Gtk::Action>		action_about;
Glib::RefPtr<Gtk::ToggleAction>	action_controls;
Glib::RefPtr<Gtk::ToggleAction>	action_fullscreen;
Glib::RefPtr<Gtk::Action>		action_backward;
Glib::RefPtr<Gtk::RadioAction>	action_engine_auto;
Glib::RefPtr<Gtk::RadioAction>	action_engine_vlc;
Glib::RefPtr<Gtk::RadioAction>	action_engine_xine;
Glib::RefPtr<Gtk::RadioAction>	action_engine_gstreamer;
Glib::RefPtr<Gtk::Action>		action_forward;
Glib::RefPtr<Gtk::Action>		action_next;
Glib::RefPtr<Gtk::Action>		action_open;
Glib::RefPtr<Gtk::ToggleAction>	action_pause;
Glib::RefPtr<Gtk::Action>		action_play;
Glib::RefPtr<Gtk::ToggleAction>	action_playlist;
Glib::RefPtr<Gtk::Action>		action_playlist_remove;
Glib::RefPtr<Gtk::Action>		action_previous;
Glib::RefPtr<Gtk::Action>		action_quit;
Glib::RefPtr<Gtk::Action>		action_restart_engine;
Glib::RefPtr<Gtk::Action>		action_stop;
Glib::RefPtr<Gtk::ToggleAction>	action_deinterlace;
Glib::RefPtr<Gtk::ToggleAction>	action_show_drawable;

Glib::ustring	engine_type;
bool			hide_volume_controls = false;
bool			next = true;
bool			deinterlace = false;
MainWindow*		main_window = NULL;
bool			gtk_initialised = false;
bool			on_top = false;
GConfClient* 	gconf_client;

enum
{
	COMMAND_0,
	COMMAND_PLAY,
	COMMAND_QUEUE
};

static UniqueResponse on_message_received (
	UniqueApp*			app,
	UniqueCommand		command,
	UniqueMessageData*  message,
	guint          		time_,
	gpointer       		user_data)
{
	UniqueResponse response = UNIQUE_RESPONSE_FAIL;

	switch (command)
    {
		case COMMAND_PLAY:
		{
			gchar* filename = unique_message_data_get_text(message);
			g_debug("Got PLAY(%s) command", filename);
			main_window->clear_playlist();
			main_window->add_to_playlist(filename);
			g_free(filename);
			action_stop->activate();
			action_play->activate();
			response = UNIQUE_RESPONSE_OK;
		}
		break;
			
		case COMMAND_QUEUE:
		{
			gchar* filename = unique_message_data_get_text(message);
			g_debug("Got QUEUE(%s) command", filename);
			main_window->add_to_playlist(filename);
			g_free(filename);
			response = UNIQUE_RESPONSE_OK;
		}
		break;

		default:
			break;
	}

	return response;
}

void handle_error_message(const Glib::ustring& message)
{
	g_message("Error: '%s'", message.c_str());

	if (gtk_initialised)
	{
		if (main_window != NULL)
		{
			Gtk::MessageDialog dialog(*main_window, message, false, Gtk::MESSAGE_ERROR);
			dialog.set_position(Gtk::WIN_POS_CENTER_ON_PARENT);
			dialog.set_title(PACKAGE_NAME);
			dialog.run();
		}
		else
		{
			Gtk::MessageDialog dialog(message, false, Gtk::MESSAGE_ERROR);
			dialog.set_title(PACKAGE_NAME);
			dialog.run();
		}
	}
}

void on_error()
{
	try
	{
		throw;
	}
	catch (const Exception& exception)
	{
		handle_error_message(exception.what());
	}
	catch (const Glib::Error& exception)
	{
		handle_error_message(exception.what());
	}
	catch (...)
	{
		handle_error_message("Unhandled exception");
	}
}

int main (int argc, char *argv[])
{
	StringArray args;

	try
	{
		if (!XInitThreads())
		{
			throw Exception("XInitThreads() failed");
		}
		setlocale(LC_ALL, "");
		setlocale(LC_MESSAGES, "");
  		bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
  		textdomain (GETTEXT_PACKAGE);
		
		gconf_client = gconf_client_get_default();
		if (gconf_client_get_bool(gconf_client,(gchar*)"/apps/gnome-media-player/hide-volume-controls",NULL))
		{
			hide_volume_controls = true;
		}
		if (gconf_client_get_bool(gconf_client,(gchar*)"/apps/gnome-media-player/on-top",NULL))
		{
			on_top = true;
		}
		
		sigc::connection connection_on_error_handler = Glib::add_exception_handler(&on_error);
		Glib::OptionGroup option_group(PACKAGE_NAME, "GNOME Media Player options", _("Show GNOME Media Player help options"));

		Glib::OptionEntry on_top_option_entry;
		on_top_option_entry.set_description(_("Set the window to always remain on top of other windows"));
		on_top_option_entry.set_long_name("on-top");
		option_group.add_entry(on_top_option_entry, on_top);

		Glib::OptionEntry hide_volume_controls_option_entry;
		hide_volume_controls_option_entry.set_description(_("Hide the volume controls from the user interface"));
		hide_volume_controls_option_entry.set_long_name("hide-volume-controls");
		option_group.add_entry(hide_volume_controls_option_entry, hide_volume_controls);

		Glib::OptionEntry engine_type_option_entry;
		engine_type_option_entry.set_description(_("Force engine to use \"auto\", \"vlc\", \"xine\" or \"gstreamer\" (default: auto)"));
		engine_type_option_entry.set_short_name('e');
		engine_type_option_entry.set_long_name("engine-type");
		engine_type_option_entry.set_arg_description("ENGINE");
		option_group.add_entry(engine_type_option_entry, engine_type);

		Glib::OptionEntry deinterlace_type_option_entry;
		deinterlace_type_option_entry.set_description(_("Enable deinterlace"));
		deinterlace_type_option_entry.set_short_name('d');
		deinterlace_type_option_entry.set_long_name("deinterlace");
		option_group.add_entry(deinterlace_type_option_entry, deinterlace);

		Glib::OptionEntry remaining_option_entry;
		remaining_option_entry.set_long_name(G_OPTION_REMAINING);
		option_group.add_entry(remaining_option_entry, args);

		Glib::OptionContext option_context("[FILES...]");
		option_context.set_description("A simple media player for GNOME");
		option_context.set_help_enabled(true);
		option_context.set_ignore_unknown_options(false);
		option_context.set_main_group(option_group);

		// Do this so we can show a Gtk::MessageDialog on error
		gtk_initialised = gtk_init_check(&argc, &argv);

		Gtk::Main main(argc, argv, option_context);
		gst_init(&argc, &argv);

		UniqueApp* unique_application = unique_app_new_with_commands(
		    "org.lamothe.gnome-media-player", NULL,
		    "play", COMMAND_PLAY,
		    "queue", COMMAND_QUEUE,
		    NULL);

		if (unique_app_is_running(unique_application))
		{
			g_debug("GNOME Media Player is already running");

			guint size = args.size();
			for (guint i = 0; i < size; i++)
			{
				UniqueCommand command = (i == 0) ? (UniqueCommand)COMMAND_PLAY : (UniqueCommand)COMMAND_QUEUE;
				String command_name = (i == 0) ? "PLAY" : "QUEUE";
				UniqueMessageData* message = unique_message_data_new();
			
				unique_message_data_set_text(message, args[i].c_str(), args[i].size());

				g_debug("Sending %s(%s) command", command_name.c_str(), args[i].c_str());
			
				UniqueResponse response = unique_app_send_message (unique_application, command, message);

				if (response == UNIQUE_RESPONSE_OK)
				{
					g_debug("%s command sent", command_name.c_str());
				}
				else
				{
					g_debug("%s command failed", command_name.c_str());
				}
				
				unique_message_data_free(message);
			}
		}
		else
		{
			builder = Gtk::Builder::create_from_file(UI_FILE);
			ui_manager = Gtk::UIManager::create();

			Gtk::RadioButtonGroup	engine_group;

			action_about			= Gtk::Action::create("About", Gtk::Stock::ABOUT);
			action_controls			= Gtk::ToggleAction::create("ToggleControls", _("Show Controls"), _("Show/hide video controls"));
			action_backward			= Gtk::Action::create("Backward", Gtk::Stock::MEDIA_REWIND);
			action_deinterlace		= Gtk::ToggleAction::create("ToggleDeinterlace", _("Deinterlace"), _("Enable/Disable deinterlacer"));
			action_engine_auto		= Gtk::RadioAction::create(engine_group, "EngineAuto", _("Auto Select"));
			action_engine_vlc		= Gtk::RadioAction::create(engine_group, "EngineVLC", _("VLC Engine"));
			action_engine_xine		= Gtk::RadioAction::create(engine_group, "EngineXine", _("Xine Engine"));
			action_engine_gstreamer	= Gtk::RadioAction::create(engine_group, "EngineGStreamer", _("GStreamer Engine"));
			action_forward			= Gtk::Action::create("Forward", Gtk::Stock::MEDIA_FORWARD);
			action_fullscreen		= Gtk::ToggleAction::create("ToggleFullscreen", Gtk::Stock::FULLSCREEN);
			action_next				= Gtk::Action::create("Next", Gtk::Stock::MEDIA_NEXT);
			action_open				= Gtk::Action::create("Open", Gtk::Stock::OPEN);
			action_pause			= Gtk::ToggleAction::create("TogglePause", Gtk::Stock::MEDIA_PAUSE);
			action_play				= Gtk::Action::create("Play", Gtk::Stock::MEDIA_PLAY);
			action_playlist			= Gtk::ToggleAction::create("TogglePlaylist", _("Playlist"), _("Show/hide playlist"));
			action_playlist_remove  = Gtk::Action::create("PlaylistRemove", Gtk::Stock::REMOVE);
			action_previous			= Gtk::Action::create("Previous", Gtk::Stock::MEDIA_PREVIOUS);
			action_quit				= Gtk::Action::create("Quit", Gtk::Stock::QUIT);
			action_restart_engine	= Gtk::Action::create("RestartEngine", Gtk::Stock::REFRESH, _("_Restart Engine"));
			action_stop				= Gtk::Action::create("Stop", Gtk::Stock::MEDIA_STOP);
			action_show_drawable	= Gtk::ToggleAction::create("ToggleDrawable", _("Show Video"), _("Show/hide video widget"), true);

			action_group			= Gtk::ActionGroup::create();
			action_group->add(Gtk::Action::create("File", _("_File"), _("_File")));
			action_group->add(Gtk::Action::create("View", _("_View")));
			action_group->add(Gtk::Action::create("Engine", _("_Engine")));
			action_group->add(Gtk::Action::create("Help", _("_Help")));
			action_group->add(action_about, Gtk::AccelKey("F1"));
			action_group->add(action_backward, Gtk::AccelKey("<control>Left"));
			action_group->add(action_controls, Gtk::AccelKey("c"));
			action_group->add(action_deinterlace, Gtk::AccelKey("d"));
			action_group->add(action_engine_auto, Gtk::AccelKey("<control>0"));
			action_group->add(action_engine_vlc, Gtk::AccelKey("<control>1"));
			action_group->add(action_engine_xine, Gtk::AccelKey("<control>2"));
			action_group->add(action_engine_gstreamer, Gtk::AccelKey("<control>3"));
			action_group->add(action_fullscreen, Gtk::AccelKey("f"));
			action_group->add(action_forward, Gtk::AccelKey("<control>Right"));
			action_group->add(action_next, Gtk::AccelKey("<control>Down"));
			action_group->add(action_open);
			action_group->add(action_pause, Gtk::AccelKey("space"));
			action_group->add(action_play);
			action_group->add(action_playlist, Gtk::AccelKey("p"));
			action_group->add(action_playlist_remove, Gtk::AccelKey("<del>"));
			action_group->add(action_previous, Gtk::AccelKey("<control>Up"));
			action_group->add(action_quit);
			action_group->add(action_stop);
			action_group->add(action_restart_engine, Gtk::AccelKey("F5"));
			action_group->add(action_show_drawable);

			action_controls->set_active(true);
			builder->get_widget_derived("main_window", main_window);
			action_quit->signal_activate().connect(sigc::ptr_fun(Gtk::Main::quit));
			
			main_window->show();
			
			if (engine_type == "vlc")
			{
				action_engine_vlc->activate();
			}
			else if (engine_type == "gstreamer")
			{
				action_engine_gstreamer->activate();
			}
			else if (engine_type == "xine")
			{
				action_engine_xine->activate();
			}
			else if (engine_type == "auto")
			{
				action_engine_auto->activate();
			}
			else if (engine_type.empty())
			{
				if (gconf_client_get_string(gconf_client,(gchar*)"/apps/gnome-media-player/engine",NULL))
				{
					if (!strcmp(gconf_client_get_string(gconf_client,(gchar*)"/apps/gnome-media-player/engine",NULL), "auto"))
					{
						action_engine_auto->activate();
					}
					else if (!strcmp(gconf_client_get_string(gconf_client,(gchar*)"/apps/gnome-media-player/engine",NULL), "vlc"))
					{
						action_engine_vlc->activate();
					}
					else if (!strcmp(gconf_client_get_string(gconf_client,(gchar*)"/apps/gnome-media-player/engine",NULL), "gstreamer"))
					{
						action_engine_gstreamer->activate();
					}
					else if (!strcmp(gconf_client_get_string(gconf_client,(gchar*)"/apps/gnome-media-player/engine",NULL), "xine"))
					{
						action_engine_xine->activate();
					}
					else
					{
						action_engine_auto->activate();
					}
				}
				else
				{
					action_engine_auto->activate();
				}
			}
			else
			{
				throw Exception(_("Unknown engine type"));
			}

			if (deinterlace)
			{
				action_deinterlace->activate();
			}

			for (std::vector<Glib::ustring>::iterator i = args.begin(); i != args.end(); i++)
			{
				main_window->add_to_playlist(*i);
			}

			unique_app_watch_window(unique_application, GTK_WINDOW(main_window->gobj()));
			g_signal_connect(unique_application, "message-received", G_CALLBACK (on_message_received), NULL);

			action_pause->set_active(true);
			action_play->activate();
			
			main_window->set_icon_from_file(WINDOWICON);
			
			main.run(*main_window);
		}
	}
	catch (const Exception& exception)
	{
		handle_error_message(exception.what());
	}
	catch (const Glib::Error& exception)
	{
		handle_error_message(exception.what());
	}
	catch (...)
	{
		handle_error_message("Unhandled exception");
	}
	
	return 0;
}
