#include "ustring.h"

#include <stdexcept>
#include <unicode/unistr.h>

using namespace icu;

void
u_fputs(const UString& str, UFILE* output)
{
  u_fputs(str.c_str(), output);
}

int
stoi(const UString& str)
{
  int ret;
  int c = u_sscanf(str.c_str(), "%d", &ret);
  if (c != 1) {
    throw std::invalid_argument();
  }
  return ret;
}

double
stod(const UString& str)
{
  double ret;
  int c = u_sscanf(str.c_str(), "%f", &ret);
  if (c != 1) {
    throw std::invalid_argument();
  }
  return ret;
}

UString
to_ustring(const char* s)
{
  UnicodeString temp = UnicodeString::fromUTF8(s);
  UString ret = temp.getTerminatedBuffer();
  return ret;
}

char*
to_char(const UString& str)
{
  std::string stemp;
  UnicodeString utemp = str.c_str();
  utemp.toUTF8String(stemp);
  return stemp.c_str();
}

static std::ostream&
operator<<(std::ostream& ostr, const UString& str)
{
  std::string res;
  UnicodeString temp = str.c_str();
  temp.toUTF8String(res);
  ostr << res;
  return ostr;
}
