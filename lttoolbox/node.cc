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
#include <lttoolbox/node.h>

Node::Node()
{
}

Node::~Node()
{
  destroy();
}

Node::Node(Node const &n)
{
  copy(n);  
}

Node &
Node::operator =(Node const &n)
{
  if(this != &n)
  {
    destroy();
    copy(n);
  }
  return *this; 
}

void
Node::copy(Node const &n)
{
  transitions = n.transitions;  
}

void
Node::destroy()
{
}

void
Node::addTransition(int const i, int const o, Node * const d)
{
  Dest &aux = transitions[i];
  aux.size++;
  int *out_tag = new int[aux.size];
  Node **dest = new Node*[aux.size];
 
  for(int i = 0; i<aux.size-1; i++)
  {
    out_tag[i] = aux.out_tag[i];
    dest[i] = aux.dest[i];
  }
  
  if(aux.size > 1)
  {
    delete[] aux.out_tag;
    delete[] aux.dest;
  }

  out_tag[aux.size-1] = o;
  dest[aux.size-1] = d;
  aux.out_tag = out_tag;
  aux.dest = dest;
}
