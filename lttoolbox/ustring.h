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
#include <utf8.h>
#include <vector>
#include <cstdint>
#include <iomanip>
#include <lttoolbox/string_view.h>

typedef std::basic_string<UChar> UString;
typedef std::basic_string_view<UChar> UString_view;

void write(const UString& str, UFILE* output);

UString to_ustring(const char* str);
UString to_ustring(const uint8_t* str);

// append UTF-16 string to UTF-32 vector of symbols
void ustring_to_vec32(UString_view str, std::vector<int32_t>& vec);

inline std::ostream&
operator<<(std::ostream& ostr, char16_t c)
{
  ostr << std::hex << static_cast<uint16_t>(c);
  return ostr;
}

inline std::ostream&
operator<<(std::ostream& ostr, UString_view str)
{
  std::string res;
  utf8::utf16to8(str.begin(), str.end(), std::back_inserter(res));
  ostr << res;
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
