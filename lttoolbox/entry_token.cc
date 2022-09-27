/*
 * Copyright (C) 2005 Universitat d'Alacant / Universidad de Alicante
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
#include <lttoolbox/entry_token.h>


EntryToken::EntryToken() :
type(paradigm)
{
}

EntryToken::~EntryToken()
{
  destroy();
}

EntryToken::EntryToken(EntryToken const &e)
{
  copy(e);
}

EntryToken &
EntryToken::operator =(EntryToken const &e)
{
  if(this != &e)
  {
    destroy();
    copy(e);
  }

  return *this;
}

void
EntryToken::copy(EntryToken const &e)
{
  type = e.type;
  weight = e.weight;
  leftSide = e.leftSide;
  rightSide = e.rightSide;
  parName = e.parName;
  myregexp = e.myregexp;
}

void
EntryToken::destroy()
{
}

void
EntryToken::setParadigm(UStringView np)
{
  parName = np;
  type = paradigm;
}

void
EntryToken::setSingleTransduction(std::vector<int> const &pi, std::vector<int> const &pd, double const ew)
{
  weight = ew;
  leftSide = pi;
  rightSide = pd;
  type = single_transduction;
}

void
EntryToken::setRegexp(UStringView r)
{
  myregexp.clear();
  ustring_to_vec32(r, myregexp);
  type = regexp;
}

void
EntryToken::setRegexp(const std::vector<int32_t>& r)
{
  myregexp = r;
  type = regexp;
}

void
EntryToken::readRegexp(xmlTextReaderPtr reader)
{
  XMLParseUtil::readValueInto32(reader, myregexp);
  type = regexp;
}

bool
EntryToken::isParadigm() const
{
  return type == paradigm;
}

bool
EntryToken::isSingleTransduction() const
{
  return type == single_transduction;
}

bool
EntryToken::isRegexp() const
{
  return type == regexp;
}

UString const &
EntryToken::paradigmName() const
{
  return parName;
}

std::vector<int> const &
EntryToken::left() const
{
  return leftSide;
}

std::vector<int> const &
EntryToken::right() const
{
  return rightSide;
}

std::vector<int32_t> const &
EntryToken::regExp() const
{
  return myregexp;
}

double const &
EntryToken::entryWeight() const
{
  return weight;
}
