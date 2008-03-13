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

#ifndef _NODE_
#define _NODE_

#include <cstdlib>
#include <list>
#include <map>

class State;
class Node;

using namespace std;

class Dest
{
private: 
  int size;
  int *out_tag;
  Node **dest;
  
  friend class State;
  friend class Node;
  
  void copy(Dest const &d)
  {
    destroy();
    size = d.size;
    out_tag = new int[size];
    dest = new Node*[size];
  }
  
  void destroy()
  {
    if(size != 0)
    {
      size = 0;
      if(out_tag)
      {
        delete[] out_tag;
      }
      if(dest)
      {
        delete[] dest;
      }
    }
  }

  void init()
  {
    size = 0;
    out_tag = NULL;
    dest = NULL;  
  }

public:
  Dest()
  {
    init();
  }
  
  ~Dest()
  {
    destroy();
  }
  
  Dest(Dest const &d)
  {
    init();
    copy(d);
  }

  Dest & operator=(Dest const &d)
  {
    if(this != &d)
    {
      destroy();
      copy(d);
    }
    return *this;
  }
};



/**
 * Node class of TransExe.  State is a friend class since the
 * algorithms are implemented in State
 */
class Node
{
private:
  friend class State;

  /**
   * The outgoing transitions of this node. 
   * Schema: (input symbol, (output symbol, destination))
   */
  map<int, Dest> transitions;

  /**
   * Copy method
   * @param n the node to be copied
   */
  void copy(Node const &n);

  /**
   * Destroy method
   */
  void destroy();

public:

  /**
   * Constructor
   */
  Node();

  /**
   * Destructor
   */
  ~Node();

  /**
   * Copy constructor 
   * @param n the node to be copied
   */
  Node(Node const &n);

  /**
   * Assignment operator
   * @param n the node to be assigned
   * @return the assigned object
   */
  Node & operator=(Node const &n);

  /**
   * Making a link between this node and another
   * @param i input symbol
   * @param o output symbol
   * @param d destination
   */
  void addTransition(int i, int o, Node * const d);
};

#endif
