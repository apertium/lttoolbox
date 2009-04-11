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
#include <lttoolbox/match_state.h>
#include <lttoolbox/pattern_list.h>

#include <climits>
#include <cstring>

int const MatchState::BUF_LIMIT = 1024;

MatchState::MatchState()
{
  first = last = 0;
  state = new MatchNode *[BUF_LIMIT];
}

MatchState::~MatchState()
{
  destroy();
}

MatchState::MatchState(MatchState const &s)
{
  copy(s);
}

MatchState &
MatchState::operator =(MatchState const &s)
{
  if(this != &s)
  {
    destroy();
    copy(s);
  }

  return *this;
}

void 
MatchState::destroy()
{
  delete[] state;
}

void
MatchState::copy(MatchState const &s)
{
  for(int i = 0; i < BUF_LIMIT; i++)
  {
    state[i] = s.state[i];
  }
  first = s.first;
  last = s.last;
}

int 
MatchState::size() const
{
  return last >= first ? last - first: last + BUF_LIMIT -first;
}

void
MatchState::init(MatchNode *initial)
{
  first = 0;
  last = 1;
  state[0] = initial;
} 

void
MatchState::applySymbol(MatchNode *pnode, int const symbol)
{
  MatchNode *aux = pnode->transitions.search(symbol);
  if(aux != NULL)
  {
    state[last] = aux;
    last = (last + 1)%BUF_LIMIT;
  }
}

void
MatchState::step(int const input)
{
  int mylast = last;
  for(int i = first; i != mylast; i=(i+1)%BUF_LIMIT)
  {
    applySymbol(state[i], input);
  }
  first = mylast;
}

void
MatchState::step(int const input, int const alt)
{
  int mylast = last;
  for(int i = first; i != mylast; i=(i+1)%BUF_LIMIT)
  {
    applySymbol(state[i], input);
    applySymbol(state[i], alt);
  }
  first = mylast;
}

int
MatchState::classifyFinals(map<MatchNode *, int> const &final_class) const
{
  int result = INT_MAX;
  for (int i = first; i != last; i = (i+1)%BUF_LIMIT)
  {
    map<MatchNode*, int>::const_iterator it2 = final_class.find(state[i]);
    if(it2 != final_class.end())
    {
      if(it2->second < result)
      {
        result = it2->second;
      }
    }
  }
  return (result < INT_MAX)? result : (-1);
}

void
MatchState::clear()
{
  first = last = 0;
}
