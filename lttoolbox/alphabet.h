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
#ifndef _ALPHABET_
#define _ALPHABET_

#include <cstdio>
#include <list>
#include <map>
#include <set>
#include <vector>

#include <lttoolbox/ltstr.h>

using namespace std;

/**
 * Alphabet class.
 * Encodes pairs of symbols into an integer.
 */
class Alphabet
{
private:
  /**
   * Symbol-identifier relationship. Only contains <tags>.
   * @see slexicinv
   */
  map<wstring, int, Ltstr> slexic;  

  /**
   * Identifier-symbol relationship. Only contains <tags>.
   * @see slexic
   */
  vector<wstring> slexicinv;


  /**
   * Map from symbol-pairs to symbols; tags get negative numbers,
   * other characters are wchar_t's casted to ints.
   * @see spairinv
   */
  map<pair<int,int>, int> spair;

  /**
   * All symbol-pairs (both <tags> and letters).
   * @see spair
   */
  vector<pair<int, int> > spairinv;
  

  void copy(Alphabet const &a);
  void destroy();

public:

  /**
   * Constructor.
   */
  Alphabet();
  
  /**
   * Destructor.
   */
  ~Alphabet();
  
  /**
   * Copy constructor.
   */
  Alphabet(Alphabet const &a);
  
  /**
   * Assign operator.
   */
  Alphabet & operator = (Alphabet const &a);

  /**
   * Include a symbol into the alphabet.
   */
  void includeSymbol(wstring const &s);  

  /**
   * Get an unique code for every symbol pair.  This flavour is for
   * character pairs. Creates the code if it does not already exist.
   * @param c1 left symbol.
   * @param c2 right symbol.
   * @return code for (c1, c2).
   */
  int operator()(int const c1, int const c2);
  int operator()(wstring const &s) const;
  
  /**
   * Gets the individual symbol identifier. Assumes it already exists!
   * @see isSymbolDefined to check if it exists first.
   * @param s symbol to be identified.
   * @return symbol identifier.
   */
  int operator()(wstring const &s);

  /**
   * Check wether the symbol is defined in the alphabet.
   * @param s symbol
   * @return true if defined
   */
  bool isSymbolDefined(wstring const &s);

  /**
   * Returns the size of the alphabet (number of symbols).
   * @return number of symbols.
   */
  int size() const;

  /**
   * Write method.
   * @param output output stream.
   */
  void write(FILE *output);
  
  /**
   * Read method.
   * @param input input stream.
   */
  void read(FILE *input);

  void serialise(std::ostream &serialised) const;
  void deserialise(std::istream &serialised);

  /**
   * Write a symbol enclosed by angle brackets in the output stream.
   * @param symbol symbol code.
   * @param output output stream.
   */
  void writeSymbol(int const symbol, FILE *output) const;
  
  /**
   * Concat a symbol in the string that is passed by reference.
   * @param result string where the symbol should be concatenated
   * @param symbol code of the symbol
   * @param uppercase true if we want an uppercase symbol
   */  
  void getSymbol(wstring &result, int const symbol, 
		 bool uppercase = false) const;
		 
  /**
   * Checks whether a symbol is a tag or not.
   * @param symbol the code of the symbol
   * @return true if the symbol is a tag
   */
  bool isTag(int const symbol) const;

  /**
   * Sets an already existing symbol to represent a new value.
   * @param symbol the code of the symbol to set
   * @param newSymbolString the new string for this symbol
   */
  void setSymbol(int symbol, wstring newSymbolString);

  /**
   * Note: both the symbol int and int-pair are specific to this alphabet instance.
   * @see operator() to go from general wstrings to alphabet-specific ints.
   * @param code a symbol
   * @return the pair which code represents in this alphabet
   */
  pair<int, int> const & decode(int const code) const;

  enum Side
  {
    left,
    right
  };

  /**
   * For every symbol a:b in basis, create a pair of the form b:b (or
   * a:a if s==left), inserting the symbol into symbols, and ensuring
   * it exists in this alphabet.
   * @param basis use the symbols from this alphabet
   * @param symbols all the loopback symbols, referenced with this alphabet
   * @param s whether to loopback on the left or right side of the symbol-pair
   * @param nonTagsToo by default only tags are included, but if this is true we include all symbols
   */
  void createLoopbackSymbols(set<int> &symbols, Alphabet &basis, Side s = right, bool nonTagsToo = false);
};

#endif
