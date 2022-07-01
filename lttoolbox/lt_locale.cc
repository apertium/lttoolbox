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
 * along with this program; if not, see <https://www.gnu.org/licenses/>.
 */
#include <lttoolbox/lt_locale.h>
#include <unicode/uloc.h>
#include <unicode/ucnv.h>

#include <clocale>
#include <iostream>
#ifdef __MINGW32__
#include <windows.h>
#endif


void
LtLocale::tryToSetLocale()
{
  try {
    std::locale::global(std::locale(std::locale::classic(), "", std::locale::ctype));
  }
  catch (...) {
    // Nothing
  }

  UErrorCode status = U_ZERO_ERROR;
  uloc_setDefault("en_US_POSIX", &status);
  ucnv_setDefaultName("UTF-8");

#if !defined(__CYGWIN__) && !defined (__MINGW32__)
  if(setlocale(LC_CTYPE, "") != NULL)
  {
    return;
  }

  std::cerr << "Warning: unsupported locale, fallback to \"C\"" << std::endl;

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
