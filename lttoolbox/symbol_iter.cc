#include <lttoolbox/symbol_iter.h>
#include <unicode/uchar.h>

symbol_iter::iterator::iterator(UStringView s) : str(s)
{
  ++*this;
}

symbol_iter::iterator::iterator(const symbol_iter::iterator& other)
  : str(other.str), sloc(other.sloc), eloc(other.eloc) {}

symbol_iter::iterator::~iterator() {}

UStringView symbol_iter::iterator::operator*() const {
  return str.substr(sloc, eloc-sloc);
}

symbol_iter::iterator& symbol_iter::iterator::operator++()
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

bool symbol_iter::iterator::operator!=(const symbol_iter::iterator& o) const
{
  return str != o.str || sloc != o.sloc || eloc != o.eloc;
}

bool symbol_iter::iterator::operator==(const symbol_iter::iterator& o) const
{
  return str == o.str && sloc == o.sloc && eloc == o.eloc;
}

symbol_iter::iterator symbol_iter::begin() const
{
  return symbol_iter::iterator(str);
}

symbol_iter::iterator symbol_iter::end() const
{
  symbol_iter::iterator ret(str);
  ret.sloc = str.size();
  ret.eloc = str.size();
  return ret;
}
