/*
 * Copyright (C) 2024 Apertium
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

#ifndef __LT_STREAM_READER_H__
#define __LT_STREAM_READER_H__

#include <lttoolbox/alphabet.h>
#include <lttoolbox/input_file.h>
#include <unicode/ustdio.h>

class StreamReader {
private:
  InputFile* in;
public:
  struct Reading {
    UChar32 mark = '\0';
    UString content;
    std::vector<int32_t> symbols;
  };
  bool at_null = false;
  bool at_eof = false;
  UString blank;
  UString wblank;
  std::vector<Reading> readings;
  UString chunk;

  Alphabet* alpha = nullptr;
  bool add_unknowns = false;

  StreamReader(InputFile* i);
  ~StreamReader();
  void next();
};

#endif
