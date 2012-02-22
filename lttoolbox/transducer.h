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
#ifndef _TRANSDUCTOR_
#define _TRANSDUCTOR_

#include <cstdio>
#include <map>
#include <set>

#include <lttoolbox/alphabet.h>

using namespace std;

class MatchExe;

/**
 * Class to represent a letter transducer during the dictionary compilation
 */
class Transducer
{
private:
  friend class MatchExe;
  
  /**
   * Initial state
   */
  int initial;

  /**
   * Final state set
   */
  set<int> finals;

  /**
   * Transitions of the transducer
   */
  map<int, multimap<int, int> > transitions;

  /**
   * New state creator
   * @return the new state number
   */
  int newState();

  /**
   * Test if the intersection of two sets is empty
   * @param s1 first set
   * @param s2 second set
   * @return true if the intersection is empty
   */
  static bool isEmptyIntersection(set<int> const &s1, set<int> const &s2);

  /**
   * Copy function
   * @param t the transducer to be copied
   */
  void copy(Transducer const &t);

  /**
   * Empty transducer
   */
  void destroy();
public:

  /**
   * Constructor
   */
  Transducer();

  /**
   * Destructor
   */
  ~Transducer();

  /**
   * Copy constructor
   * @param t transducer to be copied
   */
  Transducer(Transducer const &t);

  /**
   * Assignment operator
   * @param t transducer to be assigned
   * @return the object result of the assignment
   */
  Transducer & operator =(Transducer const &t);

  /**
   * Insertion of a single transduction, creating a new target state
   * if needed  
   * @param tag the tag of the transduction being inserted
   * @param source the source state of the new transduction
   * @return the target state
   */
  int insertSingleTransduction(int const tag, int const source);

  /**
   * Insertion of a single transduction, forcing create a new target
   * state
   * @param tag the tag of the transduction being inserted
   * @param source the source state of the new transduction
   * @return the target state
   */
  int insertNewSingleTransduction(int const tag, int const source);

  /**
   * Insertion of a transducer in a given source state, unifying their
   * final states using a optionally given epsilon tag
   * @param source the source state
   * @param t the transducer being inserted
   * @param epsilon_tag the epsilon tag
   * @return the new target state
   */
  int insertTransducer(int const source, Transducer &t,
		       int const epsilon_tag = 0); 
  
  /**
   * Link two existing states by a transduction
   * @param source the source state
   * @param target the target state
   * @param tag the tag of the transduction
   */
  void linkStates(int const source, int const target, int const tag);

  /**
   * Test if the state is a final state
   * @param state the state
   * @return true if is a final state
   */
  bool isFinal(int const state) const;

  /**
   * Test if a pattern is recognised by the FST
   * @param a widestring of the pattern to be recognised
   * @return true if the pattern is recognised by the transducer
   */
  bool recognise(wstring patro, Alphabet &a, FILE *err = stderr);

  /**
   * Set the state as a final or not, yes by default
   * @param state the state
   * @param value if true, the state is set as final state
   */
  void setFinal(int const state, bool value = true);

  /**
   * Returns the initial state of a transducer
   * @return the initial state identifier
   */
  int getInitial() const;

  /**
   * Returns the epsilon closure of a given state
   * @param state the state
   * @param epsilon_tag the tag to take as epsilon
   * @return the epsilon-connected states
   */
  set<int> closure(int const state, int const epsilon_tag = 0);

  /**
   * Join all finals in one using epsilon transductions
   * @param epsilon_tag the tag to take as epsilon
   */
  void joinFinals(int const epsilon_tag = 0);

  /**
   * Reverse all the transductions of a transducer
   * @param epsilon_tag the tag to take as epsilon
   */
  void reverse(int const epsilon_tag = 0);

  /**
   * Print all the transductions of a transducer in ATT format
   * @param epsilon_tag the tag to take as epsilon
   */
  void show(Alphabet &a, FILE *output = stdout, int const epsilon_tag = 0);

  /**
   * Determinize the transducer
   * @param epsilon_tag the tag to take as epsilon
   */
  void determinize(int const epsilon_tag = 0);

  /**
   * Minimize = reverse + determinize + reverse + determinize
   * @param epsilon_tag the tag to take as epsilon
   */
  void minimize(int const epsilon_tag = 0);


  /**
   * Make a transducer optional (link initial state with final states with
   * empty transductions)
   * @param epsilon_tag the tag to take as epsilon
   */
  void optional(int const epsilon_tag = 0);

  /**
   * Make a transducer cyclic (link final states with initial state with 
   * empty transductions)
   * @param epsilon_tag the tag to take as epsilon
   */
  void oneOrMore(int const epsilon_tag = 0);

  /**
   * zeroOrMore = oneOrMore + optional
   * @param epsilon_tag the tag to take as epsilon
   */
  void zeroOrMore(int const epsilon_tag = 0);

  /**
   * Clear transducer
   */
  void clear();

  /**
   * Check if the transducer is empty
   * @return true if the transducer is empty
   */
  bool isEmpty() const;
  
  /**
   * Returns the number of states of the transducer
   * @return the number of states
   */
  int size() const;

  /**
   * Returns the number of transitions of the transducer
   * @return the number of transitions
   */
  int numberOfTransitions() const;

  /**
   * Checks if a state is empty (not outgoing transductions)
   * @param state the state to check
   * @return true if the state is empty
   */
  bool isEmpty(int const state) const;

  /**
   * Returns the number of transitions from a given state
   * @return the number of transitions
   */
  int getStateSize(int const state);

  /**
   * Write method
   * @param output the stream to write to
   * @param decalage offset to sum to the tags
   */
  void write(FILE *output, int const decalage = 0);

  /**
   * Read method
   * @param input the stream to read from
   * @param decalage offset to sum to the tags
   */
  void read(FILE *input, int const decalage = 0);
};

#endif
