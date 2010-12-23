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

#include "exception.h"

Exception::Exception(const Glib::ustring& exception_message) : message(exception_message)
{
	g_message("Exception: %s", message.c_str());
}

Glib::ustring SystemException::create_message(gint error_number, const Glib::ustring& exception_message)
{
	Glib::ustring detail = _("Failed to get error message");
	
	char* system_error_message = strerror(error_number);
	if (system_error_message != NULL)
	{
		detail = system_error_message;
	}
	
	return Glib::ustring::compose("%1: %2", exception_message, detail);
}
