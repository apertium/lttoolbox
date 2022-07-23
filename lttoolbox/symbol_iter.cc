/*
 * Copyright (C) 2021 Apertium
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <https://www.gnu.org/licenses/>.
 */

#include <lttoolbox/symbol_iter.h>

symbol_iter::symbol_iter(UString_view s_, const AlphabetExe* a_) : s(s_), a(a_)
{
  ++(*this);
}

symbol_iter::symbol_iter(const symbol_iter& other)
{
  i = other.i;
  j = other.j;
  s = other.s;
  a = other.a;
  sym = other.sym;
}

symbol_iter::~symbol_iter() {}

int32_t
symbol_iter::operator*() const
{
  return sym;
}

symbol_iter
symbol_iter::operator++(int)
{
  auto temp = *this;
  ++(*this);
  return temp;
}

symbol_iter&
symbol_iter::operator++()
{
  if (i == s.size()) {
    return *this;
  }
  i = j;
  j++;
  if (s[i] == '\\') {
    j++;
    sym = s[i+1];
  } else if (s[i] == '<') {
    while (j < s.size() && s[j] != '>') j++;
    j++;
    sym = (*a)(s.substr(i, j));
  } else {
    sym = s[i];
  }
  return *this;
}

bool
symbol_iter::operator!=(const symbol_iter& o) const
{
  return (i != o.i) || (j != o.j) || (s != o.s) || (a != o.a) || (sym != o.sym);
}

bool
symbol_iter::operator==(const symbol_iter& o) const
{
  return (i == o.i) && (j == o.j) && (s == o.s) && (a == o.a) && (sym == o.sym);
}

symbol_iter
symbol_iter::begin()
{
  return symbol_iter(s, a);
}

symbol_iter
symbol_iter::end()
{
  symbol_iter ret(s, a);
  ret.j = s.size();
  ++ret;
  return ret;
}

UString_view
symbol_iter::string()
{
  return s.substr(i,j);
}
