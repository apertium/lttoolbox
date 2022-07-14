/*
 * Copyright (C) 2022 Apertium
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
#ifndef __VECSET_H__
#define __VECSET_H__

#include <set>
#include <vector>

template <class T>
class VecSet : public std::vector<T> {
public:
  void add(const T value) {
    if (this->empty()) {
      this->push_back(value);
      return;
    }
    int l = 0;
    int r = this->size();
    int m;
    while (l < r) {
      m = (l + r) / 2;
      if (this->operator[](m) == value) {
        return;
      } else if (this->operator[](m) < value) {
        l = m+1;
      } else {
        r = m;
      }
    }
    this->insert(this->begin()+l, value);
  }

  bool has(const T value) const {
    int l = 0;
    int r = this->size();
    int m;
    while (l < r) {
      m = (l + r) / 2;
      if (this->operator[](m) == value) {
        return true;
      } else if (this->operator[](m) < value) {
        l = m+1;
      } else {
        r = m;
      }
    }
    return false;
  }

  void union_with(const VecSet<T>& other) {
    VecSet<T> temp;
    auto ti = this->begin();
    auto oi = other.begin();
    auto te = this->end();
    auto oe = other.end();
    while (ti != te || oi != oe) {
      if (ti == te) {
        temp.push_back(*oi);
        oi++;
      } else if (oi == oe) {
        temp.push_back(*ti);
        ti++;
      } else if (*ti == *oi) {
        temp.push_back(*ti);
        ti++;
        oi++;
      } else if (*ti < *oi) {
        temp.push_back(*ti);
        ti++;
      } else {
        temp.push_back(*oi);
        oi++;
      }
    }
    this->swap(temp);
  }

  void union_with(const std::set<T>& other) {
    VecSet<T> temp;
    auto ti = this->begin();
    auto oi = other.begin();
    auto te = this->end();
    auto oe = other.end();
    while (ti != te || oi != oe) {
      if (ti == te) {
        temp.push_back(*oi);
        oi++;
      } else if (oi == oe) {
        temp.push_back(*ti);
        ti++;
      } else if (*ti == *oi) {
        temp.push_back(*ti);
        ti++;
        oi++;
      } else if (*ti < *oi) {
        temp.push_back(*ti);
        ti++;
      } else {
        temp.push_back(*oi);
        oi++;
      }
    }
    this->swap(temp);
  }

  bool overlaps(const VecSet<T>& other) {
    auto ti = this->begin();
    auto oi = other.begin();
    auto te = this->end();
    auto oe = other.end();
    while (ti != te && oi != oe) {
      if (*ti == *oi) {
        return true;
      } else if (*ti < *oi) {
        ti++;
      } else {
        oi++;
      }
    }
    return false;
  }

  T pop() {
    T ret = this->back();
    this->pop_back();
    return ret;
  }
};

#endif
