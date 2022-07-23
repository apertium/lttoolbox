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

#ifndef __SYMBOL_ITER_H__
#define __SYMBOL_ITER_H__

#include <lttoolbox/ustring.h>
#include <lttoolbox/alphabet_exe.h>

class symbol_iter
{
private:
  size_t i = 0;
  size_t j = 0;
  UString_view s;
  const AlphabetExe* a;
  int32_t sym = 0;
public:
  symbol_iter(UString_view s_, const AlphabetExe* a_);
  symbol_iter(const symbol_iter& other);
  ~symbol_iter();
  int32_t operator*() const;
  symbol_iter operator++(int);
  symbol_iter &operator++();
  bool operator!=(const symbol_iter& other) const;
  bool operator==(const symbol_iter& other) const;
  symbol_iter begin();
  symbol_iter end();
  UString_view string();
};

#endif // __SYMBOL_ITER_H__
