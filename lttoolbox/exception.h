/*
 * Copyright (C) 2005 Universitat d'Alacant / Universidad de Alicante
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */
#ifndef __EXCEPTION_
#define __EXCEPTION_

#include <exception>
#include <string>

class Exception
: public std::exception
{
public:
  Exception(const char* _msg) throw ()
  : std::exception(), msg(_msg)
  {
  }

  virtual ~Exception() throw ()
  {
  }

  const char* what() const throw ()
  {
    return msg.c_str();
  }

private:
  std::string msg;
};

class IOException : public Exception {
public:
  IOException(const char* _msg) throw () : Exception(_msg) {};
};

class SerialisationException : public IOException {
public:
  SerialisationException(const char* _msg) throw () : IOException(_msg) {};
};

class DeserialisationException : public IOException {
public:
  DeserialisationException(const char* _msg) throw () : IOException(_msg) {};
};

#endif
