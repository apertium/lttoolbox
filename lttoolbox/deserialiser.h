// Copyright (C) 2005 Universitat d'Alacant / Universidad de Alicante
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.

#ifndef DESERIALISER_H
#define DESERIALISER_H

#include "exception.h"

#include <stdint.h>
#include <cstddef>
#include <limits>
#include <istream>
#include <limits>
#include <map>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <typeinfo>

#if __cplusplus >= 201103L
#include <type_traits>
#else
namespace std {
template <typename T>
struct remove_const;
template <typename T>
struct remove_const
{
    typedef T type;
};
template <typename T>
struct remove_const<const T>
{
    typedef T type;
};
}
#endif

template <typename DeserialisedType> class Deserialiser;

template <typename value_type>
class Deserialiser<std::basic_string<value_type> > {
public:
  inline static std::basic_string<value_type>
  deserialise(std::istream &Stream_);
};

template <typename first_type, typename second_type>
class Deserialiser<std::pair<first_type, second_type> > {
public:
  inline static std::pair<first_type, second_type>
  deserialise(std::istream &Stream_);
};

#ifdef __APPLE__
template <> class Deserialiser<size_t> {
public:
  inline static size_t deserialise(std::istream &Stream_);
};
#endif

template <> class Deserialiser<int64_t> {
public:
  inline static int64_t deserialise(std::istream &Stream_);
};

template <> class Deserialiser<uint64_t> {
public:
  inline static uint64_t deserialise(std::istream &Stream_);
};

template <> class Deserialiser<int32_t> {
public:
  inline static int32_t deserialise(std::istream &Stream_);
};

template <> class Deserialiser<uint32_t> {
public:
  inline static uint32_t deserialise(std::istream &Stream_);
};

template <> class Deserialiser<wchar_t> {
public:
  inline static wchar_t deserialise(std::istream &Stream_);
};

template <> class Deserialiser<char> {
public:
  inline static char deserialise(std::istream &Stream_);
};

template<> class Deserialiser<double> {
public:
  inline static double deserialise(std::istream &Stream_);
};

template <typename Container>
class Deserialiser {
public:
  inline static Container deserialise(std::istream &Stream_);
};

template <typename value_type>
std::basic_string<value_type>
Deserialiser<std::basic_string<value_type> >::deserialise(
    std::istream &Stream_) {
  uint64_t SerialisedValueCount = Deserialiser<uint64_t>::deserialise(Stream_);
  std::basic_string<value_type> SerialisedType_;

  for (; SerialisedValueCount != 0; --SerialisedValueCount) {
    SerialisedType_.push_back(Deserialiser<value_type>::deserialise(Stream_));
  }

  return SerialisedType_;
}

template <typename first_type, typename second_type>
std::pair<first_type, second_type>
Deserialiser<std::pair<first_type, second_type> >::deserialise(
    std::istream &Stream_) {
  first_type a = Deserialiser<typename std::remove_const<first_type>::type>::deserialise(Stream_);
  second_type b = Deserialiser<typename std::remove_const<second_type>::type>::deserialise(Stream_);
  return std::make_pair(a, b);
}

template <typename integer_type>
integer_type int_deserialise(std::istream &Stream_) {
  try {
    integer_type SerialisedType_ = 0;
    unsigned char SerialisedTypeSize = Stream_.get();

    if (!Stream_)
      throw DeserialisationException("can't deserialise size");

    for (; SerialisedTypeSize != 0;) {
      SerialisedType_ +=
          static_cast<integer_type>(Stream_.get())
          << std::numeric_limits<unsigned char>::digits * --SerialisedTypeSize;

      if (!Stream_)
        throw DeserialisationException("can't deserialise byte");
    }

    return SerialisedType_;
  } catch (const std::exception &exc) {
    std::stringstream what_;
    what_ << "can't deserialise " << sizeof(integer_type) << " byte integer type: " << exc.what();
    throw DeserialisationException(what_.str().c_str());
  }
}

#ifdef __APPLE__
size_t Deserialiser<size_t>::deserialise(std::istream &Stream_) {
  return int_deserialise<uint64_t>(Stream_);
}
#endif

int64_t Deserialiser<int64_t>::deserialise(std::istream &Stream_) {
  return int_deserialise<uint64_t>(Stream_);
}

uint64_t Deserialiser<uint64_t>::deserialise(std::istream &Stream_) {
  return int_deserialise<uint64_t>(Stream_);
}

int32_t Deserialiser<int32_t>::deserialise(std::istream &Stream_) {
  return int_deserialise<uint64_t>(Stream_);
}

uint32_t Deserialiser<uint32_t>::deserialise(std::istream &Stream_) {
  return int_deserialise<uint64_t>(Stream_);
}

wchar_t Deserialiser<wchar_t>::deserialise(std::istream &Stream_) {
  return int_deserialise<uint32_t>(Stream_);
}

char Deserialiser<char>::deserialise(std::istream &Stream_) {
  return int_deserialise<uint8_t>(Stream_);
}

double Deserialiser<double>::deserialise(std::istream &Stream_) {
  union {
    uint64_t i;
    double d;
  } u;
  u.i = Deserialiser<uint64_t>::deserialise(Stream_);
  return u.d;
}

template <typename Container>
Container
Deserialiser<Container>::deserialise(std::istream &Stream_) {
  uint64_t SerialisedValueCount =
      Deserialiser<uint64_t>::deserialise(Stream_);
  typename std::remove_const<Container>::type SerialisedType_;
  std::insert_iterator<typename std::remove_const<Container>::type> insert_it =
      std::inserter(SerialisedType_, SerialisedType_.begin());

  for (; SerialisedValueCount != 0; --SerialisedValueCount) {
    *(insert_it++) = Deserialiser<typename Container::value_type>::deserialise(Stream_);
  }

  return SerialisedType_;
}

#endif // DESERIALISER_H
