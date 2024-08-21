#include <lttoolbox/string_utils.h>

#include <unicode/utf16.h>
#include <unicode/uchar.h>
#include <unicode/ustring.h>
#include <iostream>
#include <limits>

UStringView
StringUtils::trim(UStringView str)
{
  if (str.empty()) {
    return str;
  }
  size_t begin = 0;
  size_t end = str.size();
  size_t i = 0;
  UChar32 c;
  while (begin < end) {
    U16_GET(str.data(), begin, i, end, c);
    if (!u_isspace(c)) {
      begin = i;
      break;
    } else {
      U16_FWD_1(str.data(), i, end);
    }
  }
  i = str.size();
  U16_BACK_1(str.data(), 0, i);
  U16_GET(str.data(), 0, i, end, c);
  if (!u_isspace(c)) {
    if (begin == 0) {
      return str;
    } else {
      return str.substr(begin);
    }
  }
  while (end > begin) {
    end = i;
    U16_BACK_1(str.data(), 0, i);
    U16_GET(str.data(), 0, i, str.size(), c);
    if (!u_isspace(c)) {
      break;
    }
  }
  return str.substr(begin, end-begin);
}

std::vector<UString>
StringUtils::split(UStringView str, UStringView delim)
{
  size_t pos = 0;
  size_t new_pos;
  std::vector<UString> result;
  while (pos < str.size()) {
    new_pos = str.find(delim, pos);
    if (new_pos == UStringView::npos) {
      new_pos = str.size();
    }
    if (new_pos > pos) {
      // if we have a non-empty substring between this delimiter
      // and the last one
      result.push_back(US(str.substr(pos, new_pos-pos)));
    }
    pos = new_pos + delim.size();
  }
  return result;
}

std::vector<UString>
StringUtils::split_escaped(UStringView str, UChar delim)
{
  std::vector<UString> result;
  size_t start = 0;
  for (size_t i = 0; i < str.size(); i++) {
    if (str[i] == '\\') {
      i++;
      continue;
    }
    if (str[i] == delim) {
      if (i > start) {
        result.push_back(US(str.substr(start, i-start)));
      }
      start = i+1;
    }
  }
  if (start < str.size()) {
    result.push_back(US(str.substr(start)));
  }
  return result;
}

UString
StringUtils::join(const std::vector<UString>& vec, UStringView delim)
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
StringUtils::substitute(UStringView str, UStringView olds, UStringView news)
{
  UString s{str};
  size_t p = s.find(olds, 0);
  while (p != UStringView::npos) {
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
StringUtils::tolower(UStringView str)
{
  UString buf(str.size()*2, 0);
  UErrorCode err = U_ZERO_ERROR;
  buf.resize(u_strToLower(&buf[0], str.size() * 2, str.data(), str.size(), NULL, &err));
  if (U_FAILURE(err)) {
    std::cerr << "Error: unable to lowercase string '" << str << "'.\n";
    std::cerr << "error code: " << u_errorName(err) << std::endl;
    exit(EXIT_FAILURE);
  }
  return buf;
}

UString
StringUtils::toupper(UStringView str)
{
  UString buf(str.size()*2, 0);
  UErrorCode err = U_ZERO_ERROR;
  buf.resize(u_strToUpper(&buf[0], str.size()*2, str.data(), str.size(), NULL, &err));
  if (U_FAILURE(err)) {
    std::cerr << "Error: unable to uppercase string '" << str << "'.\n";
    std::cerr << "error code: " << u_errorName(err) << std::endl;
    exit(EXIT_FAILURE);
  }
  return buf;
}

UString
StringUtils::totitle(UStringView str)
{
  UString buf(str.size()*2, 0);
  UErrorCode err = U_ZERO_ERROR;
  buf.resize(u_strToTitle(&buf[0], str.size()*2, str.data(), str.size(), NULL, NULL, &err));
  if (U_FAILURE(err)) {
    std::cerr << "Error: unable to titlecase string '" << str << "'.\n";
    std::cerr << "error code: " << u_errorName(err) << std::endl;
    exit(EXIT_FAILURE);
  }
  return buf;
}

UString
StringUtils::getcase(UStringView str)
{
  UString ret = "aa"_u;
  if (str.empty()) {
    return ret;
  }
  size_t i = 0;
  size_t l = str.size();
  UChar32 c;
  U16_NEXT(str.data(), i, l, c);
  if (u_isupper(c)) {
    ret[0] = 'A';
    if (i < l) {
      U16_BACK_1(str.data(), i, l); // decrements l
      U16_GET(str.data(), 0, l, str.size(), c);
      if (u_isupper(c)) {
        ret[1] = 'A';
      }
    }
  }
  return ret;
}

UString
StringUtils::copycase(UStringView source, UStringView target)
{
  if (source.empty() || target.empty()) {
    return US(target);
  }
  size_t i = 0;
  size_t l = source.size();
  UChar32 c;
  U16_NEXT(source.data(), i, l, c);
  bool firstupper = u_isupper(c);
  bool uppercase = false;
  if (firstupper) {
    if (i != l) {
      U16_BACK_1(source.data(), i, l); // decrements l
      U16_GET(source.data(), 0, l, source.size(), c);
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
StringUtils::caseequal(UStringView a, UStringView b)
{
  UErrorCode err = U_ZERO_ERROR;
  int cmp = u_strCaseCompare(a.data(), a.size(), b.data(), b.size(), 0, &err);
  if (U_FAILURE(err)) {
    std::cerr << "Error: caseless string comparison failed on '";
    std::cerr << a << "' and '" << b << "'" << std::endl;
    std::cerr << "error code: " << u_errorName(err) << std::endl;
    exit(EXIT_FAILURE);
  }
  return (cmp == 0);
}

bool
StringUtils::startswith(UStringView str, UStringView prefix)
{
  return (prefix.size() <= str.size() &&
          str.substr(0, prefix.size()) == prefix);
}

bool
StringUtils::endswith(UStringView str, UStringView suffix)
{
  return (suffix.size() <= str.size() &&
          str.substr(str.size()-suffix.size()) == suffix);
}

UString
StringUtils::merge_wblanks(UStringView w1, UStringView w2)
{
  if (w1.empty()) return US(w2);
  if (w2.empty()) return US(w1);
  UString ret = US(w1.substr(0, w1.size()-2));
  ret += "; "_u;
  ret += w2.substr(2);
  return ret;
}
