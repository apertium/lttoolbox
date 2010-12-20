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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */
#ifndef _ALPHABET_
#define _ALPHABET_

#include <cstdio>
#include <list>
#include <map>
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
   * Symbol-identifier relationship.
   * @see slexicinv
   */
  map<wstring, int, Ltstr> slexic;  

  /**
   * Identifier-symbol relationship.
   * @see slexic
   */
  vector<wstring> slexicinv;


  map<pair<int,int>, int> spair;
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
   * character pairs.
   * @param c1 left symbol.
   * @param c2 right symbol.
   * @return code for (c1, c2).
   */
  int operator()(int const c1, int const c2);
  
  /**
   * Gets the individual symbol identifier.
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
   * Checks whether a symbol is a tag or not
   * @param symbol the code of the symbol
   * @return true if the symbol is a tag
   */
  bool isTag(int const symbol) const;

  /**
   * Sets an already existing symbol to represent a new value
   * @param symbol the code of the symbol to set
   * @param newSymbolString the new string for this symbol
   */
  void setSymbol(int symbol, wstring newSymbolString);

  pair<int, int> const & decode(int const code) const;
  
};

#endif
