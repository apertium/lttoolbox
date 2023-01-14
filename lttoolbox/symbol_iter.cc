#include <lttoolbox/symbol_iter.h>
#include <unicode/uchar.h>

symbol_iter::symbol_iter(UStringView s) : str(s)
{
  ++*this;
}

symbol_iter::symbol_iter(const symbol_iter& other)
  : str(other.str), sloc(other.sloc), eloc(other.eloc) {}

symbol_iter::~symbol_iter() {}

UStringView symbol_iter::operator*() {
  return str.substr(sloc, eloc-sloc);
}

symbol_iter& symbol_iter::operator++()
{
  if (sloc < str.size()) {
    sloc = eloc;
    UChar32 c;
    U16_NEXT(str.data(), eloc, str.size(), c);
    if (c == '\\') {
      sloc++;
      U16_NEXT(str.data(), eloc, str.size(), c);
    } else if (c == '<') {
      auto i = eloc;
      while (c != '>' && i < str.size()) U16_NEXT(str.data(), i, str.size(), c);
      if (c == '>') eloc = i;
    }
    if (eloc > str.size()) eloc = str.size();
  }
  return *this;
}

bool symbol_iter::operator!=(const symbol_iter& o) const
{
  return str != o.str || sloc != o.sloc || eloc != o.eloc;
}

bool symbol_iter::operator==(const symbol_iter& o) const
{
  return str == o.str && sloc == o.sloc && eloc == o.eloc;
}

symbol_iter symbol_iter::begin()
{
  return symbol_iter(str);
}

symbol_iter symbol_iter::end()
{
  symbol_iter ret(str);
  ret.sloc = str.size();
  ret.eloc = str.size();
  return ret;
}
