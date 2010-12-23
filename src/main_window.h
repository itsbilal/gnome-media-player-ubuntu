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

#ifndef __MAIN_WINDOW_H__
#define __MAIN_WINDOW_H__

#include <gtkmm.h>
#include <gconf/gconf-client.h>
#include <gtkmm/volumebutton.h>
#include <dbus/dbus.h>
#include "xine_engine.h"

class MainWindow : public Gtk::Window
{
private:
	typedef Glib::RefPtr<Gdk::Pixbuf> PixbufPtr;
	
	class ModelColumns : public Gtk::TreeModel::ColumnRecord
	{
	public:
		ModelColumns()
		{
			add(column_description);
			add(column_uri);
			add(column_image);
			add(column_current);
		}

		Gtk::TreeModelColumn<Glib::ustring>	column_description;
		Gtk::TreeModelColumn<Glib::ustring>	column_uri;
		Gtk::TreeModelColumn<gboolean>		column_current;
		Gtk::TreeModelColumn<PixbufPtr>		column_image;
	};
	
	int								cookie;
	DBusConnection*					dbus_connection;
	DBusError   					dbus_error;
	gboolean						is_cursor_visible;
	guint							last_motion_time;
	ModelColumns					columns;
	Engine*							engine;
	guint							timeout_source;
	bool							timeout_updating_slider;	
	PixbufPtr						pixbuf_play;
	PixbufPtr						pixbuf_empty;
	Gtk::MenuBar*					menu_bar;
	Gtk::HBox*						hbox_controls;
	Gtk::DrawingArea*				drawing_area_video;
	Gtk::EventBox*					event_box_video;
	Gtk::HScale*					hscale_video_position;
	Gtk::Label*						label_video_time;
	Gtk::ScrolledWindow*			scrolled_window_playlist;
	Gtk::Menu*						menu_video;
	Gtk::VBox*						vbox_playlist;
	Gtk::Button*					toggle_button_playlist;
	Gtk::TreeView*					tree_view_playlist;
	Gtk::VolumeButton*				volume_button;
	Gtk::Image*						image_play;
	Glib::RefPtr<Gtk::ListStore>	list_store;
	GdkCursor*						hidden_cursor;

	void play(Gtk::TreeModel::Row row);
	void inhibit(gboolean activate = true);

	static gboolean on_timeout(gpointer data);

	void on_about();
	void on_backward();
	void on_controls();
	void on_deinterlace();
	void on_engine_type_changed(const Glib::RefPtr<Gtk::RadioAction>& current);
	void on_forward();
	void on_fullscreen();
	void on_next();
	void on_open();
	void on_pause();
	void on_play();
	void on_playlist();
	void on_playlist_remove();
	void on_previous();
	void on_restart_engine();
	void on_stop();
	void on_timeout();
	void on_show_drawable();

	bool on_change_value(Gtk::ScrollType type, double new_value);
	bool on_drawing_area_expose_event(GdkEventExpose* event_expose);
	void on_engine_changed(const Glib::RefPtr<Gtk::RadioAction>& radio_action);
	bool on_event_box_video_button_press_event(GdkEventButton* event_button);
	bool on_event_box_video_motion_notify_event(GdkEventMotion* event_motion);
	void on_video_position_value_changed();
	void on_event_box_video_drag_data_received(const Glib::RefPtr<Gdk::DragContext>& context, int, int,
        const Gtk::SelectionData& selection_data, guint, guint time);
	void on_playlist_drag_data_received(const Glib::RefPtr<Gdk::DragContext>& context, int, int,
        const Gtk::SelectionData& selection_data, guint, guint time);
	void on_playlist_row_activated(const Gtk::TreeModel::Path& tree_model_path, Gtk::TreeViewColumn* column);
	bool on_scroll(GdkEventScroll* event_scroll);
	void on_volume_changed(double value);

	// Overrides
	bool on_key_press_event(GdkEventKey* event);
	bool on_motion_notify_event(GdkEventMotion* event_motion);
	void on_drag_data_received(const Glib::RefPtr<Gdk::DragContext>& context, int, int,
        const Gtk::SelectionData& selection_data, guint, guint time);

public:
	MainWindow(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& builder);
	void add_to_playlist(gchar** uris);
	void add_to_playlist(const Glib::ustring& uris);
	void clear_playlist();
};

#endif
