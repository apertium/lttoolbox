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
write(const UString& str, UFILE* output)
{
  // u_fputs() inserts a newline
  u_fprintf(output, "%S", str.c_str());
}

int
stoi(const UString& str)
{
  int ret;
  int c = u_sscanf(str.c_str(), "%d", &ret);
  if (c != 1) {
    throw std::invalid_argument("unable to parse int");
  }
  return ret;
}

double
stod(const UString& str)
{
  double ret;
  int c = u_sscanf(str.c_str(), "%lf", &ret);
  if (c != 1) {
    throw std::invalid_argument("unable to parse float");
  }
  return ret;
}

UString
to_ustring(const char* s)
{
  auto sz = strlen(s);
  UString ret;
  ret.reserve(sz);
  utf8::utf8to16(s, s+sz, std::back_inserter(ret));
  return ret;
}

UString
to_ustring(char* s)
{
  auto sz = strlen(s);
  UString ret;
  ret.reserve(sz);
  utf8::utf8to16(s, s+sz, std::back_inserter(ret));
  return ret;
}

void
ustring_to_vec32(const UString& str, std::vector<int32_t>& vec)
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
