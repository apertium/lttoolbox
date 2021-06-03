#ifndef _LT_USTRING_H_
#define _LT_USTRING_H_

#include <unicode/unistr.h>
#include <unicode/ustdio.h>
#include <string>

typedef std::basic_string<UChar> UString;

void u_fputs(const UString& str, UFILE* output);

void write(const UString& str, UFILE* output);

// like std::stoi, throws invalid_argument if unable to parse
int stoi(const UString& str);

// like std::stoi, throws invalid_argument if unable to parse
double stod(const UString& str);

// for command-line arguments
UString to_ustring(const char* str);

// for interfacing with e.g. XML library
const char* to_char(const UString& str);

static std::ostream& operator<<(std::ostream& ostr, const UString& str)
{
  std::string res;
  icu::UnicodeString temp = str.c_str();
  temp.toUTF8String(res);
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

static void operator+=(UString& str, UChar32 c)
{
  if (c <= 0xFFFF) {
    str += static_cast<UChar>(c);
  } else {
    str += static_cast<UChar>(0xD800 + ((c - 0x10000) >> 10));
    str += static_cast<UChar>(0xDC00 + (c & 0x3FF));
  }
}

#endif
