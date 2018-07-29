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
	
#ifndef _TRANSEXE_
#define _TRANSEXE_

#include <cstdio>
#include <cstdlib>
#include <map>
#include <set>
#include <vector>

#include <lttoolbox/alphabet.h>
#include <lttoolbox/node.h>

using namespace std;

/**
 * Transducer class for execution of lexical processing algorithms
 */
class TransExe
{
private:
  /**
   * Initial state
   */
  int initial_id;

  /**
   * Default value of weight
   */
  double default_weight;

  /**
   * Node list
   */
  vector<Node> node_list;

  /**
   * Final node set mapped to its weight walues
   */
  map<Node *, double> finals;

  /**
   * Copy function
   * @param te the transducer to be copied
   */
  void copy(TransExe const &te);

  /**
   * Destroy function
   */
  void destroy();

public:

  /**
   * Constructor
   */
  TransExe();

  /**
   * Destructor
   */
  ~TransExe();

  /**
   * Copy constructor
   * @param te the transducer to be copied
   */
  TransExe(TransExe const &te);

  /**
   * Assignment operator
   * @param te the transducer to be assigned
   * @return the assigned object
   */
  TransExe & operator =(TransExe const &te);

  /**
   * Read method with an encoding base
   * @param input the stream
   * @param alphabet the alphabet object to decode the symbols
   */
  void read(FILE *input, Alphabet const &alphabet, bool read_weights);

  /**
   * Reduces all the final states to one
   */
  void unifyFinals();

  /**
   * Gets the initial node of the transducer
   * @return the initial node
   */
  Node * getInitial();

  /**
   * Gets the set of final nodes
   * @return the set of final nodes
   */
  map<Node *, double> & getFinals();
};

#endif
