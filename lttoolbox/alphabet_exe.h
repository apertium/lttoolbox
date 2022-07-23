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

#ifndef _LT_ALPHABET_EXE_
#define _LT_ALPHABET_EXE_

#include <lttoolbox/string_writer.h>
#include <map>
#include <set>
#include <vector>

class AlphabetExe {
private:
  StringWriter* sw;
  uint64_t tag_count;
  StringRef* tags;
  std::map<UString_view, int32_t> symbol_map;
  bool mmapping = false;
  // tags added at runtime - used by apertium-separable
  std::vector<UString> dynamic_symbols;
  // tags that should not be printed, such as <compound-only-L>
  // used by clearSymbol() if we're mmapping since we can't edit the data
  std::set<int32_t> clearedSymbols;
public:
  AlphabetExe(StringWriter* sw_);
  ~AlphabetExe();
  void read(FILE* in, bool mmap, bool compressed=true);
  void* init(void* ptr);
  int32_t operator()(UString_view sv) const;
  void getSymbol(UString& result, int32_t symbol, bool uppercase = false) const;
  bool isTag(const int32_t symbol) const;
  void clearSymbol(const int32_t symbol);
  // call this after StringWriter buffer gets updated
  void reindex();
  // like operator() but add symbol to dynamic_symbols if not found
  int32_t lookupDynamic(const UString& symbol);
  std::vector<int32_t> tokenize(const UString& str) const;
};

#endif
