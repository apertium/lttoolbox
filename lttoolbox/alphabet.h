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
#ifndef _ALPHABET_
#define _ALPHABET_

#include <cstdio>
#include <list>
#include <map>
#include <set>
#include <vector>
#include <cstdint>
#include <lttoolbox/ustring.h>

using namespace icu;

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
  std::map<UString, int32_t, std::less<>> slexic;

  /**
   * Identifier-symbol relationship. Only contains <tags>.
   * @see slexic
   */
  std::vector<UString> slexicinv;


  /**
   * Map from symbol-pairs to symbols; tags get negative numbers,
   * other characters are UChar32's casted to ints.
   * @see spairinv
   */
  std::map<std::pair<int32_t, int32_t>, int32_t> spair;

  /**
   * All symbol-pairs (both <tags> and letters).
   * @see spair
   */
  std::vector<std::pair<int32_t, int32_t> > spairinv;


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
  void includeSymbol(UStringView s);

  /**
   * Get an unique code for every symbol pair.  This flavour is for
   * character pairs. Creates the code if it does not already exist.
   * @param c1 left symbol.
   * @param c2 right symbol.
   * @return code for (c1, c2).
   */
  int32_t operator()(int32_t const c1, int32_t const c2);
  int32_t operator()(UStringView s) const;

  /**
   * Gets the individual symbol identifier. Assumes it already exists!
   * @see isSymbolDefined to check if it exists first.
   * @param s symbol to be identified.
   * @return symbol identifier.
   */
  int32_t operator()(UStringView s);

  /**
   * Check wether the symbol is defined in the alphabet.
   * @param s symbol
   * @return true if defined
   */
  bool isSymbolDefined(UStringView s) const;

  /**
   * Returns the size of the alphabet (number of symbols).
   * @return number of symbols.
   */
  int32_t size() const;

  /**
   * Write method.
   * @param output output stream.
   */
  void write(FILE *output) const;

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
  void writeSymbol(int32_t symbol, UFILE *output) const;

  /**
   * Concat a symbol in the string that is passed by reference.
   * @param result string where the symbol should be concatenated
   * @param symbol code of the symbol
   * @param uppercase true if we want an uppercase symbol
   */
  void getSymbol(UString &result, int32_t symbol,
		 bool uppercase = false) const;

  /**
   * Checks whether a symbol is a tag or not.
   * @param symbol the code of the symbol
   * @return true if the symbol is a tag
   */
  bool isTag(int32_t symbol) const;

  /**
   * Sets an already existing symbol to represent a new value.
   * @param symbol the code of the symbol to set
   * @param newSymbolString the new string for this symbol
   */
  void setSymbol(int32_t symbol, UStringView newSymbolString);

  /**
   * Note: both the symbol int and int-pair are specific to this alphabet instance.
   * @see operator() to go from general strings to alphabet-specific ints.
   * @param code a symbol
   * @return the pair which code represents in this alphabet
   */
  std::pair<int32_t, int32_t> const & decode(int32_t code) const;

  /**
   * Get all symbols where the left-hand side of the symbol-pair is l.
   */
  std::set<int32_t> symbolsWhereLeftIs(UChar32 l) const;

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
  void createLoopbackSymbols(std::set<int32_t> &symbols, const Alphabet &basis, Side s = right, bool nonTagsToo = false);

  std::vector<int32_t> tokenize(UStringView str) const;

  bool sameSymbol(int32_t tsym, const Alphabet& other, int32_t osym,
                  bool allow_anys=false) const;
};

#endif
