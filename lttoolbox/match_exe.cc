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

#include <lttoolbox/match_exe.h>
#include <lttoolbox/lttoolbox_config.h>
#include <lttoolbox/my_stdio.h>
#include <lttoolbox/compression.h>

MatchExe::MatchExe()
{
}

MatchExe::~MatchExe()
{
  destroy();
}

MatchExe::MatchExe(MatchExe const &te)
{
  copy(te);
}

MatchExe::MatchExe(Transducer const &t, map<int, int > const &final_type)
{
  // memory allocation
  node_list.reserve(t.transitions.size());

  for(map<int, multimap<int, int> >::const_iterator it = t.transitions.begin(),
        limit = t.transitions.end(); it != limit; it++)
  {
    MatchNode mynode(it->second.size());
    node_list.push_back(mynode);
  }
  
  // set up finals
  for(map<int, int>::const_iterator it = final_type.begin(), limit = final_type.end();
      it != limit; it++)
  {
    finals[&node_list[it->first]] = it->second;
  }  
 
  // set up initial node
  initial_id = t.initial;
 
  // set up the transitions
  for(map<int, multimap<int, int> >::const_iterator it = t.transitions.begin(),
        limit = t.transitions.end(); it != limit; it++)
  {
    MatchNode &mynode = node_list[it->first];
    int i = 0;
    for(multimap<int, int>::const_iterator it2 = it->second.begin(),
          limit2 = it->second.end(); it2 != limit2; it2++)
    {
      mynode.addTransition(it2->first, &node_list[it2->second], i++);
    }
  }
}

MatchExe &
MatchExe::operator =(MatchExe const &te)
{
  if(this != &te)
  {
    destroy();
    copy(te);
  }
  return *this;
}

void
MatchExe::copy(MatchExe const &te)
{
  initial_id = te.initial_id;
  node_list = te.node_list;
  finals = te.finals;
}

void
MatchExe::destroy()
{
}

MatchNode *
MatchExe::getInitial()
{
  return &node_list[initial_id];
}

map<MatchNode *, int> &
MatchExe::getFinals()
{
  return finals;
}
