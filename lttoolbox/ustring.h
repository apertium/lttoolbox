#ifndef _LT_USTRING_H_
#define _LT_USTRING_H_

#include <unicode/unistr.h>
#include <unicode/ustdio.h>
#include <string>

typedef std::basic_string<UChar> UString;

void u_fputs(const UString& str, UFILE* output);

// like std::stoi, throws invalid_argument if unable to parse
int stoi(const UString& str);

// like std::stoi, throws invalid_argument if unable to parse
double stod(const UString& str);

// for command-line arguments
UString to_ustring(const char* str);

// for interfacing with e.g. XML library
char* to_char(const UString& str);

static std::ostream& operator<<(std::ostream& ostr, const UString& str);

#endif
