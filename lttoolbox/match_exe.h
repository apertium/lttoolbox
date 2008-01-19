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
	
#ifndef _MATCHEXE_
#define _MATCHEXE_

#include <cstdio>
#include <cstdlib>
#include <map>
#include <set>
#include <vector>

#include <lttoolbox/match_node.h>
#include <lttoolbox/transducer.h>

using namespace std;

/**
 * Matcher class for execution of lexical recognizing algorithms
 */
class MatchExe
{
private:
  /**
   * Initial state
   */
  int initial_id;

  /**
   * MatchNode list
   */
  vector<MatchNode> node_list;

  /**
   * Set of final nodes
   */
  map<MatchNode *, int> finals;

  /**
   * Copy function
   * @param te the transducer to be copied
   */
  void copy(MatchExe const &te);

  /**
   * Destroy function
   */
  void destroy();

public:
  
  /**
   * Constructor
   */
  MatchExe();

  /**
   * From transducer constructor
   * @param t the transducer
   * @param final_type the final types
   */
   MatchExe(Transducer const &t, map<int, int> const &final_type);

  /**
   * Destructor
   */
  ~MatchExe();

  /**
   * Copy constructor
   * @param te the transducer to be copied
   */
  MatchExe(MatchExe const &te);

  /**
   * Assignment operator
   * @param te the transducer to be assigned
   * @return the assigned object
   */
  MatchExe & operator =(MatchExe const &te);

  /**
   * Gets the initial node of the transducer
   * @return the initial node
   */
  MatchNode * getInitial();

  /**
   * Gets the set of final nodes
   * @return the set of final nodes
   */
  map<MatchNode *, int> & getFinals();
};

#endif
