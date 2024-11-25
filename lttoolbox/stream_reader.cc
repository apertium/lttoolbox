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

#include <stream_reader.h>

StreamReader::StreamReader(InputFile* i) : in(i) {}

StreamReader::~StreamReader() {}

void StreamReader::next() {
  blank.clear();
  wblank.clear();
  readings.clear();
  chunk.clear();

  if (at_eof) return;

  at_null = false;

  blank = in->readBlank(false);

  UChar32 c = in->get();

  if (c == '[') {
    in->get();
    wblank = in->finishWBlank();
    if (in->peek() != '^') {
      UString temp = blank + wblank;
      next();
      blank = temp + blank;
      return;
    }
    c = in->get();
  }

  if (c == '\0') {
    at_null = true;
    return;
  }

  if (in->eof()) {
    at_eof = true;
    return;
  }

  while (c != '$' && c != '\0' && !in->eof()) {
    readings.resize(readings.size()+1);
    auto& cur = readings.back();
    c = in->peek();
    if (c == '*' || c == '@' || c == '#' || c == '=' || c == '%') {
      in->get();
      cur.mark = c;
    }
    c = in->get();
    while (c != '/' && c != '$' && c != '\0' && !in->eof()) {
      if (c == '<') {
        UString tag = in->readBlock('<', '>');
        cur.content += tag;
        if (alpha) {
          if (add_unknowns) alpha->includeSymbol(tag);
          cur.symbols.push_back((*alpha)(tag));
        }
      }
      else if (c == '{') {
        chunk += in->readBlock('{', '}');
      }
      else {
        cur.content += c;
        if (c == '\\') {
          UChar32 c2 = in->get();
          if (alpha) cur.symbols.push_back(static_cast<int32_t>(c2));
          cur.content += c2;
        }
        else if (alpha) {
          cur.symbols.push_back(static_cast<int32_t>(c));
        }
      }
      c = in->get();
    }
  }

  if (c == '\0') at_null = true;
  else if (c == U_EOF || in->eof()) at_eof = true;
}
