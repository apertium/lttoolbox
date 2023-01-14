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
#include <lttoolbox/alphabet.h>
#include <lttoolbox/compression.h>
#include <lttoolbox/my_stdio.h>
#include <lttoolbox/serialiser.h>
#include <lttoolbox/deserialiser.h>
#include <lttoolbox/symbol_iter.h>

#include <cctype>
#include <cstdlib>
#include <set>

#include <unicode/uchar.h>

using namespace icu;

Alphabet::Alphabet()
{
  spair[std::pair<int32_t, int32_t>(0,0)] = 0;
  spairinv.push_back(std::pair<int32_t, int32_t>(0,0));
}

Alphabet::~Alphabet()
{
  destroy();
}

Alphabet::Alphabet(Alphabet const &a)
{
  copy(a);
}

Alphabet &
Alphabet::operator =(Alphabet const &a)
{
  if(this != &a)
  {
    destroy();
    copy(a);
  }
  return *this;
}

void
Alphabet::destroy()
{
}

void
Alphabet::copy(Alphabet const &a)
{
  slexic = a.slexic;
  slexicinv = a.slexicinv;
  spair = a.spair;
  spairinv = a.spairinv;
}

void
Alphabet::includeSymbol(UStringView s)
{
  if(slexic.find(s) == slexic.end())
  {
    int32_t slexic_size = slexic.size();
    UString st{s};
    slexic[st] = -(slexic_size+1);
    slexicinv.push_back(st);
  }
}

int32_t
Alphabet::operator()(int32_t const c1, int32_t const c2)
{
  auto tmp = std::make_pair(c1, c2);
  if(spair.find(tmp) == spair.end())
  {
    int32_t spair_size = spair.size();
    spair[tmp] = spair_size;
    spairinv.push_back(tmp);
  }

  return spair[tmp];
}

int32_t
Alphabet::operator()(UStringView s)
{
  // While the documentation says this assumes existence, there are clearly code paths that call it with an unknown symbol and thus get 0 back AND create an entry for that 0. Changing it to just return 0 still passes all tests.
  auto it = slexic.find(s);
  if (it == slexic.end()) {
    return 0;
  }
  return it->second;
}

int32_t
Alphabet::operator()(UStringView s) const
{
  auto it = slexic.find(s);
  if (it == slexic.end()) {
    return -1;
  }
  return it->second;
}

bool
Alphabet::isSymbolDefined(UStringView s) const
{
  return slexic.find(s) != slexic.end();
}

int32_t
Alphabet::size() const
{
  return slexic.size();
}

void
Alphabet::write(FILE *output) const
{
  // First, we write the taglist
  Compression::multibyte_write(slexicinv.size(), output);  // taglist size
  for(size_t i = 0, limit = slexicinv.size(); i < limit; i++)
  {
    Compression::string_write(slexicinv[i].substr(1, slexicinv[i].size()-2), output);
  }

  // Then we write the list of pairs
  // All numbers are biased + slexicinv.size() to be positive or zero
  size_t bias = slexicinv.size();
  Compression::multibyte_write(spairinv.size(), output);
  for(size_t i = 0, limit = spairinv.size(); i != limit; i++)
  {
    Compression::multibyte_write(spairinv[i].first + bias, output);
    Compression::multibyte_write(spairinv[i].second + bias, output);
  }
}

void
Alphabet::read(FILE *input)
{
  Alphabet a_new;
  a_new.spairinv.clear();
  a_new.spair.clear();

  // Reading of taglist
  int32_t tam = Compression::multibyte_read(input);
  std::map<int32_t, std::string> tmp;
  while(tam > 0)
  {
    tam--;
    UString mytag = "<"_u;
    mytag += Compression::string_read(input);
    mytag += ">"_u;
    a_new.slexicinv.push_back(mytag);
    a_new.slexic[mytag]= -a_new.slexicinv.size(); // ToDo: This does not turn the result negative due to unsigned semantics
  }

  // Reading of pairlist
  size_t bias = a_new.slexicinv.size();
  tam = Compression::multibyte_read(input);
  while(tam > 0)
  {
    tam--;
    int32_t first = Compression::multibyte_read(input);
    int32_t second = Compression::multibyte_read(input);
    std::pair<int32_t, int32_t> tmp(first - bias, second - bias);
    int32_t spair_size = a_new.spair.size();
    a_new.spair[tmp] = spair_size;
    a_new.spairinv.push_back(tmp);
  }

  *this = a_new;
}

void
Alphabet::serialise(std::ostream &serialised) const
{
  Serialiser<const std::vector<UString> >::serialise(slexicinv, serialised);
  Serialiser<std::vector<std::pair<int32_t, int32_t> > >::serialise(spairinv, serialised);
}

void
Alphabet::deserialise(std::istream &serialised)
{
  slexicinv.clear();
  slexic.clear();
  spairinv.clear();
  spair.clear();
  slexicinv = Deserialiser<std::vector<UString> >::deserialise(serialised);
  for (size_t i = 0; i < slexicinv.size(); i++) {
    slexic[slexicinv[i]] = -i - 1; // ToDo: This does not turn the result negative due to unsigned semantics
  }
  spairinv = Deserialiser<std::vector<std::pair<int32_t, int32_t> > >::deserialise(serialised);
  for (size_t i = 0; i < slexicinv.size(); i++) {
    spair[spairinv[i]] = i;
  }
}

void
Alphabet::writeSymbol(int32_t const symbol, UFILE *output) const
{
  if(symbol < 0)
  {
    // write() has a name conflict
    u_fprintf(output, "%S", slexicinv[-symbol-1].c_str());
  }
  else
  {
    u_fputc(static_cast<UChar32>(symbol), output);
  }
}

void
Alphabet::getSymbol(UString &result, int32_t symbol, bool uppercase) const
{
  if (symbol == 0) {
    return;
  } else if (symbol < 0) {
    result.append(slexicinv[-symbol-1]);
  } else if (uppercase) {
    result += u_toupper(static_cast<UChar32>(symbol));
  } else {
    result += static_cast<UChar32>(symbol);
  }
}

bool
Alphabet::isTag(int32_t symbol) const
{
  return symbol < 0;
}

std::pair<int32_t, int32_t> const &
Alphabet::decode(int32_t code) const
{
  return spairinv[code];
}

std::set<int32_t>
Alphabet::symbolsWhereLeftIs(UChar32 l) const {
  std::set<int32_t> eps;
  for(const auto& sp: spair) {  // [(l, r) : tag]
    if(sp.first.first == l) {
      eps.insert(sp.second);
    }
  }
  return eps;
}

void Alphabet::setSymbol(int32_t symbol, UStringView newSymbolString) {
  //Should be a special character!
  if (symbol < 0) slexicinv[-symbol-1] = newSymbolString;
}

void
Alphabet::createLoopbackSymbols(std::set<int32_t> &symbols, const Alphabet &basis, Side s, bool nonTagsToo)
{
  // Non-tag letters get the same int32_t in spairinv across alphabets,
  // but tags may differ, so do those separately afterwards.
  std::set<int32_t> tags;
  for(auto& it : basis.spairinv)
  {
    if(s == left) {
      if(basis.isTag(it.first))
      {
        tags.insert(it.first);
      }
      else if(nonTagsToo)
      {
        symbols.insert(operator()(it.first, it.first));
      }
    }
    else {
      if(basis.isTag(it.second))
      {
        tags.insert(it.second);
      }
      else if(nonTagsToo)
      {
        symbols.insert(operator()(it.second, it.second));
      }
    }
  }
  for(auto& it : basis.slexic)
  {
    // Only include tags that were actually seen on the correct side
    if(tags.find(it.second) != tags.end())
    {
      includeSymbol(it.first);
      symbols.insert(operator()(operator()(it.first),
                                operator()(it.first)));
    }
  }
}

std::vector<int32_t>
Alphabet::tokenize(UStringView str) const
{
  std::vector<int32_t> ret;
  for (auto sym : symbol_iter(str)) {
    if (sym.size() > 1) ret.push_back(operator()(sym));
    else ret.push_back(static_cast<int32_t>(sym[0]));
  }
  return ret;
}

bool
Alphabet::sameSymbol(int32_t tsym, const Alphabet& other, int32_t osym,
                     bool allow_anys) const
{
  // if it's a letter, then it's equal across alphabets
  if (tsym >= 0 && tsym == osym) return true;
  if (tsym < 0 && osym < 0 &&
      this->slexicinv[-tsym-1] == other.slexicinv[-osym-1]) {
    return true;
  }
  if (allow_anys &&
      ((tsym < 0 && this->slexicinv[-tsym-1] == u"<ANY_CHAR>"_uv && osym > 0) ||
       (tsym < 0 && this->slexicinv[-tsym-1] == u"<ANY_TAG>"_uv && osym < 0) ||
       (osym < 0 && other.slexicinv[-osym-1] == u"<ANY_CHAR>"_uv && tsym > 0) ||
       (osym < 0 && other.slexicinv[-osym-1] == u"<ANY_TAG>"_uv && tsym < 0))) {
    return true;
  }
  return false;
}
