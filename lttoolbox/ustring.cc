/*
 * Copyright (C) 2021 Apertium
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

#include "ustring.h"

#include <stdexcept>
#include <utf8.h>
#include <cstring>
#include <unicode/utf16.h>

using namespace icu;

void
write(UStringView str, UFILE* output)
{
  // u_fputs() inserts a newline
  u_fprintf(output, "%.*S", str.size(), str.data());
}

UString
to_ustring(const char* s)
{
  return to_ustring(reinterpret_cast<const uint8_t*>(s));
}

UString
to_ustring(const uint8_t* s)
{
  auto sz = strlen(reinterpret_cast<const char*>(s));
  UString ret;
  ret.reserve(sz);
  utf8::utf8to16(s, s+sz, std::back_inserter(ret));
  return ret;
}

void
ustring_to_vec32(UStringView str, std::vector<int32_t>& vec)
{
  if (str.empty()) {
    return;
  }

  size_t i = 0;
  size_t len = str.size();
  vec.reserve(vec.size() + str.size());
  int32_t c;
  while (i < str.size()) {
    U16_NEXT(str, i, len, c);
    vec.push_back(c);
  }
}
