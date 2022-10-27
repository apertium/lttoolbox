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

#ifndef _LT_USTRING_H_
#define _LT_USTRING_H_

#include <unicode/ustdio.h>
#include <unicode/uchar.h>
#include <string>
#include <string_view>
#include <utf8.h>
#include <vector>
#include <cstdint>
#include <iomanip>

typedef std::basic_string<UChar> UString;
typedef std::basic_string_view<UChar> UStringView;

void write(UStringView str, UFILE* output);

UString to_ustring(const char* str);
UString to_ustring(const uint8_t* str);

// append UTF-16 string to UTF-32 vector of symbols
void ustring_to_vec32(UStringView str, std::vector<int32_t>& vec);

inline std::ostream&
operator<<(std::ostream& ostr, char16_t c)
{
  utf8::utf16to8(&c, &c+1, std::ostream_iterator<char>(ostr));
  return ostr;
}

inline std::ostream&
operator<<(std::ostream& ostr, UStringView str)
{
  utf8::utf16to8(str.begin(), str.end(), std::ostream_iterator<char>(ostr));
  return ostr;
}

inline UString operator "" _u(const char* str, std::size_t len) {
	UString us(len, 0);
	for (size_t i = 0; i < len; ++i) {
		us[i] = str[i];
	}
	return us;
}

inline UString operator "" _u(const char16_t* str, std::size_t len) {
	UString us(len, 0);
	for (size_t i = 0; i < len; ++i) {
		us[i] = str[i];
	}
	return us;
}

inline UStringView operator "" _uv(const char16_t* str, std::size_t len) {
	return UStringView(str, len);
}

inline UString US(UStringView usv) {
	return UString(usv);
}

inline void operator+=(UString& str, UChar32 c)
{
  if (c <= 0xFFFF) {
    str += static_cast<UChar>(c);
  } else {
    str += static_cast<UChar>(0xD800 + ((c - 0x10000) >> 10));
    str += static_cast<UChar>(0xDC00 + (c & 0x3FF));
  }
}

#endif
