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

#include <lttoolbox/endian_util.h>
#include <stdexcept>

StringRef
StringWriter::add(UString_view s)
{
  auto start = edit_buffer.find(s);
  if (start == UString::npos) {
    start = edit_buffer.size();
    edit_buffer += s;
  }
  StringRef ret;
  ret.start = start;
  ret.count = s.size();
  return ret;
}

UString_view
StringWriter::get(const uint32_t start, const uint32_t count)
{
  if (mmapping) {
    UString_view ret(mmap_buffer, mmap_size);
    return ret.substr(start, count);
  } else {
    UString_view ret(edit_buffer);
    return ret.substr(start, count);
  }
}

UString_view
StringWriter::get(const StringRef& ref)
{
  return get(ref.start, ref.count);
}

void
StringWriter::read(FILE* in)
{
  uint64_t len = read_le_64(in);
  edit_buffer.clear();
  edit_buffer.reserve(len);
  uint8_t temp[len*2]{};
  if (fread_unlocked(&temp, 1, len*2, in) != len*2) {
    throw std::runtime_error("Failed to read strings");
  }
  uint16_t c;
  for (uint64_t i = 0; i < len*2; i += 2) {
    edit_buffer += static_cast<UChar>(temp[i] | (temp[i+1] << 8));
  }
}

void
StringWriter::write(FILE* out)
{
  write_le_64(out, edit_buffer.size());
  uint8_t temp[edit_buffer.size()*2]{};
  for (uint64_t i = 0; i < edit_buffer.size(); i++) {
    temp[2*i] = edit_buffer[i] & 0xFF;
    temp[2*i+1] = (edit_buffer[i] >> 8) & 0xFF;
  }
  if (fwrite_unlocked(&temp, 1, edit_buffer.size()*2, out) != edit_buffer.size()*2) {
    throw std::runtime_error("Failed to write strings");
  }
}

void*
StringWriter::init(void* ptr)
{
  mmapping = true;
  mmap_size = reinterpret_cast<uint64_t*>(ptr)[0];
  ptr += sizeof(uint64_t);
  mmap_buffer = reinterpret_cast<UChar*>(ptr);
  get(0, mmap_size);
  return ptr + sizeof(UChar)*mmap_size;
}
