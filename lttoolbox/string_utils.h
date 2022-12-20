#ifndef __LT_STRING_UTILS_H__
#define __LT_STRING_UTILS_H__

#include <lttoolbox/ustring.h>
#include <vector>

class StringUtils {
public:
  // delete leading and trailing whitespace
  static UStringView trim(UStringView str);

  // split string on delimiter
  static std::vector<UString> split(UStringView str, UStringView delim=u" ");

  // split but respect \ escapes
  static std::vector<UString> split_escaped(UStringView str, UChar delim);

  // inverse of split
  static UString join(const std::vector<UString>& vec, UStringView delim);

  // replace each occurrence of olds with news
  static UString substitute(UStringView str, UStringView olds, UStringView news);

  static UString itoa(int n);
  static std::string itoa_string(int n);
  static UString ftoa(double f);
  // these throw std::invalid_argument if parsing fails
  static int stoi(const UString& str);
  static double stod(const UString& str);

  static UString tolower(UStringView str);
  static UString toupper(UStringView str);
  static UString totitle(UStringView str);

  static UString getcase(UStringView str);
  static UString copycase(UStringView source, UStringView target);

  static bool caseequal(UStringView a, UStringView b);

  static bool startswith(UStringView str, UStringView prefix);
  static bool endswith(UStringView str, UStringView suffix);

  static UString merge_wblanks(UStringView w1, UStringView w2);
};

#endif // __LT_STRING_UTILS_H__
