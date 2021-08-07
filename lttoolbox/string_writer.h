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

#ifndef _LT_STRING_WRITER_
#define _LT_STRING_WRITER_

#include <lttoolbox/ustring.h>
#include <cstdint>
#include <cstdio>

struct StringRef {
  uint32_t start;
  uint32_t count;
};

class StringWriter {
private:
  bool mmapping = false;
  UString edit_buffer;
  uint64_t mmap_size;
  UChar* mmap_buffer;
public:
  StringRef add(UString_view s);
  StringRef find(UString_view s) const;
  UString_view get(const uint32_t start, const uint32_t count);
  UString_view get(const StringRef& ref);
  void read(FILE* in);
  void write(FILE* out);
  void* init(void* ptr);
};

#endif
