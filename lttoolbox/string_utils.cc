#include <lttoolbox/string_utils.h>

#include <unicode/utf16.h>
#include <unicode/uchar.h>
#include <unicode/ustring.h>
#include <iostream>
#include <limits>

UString
StringUtils::trim(const UString& str)
{
  if (str.empty()) {
    return str;
  }
  size_t begin = 0;
  size_t end = str.size();
  size_t i = 0;
  UChar32 c;
  while (begin < end) {
    U16_GET(str.c_str(), begin, i, end, c);
    if (!u_isspace(c)) {
      begin = i;
      break;
    } else {
      U16_FWD_1(str.c_str(), i, end);
    }
  }
  i = str.size();
  U16_BACK_1(str.c_str(), 0, i);
  U16_GET(str.c_str(), 0, i, end, c);
  if (!u_isspace(c)) {
    if (begin == 0) {
      return str;
    } else {
      return str.substr(begin);
    }
  }
  while (end > begin) {
    end = i;
    U16_BACK_1(str.c_str(), 0, i);
    U16_GET(str.c_str(), 0, i, str.size(), c);
    if (!u_isspace(c)) {
      break;
    }
  }
  return str.substr(begin, end-begin);
}

std::vector<UString>
StringUtils::split(const UString& str, const UString& delim)
{
  size_t pos = 0;
  size_t new_pos;
  std::vector<UString> result;
  while (pos < str.size()) {
    new_pos = str.find(delim, pos);
    if (new_pos == UString::npos) {
      new_pos = str.size();
    }
    if (new_pos > pos) {
      // if we have a non-empty substring between this delimiter
      // and the last one
      result.push_back(str.substr(pos, new_pos-pos));
    }
    pos = new_pos + delim.size();
  }
  return result;
}

std::vector<UString_view>
StringUtils::split_escape(UString_view str, const UChar delim)
{
  std::vector<UString_view> ret;
  size_t last = 0;
  for (size_t i = 0; i < str.size(); i++) {
    if (str[i] == '\\') {
      i++;
    } else if (str[i] == delim) {
      if (i > last) {
        ret.push_back(str.substr(last, i-last));
      }
      last = i+1;
    }
  }
  if (str.size() > last) {
    ret.push_back(str.substr(last));
  }
  return ret;
}

UString
StringUtils::join(const std::vector<UString>& vec, const UString& delim)
{
  UString s;
  for (auto& piece : vec) {
    if (!s.empty()) {
      s.append(delim);
    }
    s.append(piece);
  }
  return s;
}

UString
StringUtils::substitute(const UString& str, const UString& olds, const UString& news)
{
  UString s = str;
  size_t p = s.find(olds, 0);
  while (p != UString::npos) {
    s.replace(p, olds.length(), news);
    p += news.length();
    p = s.find(olds, p);
  }
  return s;
}

UString
StringUtils::itoa(int n)
{
  UChar str[256];
  u_snprintf(str, 256, "%d", n);
  return str;
}

std::string
StringUtils::itoa_string(int n)
{
  char str[256];
  snprintf(str, 256, "%d", n);
  return str;
}

UString
StringUtils::ftoa(double f)
{
  UChar str[256];
  u_snprintf(str, 256, "%f", f);
  return str;
}

int
StringUtils::stoi(const UString& str)
{
  int ret;
  int c = u_sscanf(str.c_str(), "%d", &ret);
  if (c != 1) {
    throw std::invalid_argument("unable to parse int");
  }
  return ret;
}

double
StringUtils::stod(const UString& str)
{
  double ret;
  int c = u_sscanf(str.c_str(), "%lf", &ret);
  if (str.size() == 3 && str[0] == 'i' && str[1] == 'n' && str[2] == 'f') {
    ret = std::numeric_limits<double>::infinity();
    c = 1;
  }
  if (str.size() == 4 && str[0] == '-' && str[1] == 'i' && str[2] == 'n' && str[3] == 'f') {
    ret = -1*std::numeric_limits<double>::infinity();
    c = 1;
  }
  if (c != 1) {
    throw std::invalid_argument("unable to parse float");
  }
  return ret;
}

UString
StringUtils::tolower(const UString& str)
{
  UChar buf[str.size()*2];
  UErrorCode err = U_ZERO_ERROR;
  u_strToLower(buf, str.size()*2, str.c_str(), str.size(), NULL, &err);
  if (U_FAILURE(err)) {
    std::cerr << "Error: unable to lowercase string '" << str << "'.\n";
    std::cerr << "error code: " << u_errorName(err) << std::endl;
    exit(EXIT_FAILURE);
  }
  return buf;
}

UString
StringUtils::toupper(const UString& str)
{
  UChar buf[str.size()*2];
  UErrorCode err = U_ZERO_ERROR;
  u_strToUpper(buf, str.size()*2, str.c_str(), str.size(), NULL, &err);
  if (U_FAILURE(err)) {
    std::cerr << "Error: unable to uppercase string '" << str << "'.\n";
    std::cerr << "error code: " << u_errorName(err) << std::endl;
    exit(EXIT_FAILURE);
  }
  return buf;
}

UString
StringUtils::totitle(const UString& str)
{
  UChar buf[str.size()*2];
  UErrorCode err = U_ZERO_ERROR;
  u_strToTitle(buf, str.size()*2, str.c_str(), str.size(), NULL, NULL, &err);
  if (U_FAILURE(err)) {
    std::cerr << "Error: unable to titlecase string '" << str << "'.\n";
    std::cerr << "error code: " << u_errorName(err) << std::endl;
    exit(EXIT_FAILURE);
  }
  return buf;
}

UString
StringUtils::getcase(const UString& str)
{
  UString ret = "aa"_u;
  if (str.empty()) {
    return ret;
  }
  size_t i = 0;
  size_t l = str.size();
  UChar32 c;
  U16_NEXT(str.c_str(), i, l, c);
  if (u_isupper(c)) {
    ret[0] = 'A';
    if (i < l) {
      U16_BACK_1(str.c_str(), i, l); // decrements l
      U16_GET(str.c_str(), 0, l, str.size(), c);
      if (u_isupper(c)) {
        ret[1] = 'A';
      }
    }
  }
  return ret;
}

UString
StringUtils::copycase(const UString& source, const UString& target)
{
  if (source.empty() || target.empty()) {
    return target;
  }
  size_t i = 0;
  size_t l = source.size();
  UChar32 c;
  U16_NEXT(source.c_str(), i, l, c);
  bool firstupper = u_isupper(c);
  bool uppercase = false;
  if (firstupper) {
    if (i != l) {
      U16_BACK_1(source.c_str(), i, l); // decrements l
      U16_GET(source.c_str(), 0, l, source.size(), c);
      uppercase = u_isupper(c);
    }
  }
  if (firstupper) {
    if (uppercase) {
      return toupper(target);
    } else {
      return totitle(target);
    }
  } else {
    return tolower(target);
  }
}

bool
StringUtils::caseequal(const UString& a, const UString& b)
{
  UErrorCode err = U_ZERO_ERROR;
  int cmp = u_strCaseCompare(a.c_str(), -1, b.c_str(), -1, 0, &err);
  if (U_FAILURE(err)) {
    std::cerr << "Error: caseless string comparison failed on '";
    std::cerr << a << "' and '" << b << "'" << std::endl;
    std::cerr << "error code: " << u_errorName(err) << std::endl;
    exit(EXIT_FAILURE);
  }
  return (cmp == 0);
}

bool
StringUtils::endswith(const UString& str, const UString& suffix)
{
  return (suffix.size() <= str.size() &&
          str.substr(str.size()-suffix.size()) == suffix);
}

UString
StringUtils::merge_wblanks(const UString& w1, const UString& w2)
{
  if (w1.empty()) return w2;
  if (w2.empty()) return w1;
  UString ret = w1.substr(0, w1.size()-2);
  ret += "; "_u;
  ret += w2.substr(2);
  return ret;
}
