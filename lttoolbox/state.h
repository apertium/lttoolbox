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

#include <map>
#include <set>
#include <string>
#include <vector>
#include <queue>

#include <lttoolbox/alphabet.h>
#include <lttoolbox/node.h>
#include <lttoolbox/match_exe.h>
#include <lttoolbox/match_state.h>
#include <lttoolbox/transducer.h>

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
  struct TNodeState
  {
    Node *where;
    vector<int> *sequence;
    bool dirty; // What does "dirty" mean ? 
    
    TNodeState(Node * const &w, vector<int> * const &s, bool const &d): where(w), sequence(s), dirty(d){}
    TNodeState & operator=(TNodeState const &other)
    {
      where = other.where;
      sequence = other.sequence;
      dirty = other.dirty;
      return *this;
    }
  };
  
  vector<TNodeState> state;

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

  void apply(int const input, int const alt1, int const alt2);

  /**
   * Calculate the epsilon closure over the current state, replacing
   * its content.
   */
  void epsilonClosure();

  bool lastPartHasRequiredSymbol(const vector<int> &seq, int requiredSymbol, int separationSymbol);

public:

  /**
   * Copy function
   * @param s the state to be copied
   */
  void copy(State const &s);


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

  void step(int const input, int const alt1, int const alt2);

  void step_case(wchar_t val, bool caseSensitive);

  void step_case(wchar_t val, wchar_t val2, bool caseSensitive);


  /**
   * Init the state with the initial node and empty output
   * @param initial the initial node of the transducer
   */
  void init(Node *initial);

  /**
    * Remove states not containing a specific symbol in their last 'part', and states 
    * with more than a number of 'parts'
    * @param requieredSymbol the symbol requiered in the last part
    * @param separationSymbol the symbol that represent the separation between two parts
    * @param compound_max_elements the maximum part number allowed
    */
  void pruneCompounds(int requiredSymbol, int separationSymbol, int compound_max_elements);

  /**
    * Remove states containing a forbidden symbol
    * @param forbiddenSymbol the symbol forbidden
    */
  void pruneStatesWithForbiddenSymbol(int forbiddenSymbol);

  /**
   * Print all outputs of current parsing, preceded by a bar '/',
   * from the final nodes of the state
   * @param finals the set of final nodes
   * @param a the alphabet to decode strings
   * @param escaped_chars the set of chars to be preceded with one 
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
   * @param escaped_chars the set of chars to be preceded with one 
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
   * Same as previous one, but  the output is adapted to the LRX system
   * @param finals the set of final nodes
   * @param a the alphabet to decode strings
   * @param escaped_chars the set of chars to be preceded with one 
   *                      backslash
   * @param uppercase true if the word is uppercase
   * @param firstupper true if the first letter of a word is uppercase
   * @param firstchar first character of the word
   * @return the result of the transduction
   */

  set<pair<wstring, vector<wstring> > > filterFinalsLRX(set<Node *> const &finals, Alphabet const &a,
                      set<wchar_t> const &escaped_chars,
                      bool uppercase = false,
                      bool firstupper = false,
                      int firstchar = 0) const;





  /**
   * Find final states, remove those that not has a requiredSymbol and 'restart' each of them as the 
   * set of initial states, but remembering the sequence and adding a separationSymbol
   * @param finals
   * @param requiredSymbol
   * @param restart_state
   * @param separationSymbol
   */
    void restartFinals(const set<Node *> &finals, int requiredSymbol, State *restart_state, int separationSymbol);


  /**
   * Returns true if at least one record of the state references a
   * final node of the set
   * @param finals set of final nodes @return
   * @true if the state is final
   */
  bool isFinal(set<Node *> const &finals) const;

  /**
   * Return the full states string (to allow debuging...) using a Java ArrayList.toString style
   */
  wstring getReadableString(const Alphabet &a);

  wstring filterFinalsTM(set<Node *> const &finals, 
			 Alphabet const &alphabet,
                         set<wchar_t> const &escaped_chars,
			 queue<wstring> &blanks, 
                         vector<wstring> &numbers) const;
};

#endif
