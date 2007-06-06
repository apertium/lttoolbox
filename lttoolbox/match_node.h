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

#ifndef _MATCHNODE_
#define _MATCHNODE_

#include <cstdlib>
#include <list>
#include <map>
#include <lttoolbox/sorted_vector.h>

class MatchState;

using namespace std;

//class MatchNode;
//typedef map<int, MatchNode *> MNode;

typedef SortedVector MNode;

/**
 * Node class of TransExe.  State is a friend class since the
 * algorithms are implemented in MatchState
 */
class MatchNode
{
private:
  friend class MatchState;
  
  /**
   * The outgoing transitions from this node. 
   * Schema: (input symbol, destination)
   */
  MNode transitions;

  /**
   * Copy method
   * @param n the node to be copied
   */
  void copy(MatchNode const &n);

  /**
   * Destroy method
   */
  void destroy();

public:

  /**
   * Constructor
   */
  MatchNode(int const svsize);

  /**
   * Destructor
   */
  ~MatchNode();

  /**
   * Copy constructor 
   * @param n the node to be copied
   */
  MatchNode(MatchNode const &n);

  /**
   * Assignment operator
   * @param n the node to be assigned
   * @return the assigned object
   */
  MatchNode & operator=(MatchNode const &n);

  /**
   * Making a link between this node and another
   * @param i input symbol
   * @param d destination
   */
  void addTransition(int const i, MatchNode * const d, int pos);
};

#endif
