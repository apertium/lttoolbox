#include "ustring.h"

#include <stdexcept>
#include <unicode/unistr.h>
#include <utf8.h>
#include <cstring>

using namespace icu;

void
u_fputs(const UString& str, UFILE* output)
{
  u_fputs(str.c_str(), output);
}

void
write(const UString& str, UFILE* output)
{
  // u_fputs() inserts a newline
  u_fprintf(output, "%S", str.c_str());
}

int
stoi(const UString& str)
{
  int ret;
  int c = u_sscanf(str.c_str(), "%d", &ret);
  if (c != 1) {
    throw std::invalid_argument("unable to parse int");
  }
  return ret;
}

double
stod(const UString& str)
{
  double ret;
  int c = u_sscanf(str.c_str(), "%lf", &ret);
  if (c != 1) {
    throw std::invalid_argument("unable to parse float");
  }
  return ret;
}

UString
to_ustring(const char* s)
{
  auto sz = strlen(s);
  UString ret;
  ret.reserve(sz);
  utf8::utf8to16(s, s+sz, std::back_inserter(ret));
  return ret;
}

const char*
to_char(const UString& str)
{
  std::string stemp;
  UnicodeString utemp = str.c_str();
  utemp.toUTF8String(stemp);
  return stemp.c_str();
}
