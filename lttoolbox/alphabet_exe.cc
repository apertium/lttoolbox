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

#include <lttoolbox/alphabet_exe.h>

#include <lttoolbox/compression.h>
#include <lttoolbox/endian_util.h>

AlphabetExe::AlphabetExe(StringWriter* sw_)
  : sw(sw_), tag_count(0), tags(nullptr)
{}

AlphabetExe::~AlphabetExe()
{
  if (!mmapping) {
    delete[] tags;
  }
}

void
AlphabetExe::read(FILE* input, bool mmap)
{
  if (mmap) {
    tag_count = read_le_64(input);
    tags = new StringRef[tag_count];
    for (uint64_t i = 0; i < tag_count; i++) {
      tags[i].start = read_le_32(input);
      tags[i].count = read_le_32(input);
      symbol_map[sw->get(tags[i])] = -static_cast<int32_t>(i) - 1;
    }
  } else {
    tag_count = Compression::multibyte_read(input);
    tags = new StringRef[tag_count];
    for (uint32_t i = 0; i < tag_count; i++) {
      UString tg;
      tg += '<';
      tg += Compression::string_read(input);
      tg += '>';
      tags[i] = sw->add(tg);
    }
    // has to be a separate loop, otherwise the string_views get
    // invalidated when the StringWriter buffer expands
    for (uint32_t i = 0; i < tag_count; i++) {
      symbol_map[sw->get(tags[i])] = -static_cast<int32_t>(i) - 1;
    }
    int pairs = Compression::multibyte_read(input);
    for (int i = 0; i < pairs; i++) {
      Compression::multibyte_read(input);
      Compression::multibyte_read(input);
    }
  }
}

void*
AlphabetExe::init(void* ptr)
{
  mmapping = true;
  tag_count = from_le_64(reinterpret_cast<uint64_t*>(ptr)[0]);
  tags = reinterpret_cast<StringRef*>(ptr + sizeof(uint64_t));
  for (uint64_t i = 0; i < tag_count; i++) {
    symbol_map[sw->get(tags[i])] = -static_cast<int32_t>(i) - 1;
  }
  return ptr + sizeof(uint64_t) + tag_count*sizeof(StringRef);
}

int32_t
AlphabetExe::operator()(UString_view sv)
{
  auto it = symbol_map.find(sv);
  if (it != symbol_map.end()) {
    return it->second;
  } else {
    return 0;
  }
}

void
AlphabetExe::getSymbol(UString& result, int32_t symbol, bool uppercase) const
{
  if (symbol == 0) {
    return;
  } else if (symbol < 0) {
    int idx = -symbol-1;
    if (idx < tag_count) {
      result.append(sw->get(tags[idx]));
    } else {
      result.append(dynamic_symbols[idx-tag_count]);
    }
  } else if (uppercase) {
    result += u_toupper(static_cast<UChar32>(symbol));
  } else {
    result += static_cast<UChar32>(symbol);
  }
}

bool
AlphabetExe::isTag(const int32_t symbol) const
{
  return symbol < 0;
}

void
AlphabetExe::clearSymbol(const int32_t symbol)
{
  if (symbol < 0) {
    tags[-symbol-1].start = 0;
    tags[-symbol-1].count = 0;
  }
}

int32_t
AlphabetExe::lookupDynamic(const UString& symbol)
{
  int32_t ret;
  auto it = symbol_map.find(symbol);
  if (it == symbol_map.end()) {
    if (dynamic_symbols.empty()) {
      // should be able to usually avoid reindexing with this
      dynamic_symbols.reserve(32);
    }
    ret = -tag_count -dynamic_symbols.size() -1;
    bool rebuild = (dynamic_symbols.size() == dynamic_symbols.capacity());
    dynamic_symbols.push_back(symbol);
    symbol_map[dynamic_symbols.back()] = ret;
    if (rebuild) {
      // moderately horrible, but that's what we get for invalidating
      // all the views when dynamic_symbols gets reallocated
      symbol_map.clear();
      for (uint64_t i = 0; i < tag_count; i++) {
        symbol_map[sw->get(tags[i])] = -static_cast<int32_t>(i) - 1;
      }
      int32_t n = -tag_count-1;
      for (auto& ds : dynamic_symbols) {
        symbol_map[ds] = n--;
      }
    }
  } else {
    ret = it->second;
  }
  return ret;
}