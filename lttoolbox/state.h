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
#ifndef _STATE_
#define _STATE_

#include <list>
#include <map>
#include <set>
#include <string>
#include <vector>

#include <lttoolbox/alphabet.h>
#include <lttoolbox/node.h>
#include <lttoolbox/pool.h>

using namespace std;

/**
 * Class to represent the current state of transducer processing 
 */
class State
{
private:
  /**
   * The current state of transducer processing
   */
  multimap<Node *, vector<int> * > state;

  /**
   * Pool of wchar_t vectors, for efficience (static class)
   */
  static Pool<vector<int> > pool;  

  /**
   * Copy function
   * @param s the state to be copied
   */
  void copy(State const &s);

  /**
   * Destroy function
   */
  void destroy();

  /**
   * Make a transition, version for lowercase letters and symbols
   * @param input the input symbol
   */
  void apply(int const input);

  /**
   * Make a transition, version for lowercase and uppercase letters
   * @param input the input symbol
   * @param alt the alternative input symbol
   */
  void apply(int const input, int const alt);

  /**
   * Calculate the epsilon closure over the current state, replacing
   * its content.
   */
  void epsilonClosure();

public:
  /**
   * Constructor
   */
  State();

  /**
   * Destructor
   */
  ~State();

  /**
   * Copy constructor
   * @param s the state to be copied
   */
  State(State const &s);

  /**
   * Assignment operator
   * @param s the state to be assigned
   * @return the object that results from the assignation
   */
  State & operator =(State const &s);
  
  /**
   * Number of alive transductions
   * @return the size
   */
  int size() const;

  /**
   * step = apply + epsilonClosure
   * @param input the input symbol
   */
  void step(int const input);

  /**
   * step = apply + epsilonClosure
   * @param input the input symbol
   * @param alt the alternative input symbol
   */
  void step(int const input, int const alt);

  /**
   * Init the state with the initial node and empty output
   * @param initial the initial node of the transducer
   */
  void init(Node *initial);

  /**
   * Print all outputs of current parsing, preceeded by a bar '/',
   * from the final nodes of the state
   * @param finals the set of final nodes
   * @param a the alphabet to decode strings
   * @param escaped_chars the set of chars to be preceeded with one 
   *                      backslash
   * @param uppercase true if the word is uppercase
   * @param firstupper true if the first letter of a word is uppercase
   * @param firstchar first character of the word
   * @return the result of the transduction
   */
  wstring filterFinals(set<Node *> const &finals, Alphabet const &a,
                      set<wchar_t> const &escaped_chars,
                      bool uppercase = false,
		      bool firstupper = false,
		      int firstchar = 0) const;

  /**
   * Same as previous one, but  the output is adapted to the SAO system
   * @param finals the set of final nodes
   * @param a the alphabet to decode strings
   * @param escaped_chars the set of chars to be preceeded with one 
   *                      backslash
   * @param uppercase true if the word is uppercase
   * @param firstupper true if the first letter of a word is uppercase
   * @param firstchar first character of the word
   * @return the result of the transduction
   */
  wstring filterFinalsSAO(set<Node *> const &finals, Alphabet const &a,
                      set<wchar_t> const &escaped_chars,
                      bool uppercase = false,
		      bool firstupper = false,
		      int firstchar = 0) const;


  /**
   * Returns true if at least one record of the state references a
   * final node of the set
   * @param finals set of final nodes @return
   * @true if the state is final
   */
  bool isFinal(set<Node *> const &finals) const;
};

#endif
