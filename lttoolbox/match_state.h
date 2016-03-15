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
#ifndef _MATCHSTATE_
#define _MATCHSTATE_

#include <map>
#include <set>
#include <string>
#include <vector>

#include <lttoolbox/match_node.h>

using namespace std;

/**
 * Class to represent the current state of transducer processing 
 */
class MatchState
{
private:
  static int const BUF_LIMIT;
  MatchNode **state;
  int first;
  int last;

  /**
   * The current state of transducer processing
   */
//  slist<MatchNode *> state;

  /**
   * Copy function
   * @param s the state to be copied
   */
  void copy(MatchState const &s);

  /**
   * Destroy function
   */
  void destroy();

  
  void applySymbol(MatchNode *pnode, int const symbol);
public:
  /**
   * Constructor
   */
  MatchState();

  /**
   * Destructor
   */
  ~MatchState();

  /**
   * Copy constructor
   * @param s the state to be copied
   */
  MatchState(MatchState const &s);

  /**
   * Assignment operator
   * @param s the state to be assigned
   * @return the object that results from the assignation
   */
  MatchState & operator =(MatchState const &s);
  
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
  void init(MatchNode *initial);

  int classifyFinals(map<MatchNode *, int> const &final_class, set<int> const &banned_rules) const;
  
  int classifyFinals(map<MatchNode *, int> const &final_class) const;

  void debug();
  
  void clear();
  
};

#endif
