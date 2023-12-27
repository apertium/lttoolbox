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

#ifndef _LT_INPUT_FILE_H_
#define _LT_INPUT_FILE_H_

#include <cstdio>
#include <unicode/uchar.h>
#include <lttoolbox/ustring.h>

class InputFile
{
private:
  FILE* infile;
  UChar32 ubuffer[3];
  char cbuffer[4];
  int buffer_size;
  void internal_read();
public:
  InputFile();
  ~InputFile();
  bool open(const char* fname = nullptr);
#if HAVE_DECL_FMEMOPEN
  bool open_in_memory(char* input_buffer);
#endif
  void open_or_exit(const char* fname = nullptr);
  void close();
  void wrap(FILE* newinfile);
  UChar32 get();
  UChar32 peek();
  void unget(UChar32 c);
  bool eof();
  void rewind();
  // assumes that start has already been read
  // returns string from start to end inclusive
  // respects backslash escapes
  UString readBlock(const UChar32 start, const UChar32 end);
  // assumes [[ has already been read, reads to ]]
  // returns entire string, including brackets
  UString finishWBlank();
  // read until ^ or \0
  // if readwblank == false, also stop at [[
  // Note: relies on the fact that ubuffer has length >= 2
  UString readBlank(bool readwblank = false);
};

#endif
