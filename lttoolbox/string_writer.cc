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

#include <lttoolbox/string_writer.h>

#include <stdexcept>

UString_view
StringWriter::add(const UString& s)
{
  auto start = buffer.find(s);
  if (start == UString::npos) {
    start = buffer.size();
    buffer += s;
  }
  UString_view ret(buffer);
  return ret.substr(start, s.size());
}

UString_view
StringWriter::get(const uint32_t start, const uint32_t count)
{
  UString_view ret(buffer);
  return ret.substr(start, count);
}

void
StringWriter::read(FILE* in)
{
  uint64_t len = read_u64_le(in);
  buffer.clear();
  buffer.reserve(len);
  uint8_t temp[len*2]{};
  if (fread_unlocked(&temp, 1, len*2, in) != len) {
    throw std::runtime_error("Failed to read strings");
  }
  uint16_t c;
  for (uint64_t i = 0; i < len*2; i += 2) {
    buffer += static_cast<UChar>(temp[i] | (temp[i+1] << 8));
  }
}

void
StringWriter::write(FILE* out)
{
  write_u64_le(out, buffer.size());
  uint8_t temp[buffer.size()*2]{};
  for (uint64_t i = 0; i < buffer.size(); i++) {
    temp[2*i] = buffer[i] & 0xFF;
    temp[2*i+1] = (buffer[i] >> 8) & 0xFF;
  }
  if (fwrite_unlocked(&temp, 1, buffer.size()*2, out) != buffer.size()*2) {
    throw std::runtime_error("Failed to write strings");
  }
}
