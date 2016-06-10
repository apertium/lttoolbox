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
#include <lttoolbox/lt_locale.h>

#include <clocale>
#include <iostream>
#ifdef __MINGW32__
#include <windows.h>
#endif

using namespace std;


void
LtLocale::tryToSetLocale()
{
#if !defined(__CYGWIN__) && !defined (__MINGW32__)
  if(setlocale(LC_CTYPE, "") != NULL)
  {
    return;
  }
 
  wcerr << "Warning: unsupported locale, fallback to \"C\"" << endl;

  setlocale(LC_ALL, "C");
#endif
#ifdef __CYGWIN__
  setlocale(LC_ALL, "C.UTF-8");
#endif
#ifdef __MINGW32__
  //SetConsoleInputCP(65001);
  SetConsoleOutputCP(65001);
#endif
}
