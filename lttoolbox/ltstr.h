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
#ifndef _Ltstr_
#define _Ltstr_

#include <string>
#include <cwchar>
#include <cstring>

using namespace std;

struct Ltstr
{
  bool operator()(string const &s1, string const &s2) const
  {
    return strcmp(s1.c_str(), s2.c_str()) < 0;
  }

  bool operator()(wchar_t const *s1, wchar_t const *s2) const
  {
    return wcscmp(s1, s2) < 0;
  }

  bool operator()(char const *s1, char const *s2) const
  {
    return strcmp(s1, s2) < 0;
  }

  bool operator()(wstring const &s1, wstring const &s2) const
  {
    return wcscmp(s1.c_str(), s2.c_str()) < 0;
  }
};

#endif
