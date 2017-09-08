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
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */
#include <lttoolbox/alphabet.h>
#include <lttoolbox/compression.h>
#include <lttoolbox/my_stdio.h>
#include <lttoolbox/serialiser.h>
#include <lttoolbox/deserialiser.h>

#include <cctype>
#include <cstdlib>
#include <set>
#include <cwchar>
#include <cwctype>

#ifdef _WIN32
#include <utf8_fwrap.h>
#endif

using namespace std;

Alphabet::Alphabet()
{
  spair[pair<int, int>(0,0)] = 0;
  spairinv.push_back(pair<int, int>(0,0));
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
Alphabet::includeSymbol(wstring const &s)
{
  if(slexic.find(s) == slexic.end())
  {
    int slexic_size = slexic.size();
    slexic[s] = -(slexic_size+1);
    slexicinv.push_back(s);
  }
}

int
Alphabet::operator()(int const c1, int const c2)
{
  pair<int, int> tmp = pair<int, int>(c1, c2);
  if(spair.find(tmp) == spair.end())
  {
    int spair_size = spair.size();
    spair[tmp] = spair_size;
    spairinv.push_back(tmp);
  }
  
  return spair[tmp];
}

int
Alphabet::operator()(wstring const &s)
{
  return slexic[s];
}

int
Alphabet::operator()(wstring const &s) const
{
  map<wstring, int, Ltstr>::const_iterator it = slexic.find(s);
  if (it == slexic.end()) {
    return -1;
  }
  return it->second;
}

bool
Alphabet::isSymbolDefined(wstring const &s)
{
  return slexic.find(s) != slexic.end();
}

int
Alphabet::size() const
{
  return slexic.size();
}

void
Alphabet::write(FILE *output)
{
  // First, we write the taglist
  Compression::multibyte_write(slexicinv.size(), output);  // taglist size
  for(unsigned int i = 0, limit = slexicinv.size(); i < limit; i++)
  {
    Compression::wstring_write(slexicinv[i].substr(1, slexicinv[i].size()-2), output);
  }

  // Then we write the list of pairs
  // All numbers are biased + slexicinv.size() to be positive or zero
  unsigned int bias = slexicinv.size();
  Compression::multibyte_write(spairinv.size(), output);
  for(unsigned int i = 0, limit = spairinv.size(); i != limit; i++)
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
  int tam = Compression::multibyte_read(input);
  map<int, string> tmp;
  while(tam > 0)
  {
    tam--;
    wstring mytag = L"<" + Compression::wstring_read(input) + L">";
    a_new.slexicinv.push_back(mytag);
    a_new.slexic[mytag]= -a_new.slexicinv.size();
  }

  // Reading of pairlist
  unsigned int bias = a_new.slexicinv.size();
  tam = Compression::multibyte_read(input);
  while(tam > 0)
  {
    tam--;
    int first = Compression::multibyte_read(input);
    int second = Compression::multibyte_read(input);
    pair<int, int> tmp(first - bias, second - bias);
	int spair_size = a_new.spair.size();
    a_new.spair[tmp] = spair_size;
    a_new.spairinv.push_back(tmp);
  }
  
  *this = a_new;
}

void
Alphabet::serialise(std::ostream &serialised) const
{
  Serialiser<const vector<wstring> >::serialise(slexicinv, serialised);
  Serialiser<vector<pair<int, int> > >::serialise(spairinv, serialised);
}

void
Alphabet::deserialise(std::istream &serialised)
{
  slexicinv.clear();
  slexic.clear();
  spairinv.clear();
  spair.clear();
  slexicinv = Deserialiser<vector<wstring> >::deserialise(serialised);
  for (size_t i = 0; i < slexicinv.size(); i++) {
    slexic[slexicinv[i]] = -i - 1;
  }
  spairinv = Deserialiser<vector<pair<int, int> > >::deserialise(serialised);
  for (size_t i = 0; i < slexicinv.size(); i++) {
    spair[spairinv[i]] = i;
  }
}

void
Alphabet::writeSymbol(int const symbol, FILE *output) const
{
  if(symbol < 0)
  {
    fputws_unlocked(slexicinv[-symbol-1].c_str(), output);
  }
  else
  {
    fputwc_unlocked(static_cast<wchar_t>(symbol), output);
  }
}

void
Alphabet::getSymbol(wstring &result, int const symbol, bool uppercase) const
{
  if(symbol == 0)
  {
    return;
  }
  
  if(!uppercase)
  {
    if(symbol >= 0)
    {
      result += static_cast<wchar_t>(symbol);
    }
    else
    {
      result.append(slexicinv[-symbol-1]);
    }
  }
  else if(symbol >= 0)
  {
    result += static_cast<wchar_t>(towupper(static_cast<wint_t>(symbol)));
  }
  else
  {
    result.append(slexicinv[-symbol-1]);
  }
}

bool
Alphabet::isTag(int const symbol) const
{
  return symbol < 0;
}

pair<int, int> const &
Alphabet::decode(int const code) const
{
  return spairinv[code];
}

void Alphabet::setSymbol(int symbol, wstring newSymbolString) {
  //Should be a special character!
  if (symbol < 0) slexicinv[-symbol-1] = newSymbolString;
}

void
Alphabet::createLoopbackSymbols(set<int> &symbols, Alphabet &basis, Side s, bool nonTagsToo)
{
  // Non-tag letters get the same int in spairinv across alphabets,
  // but tags may differ, so do those separately afterwards.
  set<int> tags;
  for(vector<pair<int, int> >::iterator it = basis.spairinv.begin(),
                                        limit = basis.spairinv.end();
      it != limit;
      it++)
  {
    if(s == left) {
      if(basis.isTag(it->first)) 
      {
        tags.insert(it->first);
      }
      else if(nonTagsToo)
      {
        symbols.insert(operator()(it->first, it->first));
      }
    }
    else {
      if(basis.isTag(it->second)) 
      {
        tags.insert(it->second);
      }
      else if(nonTagsToo)
      {
        symbols.insert(operator()(it->second, it->second));
      }
    }
  }
  for(map<wstring, int, Ltstr>::iterator it = basis.slexic.begin(),
                                         limit = basis.slexic.end();
      it != limit;
      it++)
  {
    // Only include tags that were actually seen on the correct side
    if(tags.find(it->second) != tags.end()) 
    {
      includeSymbol(it->first);
      symbols.insert(operator()(operator()(it->first),
                                operator()(it->first)));
    }
  }
}
