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

#include <lttoolbox/input_file.h>
#include <utf8.h>
#include <stdexcept>
#include <unicode/ustdio.h>
#include <cstring>
#include <iostream>
#include <lttoolbox/my_stdio.h>

InputFile::InputFile()
  : infile(stdin), buffer_size(0)
{}

InputFile::~InputFile()
{
  close();
}

bool
InputFile::open(const char* fname)
{
  close();
  if (fname == nullptr) {
    infile = stdin;
  } else {
    infile = fopen(fname, "rb");
  }
  return (infile != nullptr);
}

#if HAVE_DECL_FMEMOPEN
bool
InputFile::open_in_memory(char *input_buffer)
{
  close();
  infile = fmemopen(input_buffer, strlen(input_buffer), "rb");
  return (infile != nullptr);
}
#endif

void
InputFile::open_or_exit(const char* fname)
{
  if (!open(fname)) {
    std::cerr << "Error: Unable to open '" << fname << "' for reading." << std::endl;
    exit(EXIT_FAILURE);
  }
}

void
InputFile::close()
{
  if (infile != nullptr) {
    if (infile != stdin) {
      fclose(infile);
    }
    infile = nullptr;
  }
}

void
InputFile::wrap(FILE* newinfile)
{
  close();
  infile = newinfile;
}

void
InputFile::internal_read()
{
  if (buffer_size) {
    return;
  }
  if (feof(infile)) {
    ubuffer[buffer_size++] = U_EOF;
    return;
  }

  int first = fgetc_unlocked(infile);
  if (first == EOF) {
    ubuffer[buffer_size++] = U_EOF;
    return;
  } else if (first == '\0') {
    ubuffer[buffer_size++] = '\0';
    return;
  }

  int i = 1;
  cbuffer[0] = static_cast<char>(first);
  if ((first & 0xF0) == 0xF0) {
    i += 3;
    if (fread_unlocked(cbuffer+1, 1, 3, infile) != 3) {
      throw std::runtime_error("Could not read 3 expected bytes from stream");
    }
  } else if ((first & 0xE0) == 0xE0) {
    i += 2;
    if (fread_unlocked(cbuffer+1, 1, 2, infile) != 2) {
      throw std::runtime_error("Could not read 2 expected bytes from stream");
    }
  } else if ((first & 0xC0) == 0xC0) {
    i += 1;
    if (fread_unlocked(cbuffer+1, 1, 1, infile) != 1) {
      throw std::runtime_error("Could not read 1 expected byte from stream");
    }
  }
  memset(ubuffer, 0, 3*sizeof(UChar));
  utf8::utf8to32(cbuffer, cbuffer+i, ubuffer);
  buffer_size = 1;
}

UChar32
InputFile::get()
{
  if (!buffer_size) {
    internal_read();
  }
  return ubuffer[--buffer_size];
}

UChar32
InputFile::peek()
{
  if (!buffer_size) {
    internal_read();
  }
  return ubuffer[buffer_size-1];
}

void
InputFile::unget(UChar32 c)
{
  // this will probably segfault if called multiple times
  ubuffer[buffer_size++] = c;
}

bool
InputFile::eof()
{
  return (infile == nullptr) || feof(infile);
}

void
InputFile::rewind()
{
  if (infile != nullptr) {
    if (std::fseek(infile, 0, SEEK_SET) != 0) {
      std::cerr << "Error: Unable to rewind file" << std::endl;
      exit(EXIT_FAILURE);
    }
  }
}

UString
InputFile::readBlock(const UChar32 start, const UChar32 end)
{
  UString ret;
  ret += start;
  UChar32 c = 0;
  while (c != end && !eof()) {
    c = get();
    if (c == '\0') {
      break;
    }
    ret += c;
    if (c == '\\') {
      ret += get();
    }
  }
  return ret;
}

UString
InputFile::finishWBlank()
{
  UString ret;
  ret += '[';
  ret += '[';
  UChar32 c = 0;
  while (!eof()) {
    c = get();
    if (c == '\0') {
      break;
    }
    ret += c;
    if (c == '\\') {
      ret += get();
    } else if (c == ']' && peek() == ']') {
      ret += get();
      break;
    }
  }
  return ret;
}

UString
InputFile::readBlank(bool readwblank)
{
  UString ret;
  while (!eof()) {
    UChar32 c = get();
    if (c == '^' || c == '\0' || c == U_EOF) {
      unget(c);
      break;
    } else if (c == '[') {
      UChar32 c2 = get();
      if (c2 == '[') {
        if (readwblank) {
          ret += finishWBlank();
        } else {
          // buffer size is 3, so we should be ok here
          unget(c2);
          unget(c);
          break;
        }
      } else {
        unget(c2);
        ret += readBlock(c, ']');
      }
    } else {
      ret += c;
      if (c == '\\') {
        if (eof() || peek() == '\0') {
          std::cerr << "Unexpected trailing backslash" << std::endl;
          exit(EXIT_FAILURE);
        }
        ret += get();
      }
    }
  }
  return ret;
}
