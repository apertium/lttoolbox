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

TransExe::TransExe():
initial_id(0),
default_weight(0.0000)
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
  default_weight = te.default_weight;
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
  bool read_weights = false;

  fpos_t pos;
  if (fgetpos(input, &pos) == 0) {
      char header[4]{};
      fread(header, 1, 4, input);
      if (strncmp(header, HEADER_TRANSDUCER, 4) == 0) {
          auto features = read_le<uint64_t>(input);
          if (features >= TDF_UNKNOWN) {
              throw std::runtime_error("Transducer has features that are unknown to this version of lttoolbox - upgrade!");
          }
          read_weights = (features & TDF_WEIGHTS);
      }
      else {
          // Old binary format
          fsetpos(input, &pos);
      }
  }

  TransExe &new_t = *this;
  new_t.destroy();
  new_t.initial_id = Compression::multibyte_read(input);
  int finals_size = Compression::multibyte_read(input);

  int base = 0;
  double base_weight = default_weight;

  map<int, double> myfinals;

  while(finals_size > 0)
  {
    finals_size--;

    base += Compression::multibyte_read(input);
    if(read_weights)
    {
      base_weight = Compression::long_multibyte_read(input);
    }
    myfinals.insert(make_pair(base, base_weight));
  }


  base = Compression::multibyte_read(input);

  int number_of_states = base;
  int current_state = 0;
  new_t.node_list.resize(number_of_states);

  for(map<int, double>::iterator it = myfinals.begin(), limit = myfinals.end();
      it != limit; it++)
  {
    new_t.finals.insert(make_pair(&new_t.node_list[it->first], it->second));
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
      if(read_weights)
      {
        base_weight = Compression::long_multibyte_read(input);
      }
      int i_symbol = alphabet.decode(tagbase).first;
      int o_symbol = alphabet.decode(tagbase).second;

      mynode.addTransition(i_symbol, o_symbol, &new_t.node_list[state], base_weight);
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

  for(auto& it : finals)
  {
    it.first->addTransition(0, 0, newfinal, it.second);
  }

  finals.clear();
  finals.insert(make_pair(newfinal, default_weight));
}

Node *
TransExe::getInitial()
{
  return &node_list[initial_id];
}

map<Node *, double> &
TransExe::getFinals()
{
  return finals;
}
