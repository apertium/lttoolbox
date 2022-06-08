#ifndef __LT_STRING_UTILS_H__
#define __LT_STRING_UTILS_H__

#include <lttoolbox/ustring.h>
#include <vector>

class StringUtils {
public:
  // delete leading and trailing whitespace
  static UString trim(const UString& str);

  // split string on delimiter
  static std::vector<UString> split(const UString& str, const UString& delim);

  // inverse of split
  static UString join(const std::vector<UString>& vec, const UString& delim);

  // replace each occurrence of olds with news
  static UString substitute(const UString& str, const UString& olds, const UString& news);

  static UString itoa(int n);
  static std::string itoa_string(int n);
  static UString ftoa(double f);
  // these throw std::invalid_argument if parsing fails
  static int stoi(const UString& str);
  static double stod(const UString& str);

  static UString tolower(const UString& str);
  static UString toupper(const UString& str);
  static UString totitle(const UString& str);

  static UString getcase(const UString& str);
  static UString copycase(const UString& source, const UString& target);

  static bool caseequal(const UString& a, const UString& b);

  static bool endswith(const UString& str, const UString& suffix);

  static UString merge_wblanks(const UString& w1, const UString& w2);
};

#endif // __LT_STRING_UTILS_H__
