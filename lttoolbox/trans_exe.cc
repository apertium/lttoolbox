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

#include <lttoolbox/trans_exe.h>
#include <lttoolbox/compression.h>
#include <lttoolbox/lttoolbox_config.h>
#include <lttoolbox/my_stdio.h>

TransExe::TransExe() :
initial_id(0)
{
}

TransExe::~TransExe()
{
  destroy();
}

TransExe::TransExe(TransExe const &te)
{
  copy(te);
}

TransExe &
TransExe::operator =(TransExe const &te)
{
  if(this != &te)
  {
    destroy();
    copy(te);
  }
  return *this;
}

void
TransExe::copy(TransExe const &te)
{
  initial_id = te.initial_id;
  node_list = te.node_list;
  finals = te.finals;
}

void
TransExe::destroy()
{
}

#include <iostream>

void
TransExe::read(FILE *input, Alphabet const &alphabet)
{
  TransExe &new_t = *this;
  new_t.destroy();
  new_t.initial_id = Compression::multibyte_read(input);
  int finals_size = Compression::multibyte_read(input);

  int base = 0;

  set<int> myfinals;


  while(finals_size > 0)
  {
    finals_size--;

    base += Compression::multibyte_read(input);
    myfinals.insert(base);
  }
  

  base = Compression::multibyte_read(input);

  int number_of_states = base;
  int current_state = 0;   
  new_t.node_list.resize(number_of_states);

  for(set<int>::iterator it = myfinals.begin(), limit = myfinals.end(); 
      it != limit; it++)
  {
    new_t.finals.insert(&new_t.node_list[*it]);
  }

  while(number_of_states > 0)
  {
    int number_of_local_transitions = Compression::multibyte_read(input);
    int tagbase = 0;
    Node &mynode = new_t.node_list[current_state];

    while(number_of_local_transitions > 0)
    {
      number_of_local_transitions--;
      tagbase += Compression::multibyte_read(input);
      int state = (current_state + Compression::multibyte_read(input)) % base;
      int i_symbol = alphabet.decode(tagbase).first;
      int o_symbol = alphabet.decode(tagbase).second;
      
      mynode.addTransition(i_symbol, o_symbol, &new_t.node_list[state]);
    }   
    number_of_states--;
    current_state++;
  }
}

void
TransExe::unifyFinals()
{
  node_list.resize(node_list.size()+1);

  Node *newfinal = &node_list[node_list.size()-1];

  for(set<Node *>::iterator it = finals.begin(), limit = finals.end(); 
      it != limit; it++)
  {
    (*it)->addTransition(0, 0, newfinal);
  }
  
  finals.clear();
  finals.insert(newfinal);
}

Node *
TransExe::getInitial()
{
  return &node_list[initial_id];
}

set<Node *> &
TransExe::getFinals()
{
  return finals;
}
