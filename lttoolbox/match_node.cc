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
#include <lttoolbox/match_node.h>

MatchNode::MatchNode(int const svsize) :
transitions(svsize)
{
}
 
MatchNode::~MatchNode()
{
  destroy();
}

MatchNode::MatchNode(MatchNode const &n) :
transitions(1)
{
  copy(n);  
}

MatchNode &
MatchNode::operator =(MatchNode const &n)
{
  if(this != &n)
  {
    destroy();
    copy(n);
  }
  return *this; 
}

void
MatchNode::copy(MatchNode const &n)
{
  transitions = n.transitions;  
}

void
MatchNode::destroy()
{
}

void
MatchNode::addTransition(int const i, MatchNode * const d, int pos)
{
//  transitions[i] = d;
  transitions.add(i, d, pos);
}
