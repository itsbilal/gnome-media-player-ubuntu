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

#include "engine.h"
#include "exception.h"
#include "gnome-media-player.h"
#include <iomanip>

Engine::~Engine()
{
}

void Engine::increment(bool forward)
{
	int length = get_length();
	int time = get_time();

	if (time == 0 || length == 0)
	{
		g_debug("%s 0.2%%", forward ? "Incrementing" : "Decrementing");
		set_percentage(get_percentage() + (forward ? 0.002 : -0.002));
	}
	else
	{
		g_debug("%s 10 seconds", forward ? "Incrementing" : "Decrementing");
		set_time(get_time() + (forward ? 10000 : -10000));
	}
}

Glib::ustring Engine::format_2_digit(gint time)
{
	return Glib::ustring::format(std::setfill(L'0'), std::setw(2), time);
}

Glib::ustring Engine::format_time(gint time)
{
	Glib::ustring result;

	gint total_seconds = time / 1000;
	gint hours = total_seconds / 60 / 60;
	gint minutes = total_seconds / 60 % 60;
	gint seconds = total_seconds % 60;

	if (hours == 0)
	{
		result = Glib::ustring::compose("%1:%2", minutes, format_2_digit(seconds));
	}
	else
	{
		result = Glib::ustring::compose("%1:%2:%3", hours, format_2_digit(minutes), format_2_digit(seconds));
	}
	
	return result;
}

Glib::ustring Engine::get_text()
{
	int length = get_length();
	int time = get_time();
	Glib::ustring result;
	
	if (time == 0 || length == 0)
	{
		float percentage = get_percentage() * 100;
		if (percentage > 0.01)
		{
			result = Glib::ustring::compose("%1%%", (int)(percentage));
		}
	}
	else
	{
		result = Glib::ustring::compose("%1/%2", format_time(time), format_time(length));
	}

	return result;
}
