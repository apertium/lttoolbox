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
#include <lttoolbox/endian_util.h>
#include <lttoolbox/old_binary.h>

#include <cctype>
#include <cstdlib>
#include <set>

#include <unicode/uchar.h>

using namespace std;
using namespace icu;

Alphabet::Alphabet()
{
  spair[pair<int32_t, int32_t>(0,0)] = 0;
  spairinv.push_back(pair<int32_t, int32_t>(0,0));
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
Alphabet::includeSymbol(UString const &s)
{
  if(slexic.find(s) == slexic.end())
  {
    int32_t slexic_size = slexic.size();
    slexic[s] = -(slexic_size+1);
    slexicinv.push_back(s);
  }
}

int32_t
Alphabet::operator()(int32_t const c1, int32_t const c2)
{
  auto tmp = make_pair(c1, c2);
  if(spair.find(tmp) == spair.end())
  {
    int32_t spair_size = spair.size();
    spair[tmp] = spair_size;
    spairinv.push_back(tmp);
  }

  return spair[tmp];
}

int32_t
Alphabet::operator()(UString const &s)
{
  return slexic[s];
}

int32_t
Alphabet::operator()(UString const &s) const
{
  auto it = slexic.find(s);
  if (it == slexic.end()) {
    return -1;
  }
  return it->second;
}

bool
Alphabet::isSymbolDefined(UString const &s) const
{
  return slexic.find(s) != slexic.end();
}

int32_t
Alphabet::size() const
{
  return slexic.size();
}

void
Alphabet::write(FILE *output)
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
  map<int32_t, string> tmp;
  for (uint64_t tam = OldBinary::read_int(input, true); tam > 0; tam--) {
    UString mytag;
    mytag += '<';
    OldBinary::read_ustr(input, mytag, true);
    mytag += '>';
    a_new.slexicinv.push_back(mytag);
    a_new.slexic[mytag]= -a_new.slexicinv.size(); // ToDo: This does not turn the result negative due to unsigned semantics
  }

  // Reading of pairlist
  size_t bias = a_new.slexicinv.size();
  for (uint64_t tam = OldBinary::read_int(input, true); tam > 0; tam--) {
    int32_t first = OldBinary::read_int(input, true);
    int32_t second = OldBinary::read_int(input, true);
    pair<int32_t, int32_t> tmp(first - bias, second - bias);
    int32_t spair_size = a_new.spair.size();
    a_new.spair[tmp] = spair_size;
    a_new.spairinv.push_back(tmp);
  }

  *this = a_new;
}

void
Alphabet::write_mmap(FILE* output, StringWriter& sw) const
{
  write_le_64(output, slexicinv.size());
  for (auto& it : slexicinv) {
    StringRef r = sw.add(it);
    write_le_32(output, r.start);
    write_le_32(output, r.count);
  }
}

void
Alphabet::read_mmap(FILE* input, StringWriter& sw)
{
  int64_t count = read_le_64(input);
  for (int64_t i = 0; i < count; i++) {
    uint32_t s = read_le_32(input);
    uint32_t c = read_le_32(input);
    UString t = UString{sw.get(s, c)};
    slexicinv.push_back(t);
    slexic[t] = -i-1;
  }
}

void
Alphabet::serialise(std::ostream &serialised) const
{
  Serialiser<const vector<UString> >::serialise(slexicinv, serialised);
  Serialiser<vector<pair<int32_t, int32_t> > >::serialise(spairinv, serialised);
}

void
Alphabet::deserialise(std::istream &serialised)
{
  slexicinv.clear();
  slexic.clear();
  spairinv.clear();
  spair.clear();
  slexicinv = Deserialiser<vector<UString> >::deserialise(serialised);
  for (size_t i = 0; i < slexicinv.size(); i++) {
    slexic[slexicinv[i]] = -i - 1; // ToDo: This does not turn the result negative due to unsigned semantics
  }
  spairinv = Deserialiser<vector<pair<int32_t, int32_t> > >::deserialise(serialised);
  for (size_t i = 0; i < slexicinv.size(); i++) {
    spair[spairinv[i]] = i;
  }
}

void
Alphabet::read_serialised(FILE* in)
{
  slexicinv.clear();
  slexic.clear();
  spairinv.clear();
  spair.clear();
  uint64_t len = OldBinary::read_int(in, false);
  for (uint64_t i = 0; i < len; i++) {
    UString t;
    OldBinary::read_ustr(in, t, false);
    slexicinv.push_back(t);
    slexic[t] = -(int)i - 1;
  }
  len = OldBinary::read_int(in, false);
  for (uint64_t i = 0; i < len; i++) {
    int32_t a = OldBinary::read_int(in, false);
    int32_t b = OldBinary::read_int(in, false);
    auto p = make_pair(a, b);
    spairinv.push_back(p);
    spair[p] = i;
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
Alphabet::getSymbol(UString &result, int32_t const symbol, bool uppercase) const
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
Alphabet::isTag(int32_t const symbol) const
{
  return symbol < 0;
}

pair<int32_t, int32_t> const &
Alphabet::decode(int32_t const code) const
{
  return spairinv[code];
}

set<int32_t>
Alphabet::symbolsWhereLeftIs(UChar32 l) const {
  set<int32_t> eps;
  for(const auto& sp: spair) {  // [(l, r) : tag]
    if(sp.first.first == l) {
      eps.insert(sp.second);
    }
  }
  return eps;
}

void Alphabet::setSymbol(int32_t symbol, UString newSymbolString) {
  //Should be a special character!
  if (symbol < 0) slexicinv[-symbol-1] = newSymbolString;
}

void
Alphabet::createLoopbackSymbols(set<int32_t> &symbols, Alphabet &basis, Side s, bool nonTagsToo)
{
  // Non-tag letters get the same int32_t in spairinv across alphabets,
  // but tags may differ, so do those separately afterwards.
  set<int32_t> tags;
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

vector<UString>&
Alphabet::getTags()
{
  return slexicinv;
}
