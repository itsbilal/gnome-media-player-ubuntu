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

#ifndef __EXCEPTION_H__
#define __EXCEPTION_H__

#include <glibmm.h>
#include <errno.h>
#include <string.h>
#include <glibmm/i18n.h>

class Exception : public Glib::Exception
{
protected:
	Glib::ustring message;
public:
	Exception(const Glib::ustring& exception_message);
	~Exception() throw() {}
	Glib::ustring what() const { return message; }
};

class SystemException : public Exception
{
private:
	Glib::ustring create_message(gint error_number, const Glib::ustring& message);
public:
	SystemException(const Glib::ustring& m) : Exception(create_message(errno, m)) {}
};

#endif
