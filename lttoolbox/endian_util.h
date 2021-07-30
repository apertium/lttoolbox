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

#ifndef _LT_ENDIAN_UTIL_
#define _LT_ENDIAN_UTIL_

#include <lttoolbox/my_stdio.h>
#include <cstdio>
#include <cstdint>
#include <stdexcept>

inline uint32_t to_le_32(uint32_t v) {
  return (((v & 0xFF) << 24) |
          ((v & 0xFF00) << 8) |
          ((v & 0xFF0000) >> 8) |
          ((v & 0xFF000000) >> 24));
}

inline uint32_t from_le_32(uint32_t v) {
  return (((v & 0xFF000000) >> 24) |
          ((v & 0xFF0000) >> 8) |
          ((v & 0xFF00) << 8) |
          ((v & 0xFF) << 24));
}

inline uint64_t to_le_64(uint64_t v) {
  return (((v & 0xFF) << 56) |
          ((v & 0xFF00) << 40) |
          ((v & 0xFF0000) << 24) |
          ((v & 0xFF000000) << 8) |
          ((v & 0xFF00000000) >> 8) |
          ((v & 0xFF0000000000) >> 24) |
          ((v & 0xFF000000000000) >> 40) |
          ((v & 0xFF00000000000000) >> 56));
}

inline uint64_t from_le_64(uint64_t v) {
  return (((v & 0xFF00000000000000) >> 56) |
          ((v & 0xFF000000000000) >> 40) |
          ((v & 0xFF0000000000) >> 24) |
          ((v & 0xFF00000000) >> 8) |
          ((v & 0xFF000000) << 8) |
          ((v & 0xFF0000) << 24) |
          ((v & 0xFF00) << 40) |
          ((v & 0xFF) << 56));
}

inline auto write_le_32(FILE* out, uint32_t value) {
  uint32_t v = to_le_32(value);
  auto rv = fwrite_unlocked(reinterpret_cast<const char*>(&v), 1, sizeof(value), out);
  if (rv != sizeof(value)) {
    throw std::runtime_error("Failed to write uint32_t");
  }
  return rv;
}

inline auto write_le_64(FILE* out, uint64_t value) {
  uint64_t v = to_le_64(value);
  auto rv = fwrite_unlocked(reinterpret_cast<const char*>(&v), 1, sizeof(value), out);
  if (rv != sizeof(value)) {
    throw std::runtime_error("Failed to write uint64_t");
  }
  return rv;
}

inline auto read_le_32(FILE* in) {
  uint32_t value = 0;
  if (fread_unlocked(reinterpret_cast<char*>(&value), 1, sizeof(value), in) != sizeof(value)) {
    throw std::runtime_error("Failed to read uint64_t");
  }
  return from_le_32(value);
}

inline auto read_le_64(FILE* in) {
  uint64_t value = 0;
  if (fread_unlocked(reinterpret_cast<char*>(&value), 1, sizeof(value), in) != sizeof(value)) {
    throw std::runtime_error("Failed to read uint64_t");
  }
  return from_le_64(value);
}

inline auto write_le_s32(FILE* out, int32_t value) {
  return write_le_32(out, *reinterpret_cast<uint32_t*>(&value));
}

inline auto read_le_s32(FILE* in) {
  uint32_t val = read_le_32(in);
  return *reinterpret_cast<int32_t*>(&val);
}

inline auto write_le_double(FILE* out, double value) {
  return write_le_64(out, *reinterpret_cast<uint64_t*>(&value));
}

inline auto read_le_double(FILE* in) {
  uint64_t val = read_le_64(in);
  return *reinterpret_cast<double*>(&val);
}

#endif
