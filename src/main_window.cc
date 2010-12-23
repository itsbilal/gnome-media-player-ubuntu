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

#include "main_window.h"
#include "exception.h"
#include "gnome-media-player.h"
#include <gdk/gdkx.h>
#include <iomanip>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <gconf/gconf-client.h>
#include <gconf/gconf-value.h>
#include "xine_engine.h"
#include "vlc_engine.h"
#include "gstreamer_engine.h"
#include <stdlib.h>
#include <unistd.h>

#define GS_SERVICE   "org.gnome.ScreenSaver"
#define GS_PATH      "/org/gnome/ScreenSaver"
#define GS_INTERFACE "org.gnome.ScreenSaver"
#define TS_PACKET_SIZE  188
#define WINDOWICON PACKAGE_DATA_DIR"/gnome-media-player/icons/gnome-media-player-small.png"
#define LOGO PACKAGE_DATA_DIR"/gnome-media-player/icons/gnome-media-player-large.png"

Gtk::Image* add_control_button(Gtk::HBox* hbox, Glib::RefPtr<Gtk::Action> action)
{
	Gtk::Image* image = action->create_icon(Gtk::ICON_SIZE_BUTTON);
	Gtk::Button* button = new Gtk::Button();
	button->add(*image);
	button->set_relief(Gtk::RELIEF_NONE);
	button->set_can_focus(false);
	hbox->pack_start(*button, false, false);
	action->connect_proxy(*button);
	button->show();
	return image;
}

MainWindow::MainWindow(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder)
	: Gtk::Window(cobject)
{	
	is_cursor_visible		= true;
	last_motion_time		= 0;
	timeout_updating_slider = false;
	engine					= NULL;
	next					= false;
	dbus_connection			= NULL;
	cookie					= 0;
	Gtk::HBox* hbox_playlist_buttons = NULL;

	builder->get_widget("event_box_video", event_box_video);	
	builder->get_widget("drawing_area_video", drawing_area_video);
	builder->get_widget("scrolled_window_playlist", scrolled_window_playlist);
	builder->get_widget("vbox_playlist", vbox_playlist);
	builder->get_widget("tree_view_playlist", tree_view_playlist);
	builder->get_widget("hbox_controls", hbox_controls);
	builder->get_widget("hbox_playlist_buttons", hbox_playlist_buttons);

	Gtk::Button* button_playlist_remove = new Gtk::Button(Gtk::Stock::REMOVE);
	button_playlist_remove->show();
	action_playlist_remove->connect_proxy(*button_playlist_remove);

	hbox_playlist_buttons->pack_start(*button_playlist_remove, true, true);

	image_play = add_control_button(hbox_controls, Glib::RefPtr<Gtk::Action>::cast_dynamic(action_pause));
	add_control_button(hbox_controls, action_previous);
	add_control_button(hbox_controls, action_next);

	hbox_controls->pack_start(*(new Gtk::Label(_("Time:"))), false, false);

	hscale_video_position = new Gtk::HScale(0.0, 1.0, 0.01);
	hscale_video_position->set_draw_value(false);
	hscale_video_position->set_can_focus(false);
	hscale_video_position->set_sensitive(false);
	hbox_controls->pack_start(*hscale_video_position, true, true);

	label_video_time = new Gtk::Label("0:00/0:00");
	hbox_controls->pack_start(*label_video_time, false, false);

	volume_button = new Gtk::VolumeButton();
	volume_button->signal_value_changed().connect(sigc::mem_fun(*this, &MainWindow::on_volume_changed));
	volume_button->set_sensitive(false);
	volume_button->set_value(1);

	toggle_button_playlist = new Gtk::ToggleButton("Playlist");
	toggle_button_playlist->set_can_focus(false);
	action_playlist->connect_proxy(*toggle_button_playlist);

	if (!hide_volume_controls)
	{
		hbox_controls->pack_start(*volume_button, false, false);
	}
	hbox_controls->pack_start(*toggle_button_playlist, false, false);
	hbox_controls->show_all();

	std::list<Gtk::TargetEntry> listTargets;
	listTargets.push_back(Gtk::TargetEntry("STRING"));

	event_box_video->drag_dest_set(listTargets);
	toggle_button_playlist->drag_dest_set(listTargets);
	scrolled_window_playlist->drag_dest_set(listTargets);

	event_box_video->signal_scroll_event().connect(sigc::mem_fun(*this, &MainWindow::on_scroll));
	event_box_video->signal_button_press_event().connect(sigc::mem_fun(*this, &MainWindow::on_event_box_video_button_press_event));
	event_box_video->signal_drag_data_received().connect(sigc::mem_fun(*this, &MainWindow::on_event_box_video_drag_data_received));
	event_box_video->signal_motion_notify_event().connect(sigc::mem_fun(*this, &MainWindow::on_event_box_video_motion_notify_event));

	hscale_video_position->signal_change_value().connect(sigc::mem_fun(*this, &MainWindow::on_change_value));

	toggle_button_playlist->signal_drag_data_received().connect(sigc::mem_fun(*this, &MainWindow::on_playlist_drag_data_received));
	tree_view_playlist->signal_row_activated().connect(sigc::mem_fun(*this, &MainWindow::on_playlist_row_activated));
	scrolled_window_playlist->signal_drag_data_received().connect(sigc::mem_fun(*this, &MainWindow::on_playlist_drag_data_received));
	hscale_video_position->signal_value_changed().connect(sigc::mem_fun(*this, &MainWindow::on_video_position_value_changed));
	hscale_video_position->signal_scroll_event().connect(sigc::mem_fun(*this, &MainWindow::on_scroll));
	drawing_area_video->set_double_buffered(false);
	drawing_area_video->signal_expose_event().connect(sigc::mem_fun(*this, &MainWindow::on_drawing_area_expose_event));

	event_box_video->modify_fg(Gtk::STATE_NORMAL, Gdk::Color("black"));
	event_box_video->modify_bg(Gtk::STATE_NORMAL, Gdk::Color("black"));
	drawing_area_video->modify_fg(Gtk::STATE_NORMAL, Gdk::Color("black"));
	drawing_area_video->modify_bg(Gtk::STATE_NORMAL, Gdk::Color("black"));

	list_store = Gtk::ListStore::create(columns);
	tree_view_playlist->set_model(list_store);
	tree_view_playlist->append_column(_(" "), columns.column_image);
	tree_view_playlist->append_column(_("Description"), columns.column_description);
	tree_view_playlist->set_headers_visible();
	tree_view_playlist->set_reorderable(true);	
	tree_view_playlist->get_selection()->set_mode(Gtk::SELECTION_MULTIPLE);

	pixbuf_play = Gtk::Widget::render_icon(Gtk::Stock::MEDIA_PLAY, Gtk::ICON_SIZE_MENU);

	gchar     bits[] = {0};
	GdkColor  color = {0, 0, 0, 0};
	GdkPixmap* pixmap = gdk_bitmap_create_from_data(NULL, bits, 1, 1);
	hidden_cursor = gdk_cursor_new_from_pixmap(pixmap, pixmap, &color, &color, 0, 0);
	
	if (on_top)
	{
		set_keep_above(true);
	}

	action_about->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::on_about));
	action_backward->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::on_backward));
	action_controls->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::on_controls));
	action_deinterlace->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::on_deinterlace));
	action_engine_auto->signal_changed().connect(sigc::mem_fun(*this, &MainWindow::on_engine_changed));
	action_forward->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::on_forward));
	action_fullscreen->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::on_fullscreen));
	action_next->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::on_next));
	action_open->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::on_open));
	action_play->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::on_play));
	action_pause->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::on_pause));
	action_playlist->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::on_playlist));
	action_playlist_remove->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::on_playlist_remove));
	action_previous->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::on_previous));
	action_restart_engine->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::on_restart_engine));
	action_stop->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::on_stop));
	action_show_drawable->signal_activate().connect(sigc::mem_fun(*this, &MainWindow::on_show_drawable));

	ui_manager->insert_action_group(action_group);
	add_accel_group(ui_manager->get_accel_group());

	Glib::ustring ui_info =
		"<ui>"
		"  <menubar name='menu_bar'>"
		"    <menu action='File'>"
		"      <menuitem action='Open'/>"
		"      <separator/>"
		"      <menuitem action='Quit'/>"
		"    </menu>"
		"    <menu action='View'>"
		"      <menuitem action='Play'/>"
		"      <menuitem action='Stop'/>"
		"      <menuitem action='TogglePause'/>"
		"      <separator/>"
		"      <menuitem action='Forward'/>"
		"      <menuitem action='Backward'/>"
		"      <separator/>"
		"      <menuitem action='Next'/>"
		"      <menuitem action='Previous'/>"
		"      <separator/>"
		"      <menuitem action='ToggleFullscreen'/>"
		"      <menuitem action='ToggleDeinterlace'/>"
		"      <menuitem action='ToggleControls'/>"
		"	 <menuitem action='ToggleDrawable' />"
		"      <menuitem action='TogglePlaylist'/>"
		"    </menu>"
		"    <menu action='Engine'>"
		"      <menuitem action='EngineAuto'/>"
		"      <menuitem action='EngineVLC'/>"
		"      <menuitem action='EngineXine'/>"
		"      <menuitem action='EngineGStreamer'/>"
		"      <separator/>"
		"      <menuitem action='RestartEngine'/>"
		"    </menu>"
		"    <menu action='Help'>"
		"      <menuitem action='About'/>"
		"    </menu>"
		"  </menubar>"
		"  <popup name='menu_video'>"
		"    <menuitem action='ToggleFullscreen'/>"
		"    <menuitem action='ToggleDeinterlace'/>"
		"    <menuitem action='ToggleControls'/>"
		"	 <menuitem action='ToggleDrawable' />"
		"    <menuitem action='TogglePlaylist'/>"
		"    <separator/>"
		"    <menuitem action='EngineAuto'/>"
		"    <menuitem action='EngineVLC'/>"
		"    <menuitem action='EngineXine'/>"
		"    <menuitem action='EngineGStreamer'/>"
		"  </popup>"
		"</ui>";

	ui_manager->add_ui_from_string(ui_info);

	Gtk::HBox* hbox_menu = NULL;
	builder->get_widget("hbox_menu", hbox_menu);
	menu_bar = (Gtk::MenuBar*)ui_manager->get_widget("/menu_bar");
	hbox_menu->pack_start(*menu_bar, true, true);
	menu_video = (Gtk::Menu*)ui_manager->get_widget("/menu_video");

	dbus_error_init(&dbus_error);
	dbus_connection = dbus_bus_get(DBUS_BUS_SESSION, &dbus_error);
	if (!dbus_connection)
	{
		Glib::ustring message = Glib::ustring::compose("Failed to connect to the D-BUS daemon: %1", dbus_error.message);
		dbus_error_free(&dbus_error);

		throw Exception(message);
	}

	dbus_connection_setup_with_g_main(dbus_connection, NULL);
	
	image_play->set(Gtk::Stock::MEDIA_PAUSE, Gtk::ICON_SIZE_MENU);
	vbox_playlist->hide();
	on_engine_changed(action_engine_auto);
	last_motion_time = time(NULL);
	timeout_source = gdk_threads_add_timeout(100, &MainWindow::on_timeout, this);
}

gboolean MainWindow::on_timeout(gpointer data)
{
	MainWindow* main_window = (MainWindow*)data;
	main_window->on_timeout();
	return true;
}

void MainWindow::on_timeout()
{
	guint now = time(NULL);

	if (now - last_motion_time > 3)
	{
		if (action_fullscreen->get_active() || !action_controls->get_active())
		{
			menu_bar->set_visible(false);
			hbox_controls->set_visible(false);
		}

		if (is_cursor_visible)
		{
			Glib::RefPtr<Gdk::Window> event_box_video_window = event_box_video->get_window();
			if (event_box_video_window)
			{
				event_box_video_window->set_cursor(Gdk::Cursor(hidden_cursor));
				is_cursor_visible = false;
			}
		}
	}

	if (engine != NULL)
	{
		int length = engine->get_length();
		int	time = engine->get_time();

		timeout_updating_slider = true;
		if (length == 0)
		{
			hscale_video_position->set_range(0, 1);
			hscale_video_position->set_value(engine->get_percentage());
		}
		else
		{
			hscale_video_position->set_range(0, length);
			hscale_video_position->set_value(time);
		}
		timeout_updating_slider = false;
	
		label_video_time->set_text(engine->get_text());
	}

	if (next)
	{
		action_next->activate();
		next = false;
	}
}

void MainWindow::clear_playlist()
{
	list_store->clear();
}

bool MainWindow::on_drawing_area_expose_event(GdkEventExpose* event_expose)
{
	if (drawing_area_video != NULL && drawing_area_video->is_realized())
	{
		Glib::RefPtr<Gtk::Style> style = drawing_area_video->get_style();

		if (!style)
		{
			return true;
		}
		
		drawing_area_video->get_window()->draw_rectangle(
			style->get_bg_gc(Gtk::STATE_NORMAL), true,
			event_expose->area.x, event_expose->area.y,
			event_expose->area.width, event_expose->area.height);

		if (engine != NULL)
		{
			engine->on_expose(event_expose);
		}
	}

	return false;
}

bool MainWindow::on_scroll(GdkEventScroll* event_scroll)
{
	event_scroll->direction == GDK_SCROLL_DOWN ?
		action_backward->activate() : action_forward->activate();
	usleep(10000);
	return false;
}

bool MainWindow::on_change_value(Gtk::ScrollType scroll_type, double new_value)
{
	if (engine != NULL)
	{
		switch(scroll_type)
		{
			case Gtk::SCROLL_PAGE_BACKWARD:
			case Gtk::SCROLL_PAGE_FORWARD:
				{
					int x = 0, y = 0;
					hscale_video_position->get_pointer(x, y);
					float percentage = (x / (float)hscale_video_position->get_width());

					int length = engine->get_length();
					if (length == 0)
					{
						engine->set_percentage(percentage);
					}
					else
					{
						engine->set_time(length * percentage);
					}
				}
				break;
				
			default:
				break;
		}
	}
		
	return false;
}

void MainWindow::on_video_position_value_changed()
{
	if (!timeout_updating_slider && engine != NULL)
	{
		int value = hscale_video_position->get_value();
		float upper = hscale_video_position->get_adjustment()->get_upper();
		float percentage = value / upper;
		engine->set_percentage(percentage);
	}
}

void MainWindow::inhibit(gboolean activate)
{
	DBusMessage*	message = NULL;
	DBusMessageIter iter;

	if (dbus_connection == NULL)
	{
		return;
	}
		
	if (activate)
	{
		DBusMessage*	reply = NULL;
		
		message = dbus_message_new_method_call(GS_SERVICE, GS_PATH, GS_INTERFACE, "Inhibit");
		if (message == NULL)
		{
			g_warning ("Failed to create dbus message");
			return;
		}

		const char* application = PACKAGE_NAME;
		const char* reason = "Playing video";

		dbus_message_iter_init_append (message, &iter);
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &application);
		dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &reason);

		reply = dbus_connection_send_with_reply_and_block (dbus_connection, message, -1, &dbus_error);
		if (dbus_error_is_set (&dbus_error))
		{
			g_warning ("%s raised:\n %s\n", dbus_error.name, dbus_error.message);
			reply = NULL;
			dbus_error_free(&dbus_error);
		}

		if (reply != NULL)
		{
			dbus_message_iter_init (reply, &iter);
			dbus_message_iter_get_basic (&iter, &cookie);

			dbus_message_unref (message);
			dbus_message_unref (reply);

			g_debug("Got Cookie: %d", cookie);
			g_debug("Screensaver inhibited");
		}
	}
	else
	{
		if (cookie != 0)
		{
			message = dbus_message_new_method_call(GS_SERVICE, GS_PATH, GS_INTERFACE, "UnInhibit");
			if (message == NULL)
			{
				g_warning ("Couldn't allocate the dbus message");
				return;
			}
			
			g_debug("Using Cookie: %d", cookie);
			dbus_message_iter_init_append (message, &iter);
			dbus_message_iter_append_basic (&iter, DBUS_TYPE_INT32, &cookie);
			dbus_connection_send (dbus_connection, message, NULL);
			cookie = 0;

			g_debug("Screensaver uninhibited");
		}
	}
}

bool MainWindow::on_event_box_video_button_press_event(GdkEventButton* event_button)
{
	if (event_button->button == 1)
	{
		if (event_button->type == GDK_2BUTTON_PRESS)
		{
			action_fullscreen->activate();
		}
	}
	else if (event_button->button == 2)
	{
		action_pause->set_active(!action_pause->get_active());
	}
	else if (event_button->button == 3)
	{
		menu_video->popup(event_button->button, event_button->time);
	}
	
	return false;
}

bool MainWindow::on_event_box_video_motion_notify_event(GdkEventMotion* event_motion)
{
	last_motion_time = time(NULL);
	if (!is_cursor_visible)
	{
		Glib::RefPtr<Gdk::Window> event_box_video_window = event_box_video->get_window();
		if (event_box_video_window)
		{
			event_box_video_window->set_cursor();
			is_cursor_visible = true;
		}
	}
	menu_bar->set_visible(true);
	hbox_controls->set_visible(true);

	return true;
}

bool MainWindow::on_motion_notify_event(GdkEventMotion* event_motion)
{
	on_event_box_video_motion_notify_event(event_motion);
	return Gtk::Window::on_motion_notify_event(event_motion);
}

void MainWindow::add_to_playlist(const Glib::ustring& uris)
{	
	gchar** parts = g_strsplit(uris.c_str(), "\r\n", 1000);
	add_to_playlist(parts);
	g_strfreev(parts);
}

void MainWindow::add_to_playlist(gchar** uris)
{	
	for (gchar** iterator = uris; *iterator != NULL; iterator++)
	{
		gchar* part = *iterator;
		if (*part != 0)
		{
			Glib::ustring uri = part;

			if (uri[0] == '/')
			{
				uri = "file://" + uri;
			}

			Gtk::TreeModel::Row row = *(list_store->append());
			g_debug("Adding '%s' to playlist", uri.c_str());

			Glib::RefPtr<Gio::File> file = Gio::File::create_for_commandline_arg(uri);
			Glib::RefPtr<Gio::FileInfo> file_info = file->query_info();
			Glib::ustring description = file_info->get_display_name();

			row[columns.column_description] = description;
			row[columns.column_uri]			= uri;
			row[columns.column_image]		= pixbuf_empty;
			row[columns.column_current]		= false;
		}
	}
}

void MainWindow::on_playlist_drag_data_received(const Glib::RefPtr<Gdk::DragContext>& context, int, int,
        const Gtk::SelectionData& selection_data, guint, guint time)
{
	const int length = selection_data.get_length();
	if(length > 0 && selection_data.get_format() == 8)
	{
		add_to_playlist(selection_data.get_data_as_string());
	}
	context->drag_finish(false, false, time);
}

void MainWindow::on_event_box_video_drag_data_received(const Glib::RefPtr<Gdk::DragContext>& context, int x, int y,
        const Gtk::SelectionData& selection_data, guint info, guint time)
{
	clear_playlist();
	const int length = selection_data.get_length();
	if(length > 0 && selection_data.get_format() == 8)
	{
		add_to_playlist(selection_data.get_data_as_string());
	}
	context->drag_finish(false, false, time);
	action_restart_engine->activate();
}

void MainWindow::on_drag_data_received(const Glib::RefPtr<Gdk::DragContext>& context, int x, int y,
        const Gtk::SelectionData& selection_data, guint info, guint time)
{
	on_event_box_video_drag_data_received(context, x, y, selection_data, info, time);
	Gtk::Window::on_drag_data_received(context, x, y, selection_data, info, time);
}

void MainWindow::play(Gtk::TreeModel::Row row)
{
	Gtk::TreeModel::Children children = list_store->children();
	for (Gtk::TreeIter iterator = children.begin(); iterator != children.end(); iterator++)
	{
		Gtk::TreeModel::Row r(*iterator);
		r[columns.column_image] = pixbuf_empty;
		r[columns.column_current] = false;
	}

	row[columns.column_image] = pixbuf_play;
	row[columns.column_current] = true;

	Glib::ustring uri = row[columns.column_uri];
	g_debug("Playing '%s'", uri.c_str());

	if (engine != NULL)
	{
		Engine* temp = engine;
		engine = NULL;
		delete temp;
	}
	
	if (action_engine_auto->get_active())
	{
		gboolean is_mpeg2ts = false;
		gboolean done = false;
		guchar buffer[TS_PACKET_SIZE];
		
		Glib::RefPtr<Gio::FileInputStream> input = Gio::File::create_for_commandline_arg(uri)->read();
		if (input)
		{
			while (!done)
			{
				if (input->read(buffer, TS_PACKET_SIZE) != TS_PACKET_SIZE)
				{
					g_debug("Failed to read %d bytes", TS_PACKET_SIZE);
					done = true;
					break;
				}

				if (buffer[0] != 0x47)
				{
					g_debug("Not an MPEG2 Transport Stream");
					done = true;
					break;
				}

				if (buffer[1] == 0x40 && buffer[2] == 0)
				{
					g_debug("Found Program Association Table, stream is MPEG2 Transport Stream");
					is_mpeg2ts = true;
					done = true;
				}
			}
			input->close();
		}

		if (is_mpeg2ts)
		{
			action_deinterlace->set_active();
			engine = new XineEngine();
		}
		else
		{
			Glib::ustring message;
			gboolean failed = true;

			try
			{
				engine = new VlcEngine();
				failed = false;
			}
			catch (const Exception& exception)
			{
				message = exception.what();
			}
			catch (const Glib::Error& exception)
			{
				message = exception.what();
			}
			catch (...)
			{
				message = "Unhandled exception";
			}

			if (failed)
			{
				g_debug("Failed to create VLC engine: %s", message.c_str());
				engine = new XineEngine();
			}
		}
		GConfValue* gconf_engine_value = gconf_value_new_from_string(GCONF_VALUE_STRING,(gchar*)"auto",NULL);
		gconf_client_set(gconf_client,(gchar*)"/apps/gnome-media-player/engine",gconf_engine_value,NULL);
	}
	else if (action_engine_vlc->get_active())
	{
		engine = new VlcEngine();
		GConfValue* gconf_engine_value = gconf_value_new_from_string(GCONF_VALUE_STRING,(gchar*)"vlc",NULL);
		gconf_client_set(gconf_client,(gchar*)"/apps/gnome-media-player/engine",gconf_engine_value,NULL);
	}
	else if (action_engine_gstreamer->get_active())
	{
		engine = new GStreamerEngine();
		GConfValue* gconf_engine_value = gconf_value_new_from_string(GCONF_VALUE_STRING,(gchar*)"gstreamer",NULL);
		gconf_client_set(gconf_client,(gchar*)"/apps/gnome-media-player/engine",gconf_engine_value,NULL);
	}
	else if (action_engine_xine->get_active())
	{
		engine = new XineEngine();
		GConfValue* gconf_engine_value = gconf_value_new_from_string(GCONF_VALUE_STRING,(gchar*)"xine",NULL);
		gconf_client_set(gconf_client,(gchar*)"/apps/gnome-media-player/engine",gconf_engine_value,NULL);
	}
	else
	{
		throw Exception(_("Unknown engine type"));
	}

	if (engine == NULL)
	{
		throw Exception("Engine has not been created");
	}
	
	engine->set_window(GDK_WINDOW_XID(drawing_area_video->get_window()->gobj()));
	engine->set_mrl(uri);
	g_debug("Engine MRL set to '%s'", uri.c_str());
	inhibit();
	action_pause->set_active(false);
	hscale_video_position->set_sensitive(true);
	engine->set_volume(volume_button->get_value() * 100);
	engine->play();
	volume_button->set_sensitive(true);
}

bool MainWindow::on_key_press_event(GdkEventKey* event)
{
	switch(event->keyval)
	{
		case GDK_Right: action_forward->activate(); return false;
		case GDK_Left: action_backward->activate(); return false;
		case GDK_Escape: action_fullscreen->set_active(false); return false;
		default: break;
	}

	return Gtk::Window::on_key_press_event(event);
}

void MainWindow::on_playlist_row_activated(const Gtk::TreeModel::Path& tree_model_path, Gtk::TreeViewColumn* column)
{
	Glib::RefPtr<Gtk::TreeSelection> selection = tree_view_playlist->get_selection();	
	if (selection->count_selected_rows() > 0)
	{
		std::list<Gtk::TreeModel::Path> selected = selection->get_selected_rows();
		Gtk::TreeModel::Row row(*(list_store->get_iter(*selected.begin())));
		play(row);
	}
}

void MainWindow::on_about()
{
	Gtk::AboutDialog* dialog_about = NULL;
	builder->get_widget("dialog_about", dialog_about);
	dialog_about->set_version(VERSION);
	dialog_about->set_icon_from_file(WINDOWICON);
	Glib::RefPtr<Gdk::Pixbuf> logopixbuf = Gdk::Pixbuf::create_from_file(LOGO);
	dialog_about->set_logo(logopixbuf);
	dialog_about->run();
	dialog_about->hide();
	
}

void MainWindow::on_backward()
{
	if (engine != NULL)
	{
		engine->increment(false);
	}
}

void MainWindow::on_controls()
{
	menu_bar->set_visible(action_controls->get_active());
	hbox_controls->set_visible(action_controls->get_active());
}

void MainWindow::on_deinterlace()
{
	action_restart_engine->activate();
}

void MainWindow::on_engine_changed(const Glib::RefPtr<Gtk::RadioAction>& radio_action)
{
	if (radio_action->get_active())
	{
		Glib::ustring title = Glib::ustring::compose("%1 (%2)", PACKAGE_NAME, radio_action->get_label());
		set_title(title);
		g_debug("Engine: '%s'", radio_action->get_label().c_str());
		action_restart_engine->activate();
	}
}

void MainWindow::on_fullscreen()
{
	if (action_fullscreen->get_active())
	{
		action_controls->set_active(false);
		action_playlist->set_active(false);
		action_show_drawable->set_active(true);
		fullscreen();
	}
	else
	{
		action_controls->set_active();
		resize(500,400);
		unfullscreen();
	}
}

void MainWindow::on_forward()
{
	if (engine != NULL)
	{
		engine->increment(true);
	}
}

void MainWindow::on_next()
{
	Gtk::TreeModel::Children children = list_store->children();
	for (Gtk::TreeIter iterator = children.begin(); iterator != children.end(); iterator++)
	{
		Gtk::TreeModel::Row row(*iterator);
		if (row[columns.column_current])
		{
			iterator++;
			if (iterator == children.end())
			{
				iterator = children.begin();
			}

			Gtk::TreeModel::Row next_row(*iterator);
			play(next_row);
			
			break;
		}
	}
}

void MainWindow::on_open()
{
	GtkWidget* dialog = gtk_file_chooser_dialog_new ("Open File",
		gobj(),
		GTK_FILE_CHOOSER_ACTION_OPEN,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
		NULL);
	
	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
	{
		char* filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
		list_store->clear();
		gtk_widget_destroy (dialog);
		add_to_playlist(filename);
		g_free (filename);
		action_restart_engine->activate();
	}
	else
	{
		gtk_widget_destroy (dialog);
	}
}

void MainWindow::on_pause()
{
	bool active = action_pause->get_active() || engine == NULL;

	image_play->set(
		active ? Gtk::Stock::MEDIA_PLAY : Gtk::Stock::MEDIA_PAUSE,
		Gtk::ICON_SIZE_MENU);

	if (engine != NULL)
	{
		inhibit(!active);
		engine->pause(active);
	}
}

void MainWindow::on_play()
{
	if (engine == NULL)
	{
		Gtk::TreeModel::Children children = list_store->children();
		if (children.size() == 0)
		{
			g_debug("Nothing to play");
		}
		else
		{
			gboolean found = false;

			for (Gtk::TreeIter iterator = children.begin(); iterator != children.end() && found == false; iterator++)
			{
				Gtk::TreeModel::Row row(*iterator);
				Glib::ustring u = row[columns.column_uri];

				if (row[columns.column_current])
				{
					g_debug("Found a current row");
					play(row);
					found = true;
				}
			}

			if (!found)
			{
				g_debug("Selecting first row");
				play(*(children.begin()));
			}
		}
	}
}

void MainWindow::on_playlist()
{
	vbox_playlist->set_visible(action_playlist->get_active());
	if (!action_show_drawable->get_active() && action_playlist->get_active())
	{
		resize(500,400);
	}
	else if (!action_show_drawable->get_active() && !action_playlist->get_active())
	{
		resize(500,60);
	}
}

void MainWindow::on_playlist_remove()
{
	get_window()->freeze_updates();
	Glib::RefPtr<Gtk::TreeSelection> tree_selection = tree_view_playlist->get_selection();
	std::list<Gtk::TreeModel::Path> selected = tree_selection->get_selected_rows();
	while (selected.size() > 0)
	{		
		list_store->erase(list_store->get_iter(*selected.begin()));
		selected = tree_selection->get_selected_rows();
	}
	get_window()->thaw_updates();
}

void MainWindow::on_previous()
{
	Gtk::TreeModel::Children children = list_store->children();
	for (Gtk::TreeIter iterator = children.begin(); iterator != children.end(); iterator++)
	{
		Gtk::TreeModel::Row row(*iterator);
		if (row[columns.column_current])
		{
			if (engine != NULL && engine->get_time() > 5000)
			{
				engine->set_time(0);
			}
			else
			{			
				if (iterator != children.begin())
				{
					iterator--;
					Gtk::TreeModel::Row previous_row(*iterator);
					play(previous_row);
				}
			}
			
			break;
		}
	}
}

void MainWindow::on_stop()
{
	if (engine != NULL)
	{
		delete engine;
		engine = NULL;

		Glib::RefPtr<Gtk::Style> style = drawing_area_video->get_style();

		if (!style)
		{
			return;
		}

		gint width = 0, height = 0;
		drawing_area_video->get_window()->get_size(width, height);
		drawing_area_video->get_window()->draw_rectangle(
			style->get_bg_gc(Gtk::STATE_NORMAL), true,
			0, 0, width, height);

		hscale_video_position->set_range(0, 1);
		hscale_video_position->set_value(0);
		hscale_video_position->set_sensitive(false);
		volume_button->set_sensitive(false);
		label_video_time->set_text("0:00/0:00");
	}
}

void MainWindow::on_restart_engine()
{
	float percentage = 0;

	if (engine != NULL)
	{
		percentage = engine->get_percentage();
		action_stop->activate();
	}
	action_play->activate();
	if (engine != NULL)
	{
		engine->set_percentage(percentage);
	}
}

void MainWindow::on_volume_changed(double value)
{
	if (engine != NULL)
	{
		engine->set_volume(value*100);
	} 
}

void MainWindow::on_show_drawable()
{
	if (!action_show_drawable->get_active() && !action_fullscreen->get_active())
	{
		drawing_area_video->hide();
		event_box_video->hide();
		if (!action_playlist->get_active())
		{
			resize(500, 60);
		}
	}
	else
	{
		event_box_video->show();
		drawing_area_video->show();
		resize(500,400);
	}
	if (action_fullscreen->get_active())
	{
		action_show_drawable->set_active(true);
	}
}
