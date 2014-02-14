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
#include <lttoolbox/transducer.h>
#include <lttoolbox/compression.h>
#include <lttoolbox/alphabet.h>
#include <lttoolbox/lttoolbox_config.h>
#include <lttoolbox/my_stdio.h>

#include <cstdlib>
#include <iostream>
#include <vector>


int
Transducer::newState()
{
  int nstate = transitions.size();

  while(transitions.find(nstate) != transitions.end())
  {
    nstate++;
  }  
  transitions[nstate].clear();  // force creating
  
  return nstate;
}

Transducer::Transducer()
{
  initial = newState();
}

Transducer::~Transducer()
{
  destroy();
}

Transducer::Transducer(Transducer const &t)
{
  copy(t);
}

Transducer &
Transducer::operator =(Transducer const &t)
{
  if(this != &t)
  {
    destroy();
    copy(t);
  }
  return *this;
}

int
Transducer::insertSingleTransduction(int const tag, int const source)
{
  if(transitions.find(source) != transitions.end())
  {
    if(transitions[source].count(tag) == 1)
    {
      pair<multimap<int,int>::iterator, multimap<int,int>::iterator > range;
      range = transitions[source].equal_range(tag);
      return range.first->second;
    }
    else if(transitions[source].count(tag) == 0)
    {
      // new state
      int state = newState();
      transitions[source].insert(pair<int, int>(tag, state));
      return state;
    }
    else if(transitions[source].count(tag) == 2)
    {
      // there's a local cycle, must be ignored and treated like in '1'
      pair<multimap<int,int>::iterator, multimap<int,int>::iterator> range;
      range = transitions[source].equal_range(tag);
      for(; range.first != range.second; range.first++)
      {
        if(range.first->second != source)
        {
          return range.first->second;
        }
      }
      return -1; 
    } 
    else
    {
      return -1;
    }
  }
  else
  {
    return -1;
  }
}

int
Transducer::insertNewSingleTransduction(int const tag, int const source)
{
  int state = newState();
  transitions[source].insert(pair<int, int>(tag, state));
  return state;
}

int
Transducer::insertTransducer(int const source, Transducer &t, 
                                 int const epsilon_tag)
{
  map<int, int> relacion;

  t.joinFinals(epsilon_tag);
  
  for(map<int, multimap<int, int> >::const_iterator it = t.transitions.begin(),
                                                    limit = t.transitions.end();
      it != limit; it++)
  {
    relacion[it->first] = newState();
  }

  for(map<int, multimap<int, int> >::const_iterator it = t.transitions.begin();
      it != t.transitions.end(); it++)
  {
    for(multimap<int, int>::const_iterator it2 = it->second.begin(),
                                           limit2 = (it->second).end(); 
        it2 != limit2; it2++)
    {
      transitions[relacion[it->first]].insert(pair<int, int>(it2->first, relacion[it2->second]));
    }
  }

  transitions[source].insert(pair<int, int>(epsilon_tag, 
					     relacion[t.initial]));

  return relacion[*(t.finals.begin())];
}

void
Transducer::linkStates(int const source, int const destino, 
			    int const etiqueta)
{

  if(transitions.find(source) != transitions.end() &&
     transitions.find(destino) != transitions.end())
  {
    // new code
    pair<multimap<int, int>::iterator, multimap<int, int>::iterator> range;
    range = transitions[source].equal_range(etiqueta);
    for(;range.first != range.second; range.first++)
    {
      if(range.first->first == etiqueta && range.first->second == destino)
      {
        return;
      }
    }
    // end of new code
    transitions[source].insert(pair<int, int>(etiqueta, destino));
  }
  else
  {
    wcerr << L"Error: Trying to link nonexistent states (" << source;
    wcerr << L", " << destino << L", " << etiqueta << L")" << endl;
    exit(EXIT_FAILURE);
  }
}

bool
Transducer::isFinal(int const state) const
{
  return finals.find(state) != finals.end();
}

void
Transducer::setFinal(int const state, bool valor)
{
  int initial_copy = getInitial();
/*
  if(state == initial_copy)
  {
    wcerr << L"Setting initial state to final" << endl;
  }
*/
  if(valor)
  {
    finals.insert(state);
  }
  else
  {
    finals.erase(state);
  }
}

int
Transducer::getInitial() const
{
  return initial;
}

set<int>
Transducer::closure(int const state, int const epsilon_tag)
{
  set<int> nonvisited, result;
  
  nonvisited.insert(state);
  result.insert(state);

  while(nonvisited.size() > 0)
  {
    int auxest = *nonvisited.begin();
    pair<multimap<int, int>::iterator, multimap<int, int>::iterator> rango;
    rango = transitions[auxest].equal_range(epsilon_tag);
    while(rango.first != rango.second)
    {
      if(result.find(rango.first->second) == result.end())
      {
        result.insert(rango.first->second);
        nonvisited.insert(rango.first->second);
      }
      rango.first++;
    }
    nonvisited.erase(auxest);
  }

  return result;
}

void
Transducer::joinFinals(int const epsilon_tag)
{
  if(finals.size() > 1)
  {
    int state = newState();

    for(set<int>::iterator it = finals.begin(), limit = finals.end(); 
        it != limit; it++)
    {
      linkStates(*it, state, epsilon_tag);
    } 

    finals.clear();
    finals.insert(state); 
  }
  else if(finals.size() == 0)
  {
    wcerr << L"Error: empty set of final states" <<endl;
    exit(EXIT_FAILURE);
  }
}

bool
Transducer::isEmptyIntersection(set<int> const &s1, set<int> const &s2)
{

  if(s1.size() < s2.size())
  {
    for(set<int>::const_iterator it = s1.begin(), limit = s1.end(); it != limit; it++)
    {
      if(s2.find(*it) != s2.end())
      {
	return false;
      }
    }    
  }
  else
  {
    for(set<int>::const_iterator it = s2.begin(), limit = s2.end(); it != limit; it++)
    {
      if(s1.find(*it) != s1.end())
      {
        return false;
      }
    }
  }

  return true;
}

void
Transducer::determinize(int const epsilon_tag)
{
  vector<set<int> > R(2);
  map<int, set<int> > Q_prima;
  map<set<int>, int> Q_prima_inv;

  map<int, multimap<int, int> > transitions_prima;

  unsigned int talla_Q_prima = 0;
  Q_prima[0] = closure(initial, epsilon_tag);

  Q_prima_inv[Q_prima[0]] = 0;
  R[0].insert(0);

  int initial_prima = 0;
  set<int> finals_prima;

  if(finals.find(initial) != finals.end())
  {
    finals_prima.insert(0);
  }
 
  int t = 0;

  while(talla_Q_prima != Q_prima.size())
  {
    talla_Q_prima = Q_prima.size();
    R[(t+1)%2].clear();
    
    for(set<int>::iterator it = R[t].begin(), limit = R[t].end(); 
        it != limit; it++)
    {
      if(!isEmptyIntersection(Q_prima[*it], finals))
      {
	finals_prima.insert(*it);
      }

      map<int, set<int> > mymap;      

      for(set<int>::iterator it2 = Q_prima[*it].begin(), 
                             limit2 = Q_prima[*it].end();
	  it2 != limit2; it2++)
      {
        for(multimap<int, int>::iterator it3 = transitions[*it2].begin(),
                                         limit3 = transitions[*it2].end();
	    it3 != limit3; it3++)
	{
	  if(it3->first != epsilon_tag)
	  {
	    set<int> c = closure(it3->second, epsilon_tag);
           
	    for(set<int>::iterator it4 = c.begin(), limit4 = c.end(); 
	        it4 != limit4; it4++)
	    {
	      mymap[it3->first].insert(*it4);
	    }
	  }
	}
      }

      // adding new states  
      for(map<int, set<int> >::iterator it2 = mymap.begin(), limit2 = mymap.end(); 
	  it2 != limit2; it2++)
      {   
	if(Q_prima_inv.find(it2->second) == Q_prima_inv.end())
	{
	  int etiq = Q_prima.size();
	  Q_prima[etiq] = it2->second;
	  Q_prima_inv[it2->second] = etiq;
	  R[(t+1)%2].insert(Q_prima_inv[it2->second]);
          transitions_prima[etiq].clear();          
	}
        transitions_prima[*it].insert(pair<int, int>(it2->first, Q_prima_inv[it2->second]));
      }
    } 

    t = (t+1)%2;
  }

  transitions = transitions_prima;
  finals = finals_prima;
  initial = initial_prima;
}


void
Transducer::minimize(int const epsilon_tag)
{
  reverse(epsilon_tag);
  determinize(epsilon_tag);
  reverse(epsilon_tag);
  determinize(epsilon_tag);
}

void
Transducer::optional(int const epsilon_tag)
{
  joinFinals(epsilon_tag);
  int state = newState();  
  linkStates(state, initial, epsilon_tag);
  initial = state;

  state = newState();
  linkStates(*finals.begin(), state, epsilon_tag);
  finals.clear();
  finals.insert(state);
  linkStates(initial, state, epsilon_tag);
}

void
Transducer::oneOrMore(int const epsilon_tag)
{
  joinFinals(epsilon_tag);
  int state = newState();
  linkStates(state, initial, epsilon_tag);
  initial = state;

  state = newState();
  linkStates(*finals.begin(), state, epsilon_tag);
  finals.clear();
  finals.insert(state);
  linkStates(state, initial, epsilon_tag);
}

void
Transducer::zeroOrMore(int const epsilon_tag)
{
  oneOrMore(epsilon_tag);
  optional(epsilon_tag);
}

void
Transducer::clear()
{
  finals.clear();
  transitions.clear();
  initial = newState();
}

bool
Transducer::isEmpty() const
{
  return finals.size() == 0 && transitions.size() == 1;
}

int
Transducer::size() const
{
  return transitions.size();
}

int
Transducer::numberOfTransitions() const
{
  int counter = 0;

  for(map<int, multimap<int, int> >::const_iterator it = transitions.begin(),
                                                    limit = transitions.end();
      it != limit; it++)
  {
    counter += (it->second).size();
  }

  return counter;
}

bool
Transducer::isEmpty(int const state) const
{
  map<int, multimap<int, int> >::const_iterator it;

  it = transitions.find(state);
  if(it != transitions.end())
  {
    if((it->second).size() > 0)
    {
      return false;
    }
  }

  return true;
}

void
Transducer::write(FILE *output, int const decalage)
{
  Compression::multibyte_write(initial, output);
  Compression::multibyte_write(finals.size(), output);

  int base = 0;
  for(set<int>::iterator it = finals.begin(), limit = finals.end(); 
      it != limit; it++)
  {
    Compression::multibyte_write(*it - base, output);
    base = *it;
  }

  base = transitions.size();
  Compression::multibyte_write(base, output);
  for(map<int, multimap<int, int> >::iterator it = transitions.begin(),
                                              limit = transitions.end();
      it != limit; it++)
  {
    Compression::multibyte_write(it->second.size(), output);
    int tagbase = 0;
    for(multimap<int, int>::iterator it2 = it->second.begin(),
                                     limit2 = it->second.end();
        it2 != limit2; it2++)
    {
      Compression::multibyte_write(it2->first-tagbase+decalage, output);
      tagbase = it2->first; 

      if(it2->second >= it->first)
      {
        Compression::multibyte_write(it2->second-it->first, output);
      }
      else
      {
        Compression::multibyte_write(it2->second+base-it->first, output);
      }
    }
  }
}

void
Transducer::read(FILE *input, int const decalage)
{
  Transducer new_t;

  new_t.initial = Compression::multibyte_read(input);
  int finals_size = Compression::multibyte_read(input);

  int base = 0;
  while(finals_size > 0)
  {
    finals_size--;

    base += Compression::multibyte_read(input);
    new_t.finals.insert(base);
  }

  base = Compression::multibyte_read(input);
  int number_of_states = base;
  int current_state = 0; 
  while(number_of_states > 0)
  {
    int number_of_local_transitions = Compression::multibyte_read(input);
    int tagbase = 0;
    while(number_of_local_transitions > 0)
    {
      number_of_local_transitions--;
      tagbase += Compression::multibyte_read(input) - decalage;
      int state = (current_state + Compression::multibyte_read(input)) % base;
      if(new_t.transitions.find(state) == new_t.transitions.end())
      {
        new_t.transitions[state].clear(); // force create
      }
      new_t.transitions[current_state].insert(pair<int, int>(tagbase, state));
    }    
    number_of_states--;
    current_state++;
  }

  *this = new_t;
}

void
Transducer::copy(Transducer const &t)
{
  initial = t.initial;
  finals = t.finals;
  transitions = t.transitions;
}

void
Transducer::destroy()
{
}

void
Transducer::reverse(int const epsilon_tag)
{
  joinFinals(epsilon_tag);

  map<int, multimap<int, int> > temporal;
  
  for(map<int, multimap<int, int> >::reverse_iterator it = transitions.rbegin(); it != transitions.rend(); it++)
  {
    multimap<int, int> aux = it->second;
    it->second.clear();
    for(multimap<int, int>::iterator it2 = aux.begin(), limit2 = aux.end(); 
        it2 != limit2; it2++)
    {
      if(it2->second >= it->first)
      {
        transitions[it2->second].insert(pair<int, int>(it2->first, it->first));
      }
      else
      {
        temporal[it2->second].insert(pair<int, int>(it2->first, it->first));
      }
    }
    if(temporal.find(it->first) != temporal.end())
    {
      (it->second).insert(temporal[it->first].begin(), temporal[it->first].end());
      temporal.erase(it->first);
    } 
  }

  for(map<int, multimap<int, int> >::reverse_iterator it = temporal.rbegin(), 
                                                      limit = temporal.rend(); 
      it != limit; it++)
  {
    for(multimap<int, int>::iterator it2 = it->second.begin(),
                                     limit2 = it->second.end();
	it2 != limit2; it2++)
    {
      transitions[it->first].insert(pair<int, int>(it2->first, it2->second));
    }
  } 
  
  int tmp = initial;
  initial = *(finals.begin());
  finals.clear();
  finals.insert(tmp);
}

void
Transducer::show(Alphabet const &alphabet, FILE *output, int const epsilon_tag) const
{
  map<int, multimap<int, int> > temporal;

  for(map<int, multimap<int, int> >::const_iterator it = transitions.begin(); it != transitions.end(); it++)
  {
    multimap<int, int> aux = it->second;
  
    for(multimap<int, int>::const_iterator it2 = aux.begin(); it2 != aux.end(); it2++) 
    {
      pair<int, int> t = alphabet.decode(it2->first);
      fwprintf(output, L"%d\t", it->first);
      fwprintf(output, L"%d\t", it2->second);
      wstring l = L"";
      alphabet.getSymbol(l, t.first);
      if(l == L"")  // If we find an epsilon
      {
        fwprintf(output, L"ε\t", l.c_str());
      }
      else 
      {
        fwprintf(output, L"%S\t", l.c_str());
      }
      wstring r = L"";
      alphabet.getSymbol(r, t.second);
      if(r == L"")  // If we find an epsilon
      {
        fwprintf(output, L"ε\t", r.c_str());
      }
      else 
      {
        fwprintf(output, L"%S\t", r.c_str());
      }
      fwprintf(output, L"\n");
    } 
  } 

  for(set<int>::const_iterator it3 = finals.begin(); it3 != finals.end(); it3++)
  {
    fwprintf(output, L"%d\n", *it3);
  }
}

int 
Transducer::getStateSize(int const state)
{
 set<int> states;
 set<int> myclosure1 = closure(state, 0);
 states.insert(myclosure1.begin(), myclosure1.end());
 int num_transitions = 0;

 for(set<int>::iterator it2 = states.begin(); it2 != states.end(); it2++)
 {
   num_transitions += transitions[*it2].size();
 }

 return num_transitions;
}

bool
Transducer::recognise(wstring patro, Alphabet &a, FILE *err)
{
  bool accepted = false;
  set<int> states ;

  set<int> myclosure1 = closure(getInitial(), 0); 
  states.insert(myclosure1.begin(), myclosure1.end()); 
  // For each of the characters in the input string
  for(wstring::iterator it = patro.begin(); it != patro.end(); it++)  
  {
    set<int> new_state;        //Transducer::closure(int const state, int const epsilon_tag)
    int sym = *it;
    // For each of the current alive states
    //fwprintf(err, L"step: %S %C (%d)\n", patro.c_str(), *it, sym);
    for(set<int>::iterator it2 = states.begin(); it2 != states.end(); it2++)
    {
      multimap<int, int> p = transitions[*it2];
      // For each of the transitions in the state 

      for(multimap<int, int>::iterator it3 = p.begin(); it3 != p.end(); it3++)
      { 
        
	pair<int, int> t = a.decode(it3->first);
        wstring l = L"";
        a.getSymbol(l, t.first);
        //wstring r = L"";
        //a.getSymbol(r, t.second);

        //fwprintf(err, L"  -> state: %d, trans: %S:%S, targ: %d\n", *it2, (l == L"") ?  L"ε" : l.c_str(),  (r == L"") ?  L"ε" : r.c_str(), it3->second);
        //if(l.find(*it) != wstring::npos || l == L"" )
        if(l.find(*it) != wstring::npos)
        {
          set<int> myclosure = closure(it3->second, 0);
          //wcerr << L"Before closure alives: " <<new_state.size() << endl;  
          new_state.insert(myclosure.begin(), myclosure.end());
          //wcerr << L"After closure alives: " <<new_state.size() << endl;  
        }
      }
    }
    states = new_state;
  }
  for(set<int>::iterator it4 = states.begin(); it4 != states.end(); it4++)
  {
    if(isFinal(*it4)) 
    {
      accepted = true;
    }
  }

  return accepted;
}

void
Transducer::unionWith(Transducer &t, 
                      int const epsilon_tag)
{
  insertTransducer(initial, t, epsilon_tag);
}

Transducer
Transducer::appendDotStar(const set<int> &loopback_symbols,
  const int epsilon_tag)
{
  Transducer prefix_transducer(*this);

  for(set<int>::iterator prefix_it = prefix_transducer.finals.begin(),
                           prefix_limit = prefix_transducer.finals.end();
      prefix_it != prefix_limit;
      prefix_it++)
  {
    for(set<int>::iterator loopback_it = loopback_symbols.begin(),
                           loopback_limit = loopback_symbols.end();
    loopback_it != loopback_limit;
    loopback_it++)
    {
      if((*loopback_it) != epsilon_tag)
      {
        /* link the final state of the prefix transducer to itself with the
         * symbol of this class
         */
        prefix_transducer.linkStates(*prefix_it, *prefix_it, *loopback_it);
      }
    }
  }

  return prefix_transducer;
}

/* Why are the arguments called by reference? None of them are modified. Is this
 * an attempt to save memory?
 */
Transducer
Transducer::intersect(Transducer &t, Alphabet &my_a, Alphabet &t_a)
{
  // map of the states of the multiplied and trimmed transducers
  map<pair<int, int>, int> states_multiplied_trimmed;
  // trimmed transducer
  Transducer trimmed_t;
  // destroy the initial state of the trimmed transducer
  trimmed_t.transitions.clear();
  
  for(map<int, multimap<int, int> >::iterator it = transitions.begin(),
                                              limit = transitions.end();
    it != limit;
    it++)
  {
    for(map<int, multimap<int, int> >::iterator t_it = t.transitions.begin(),
                                                t_limit = t.transitions.end();
      t_it != t_limit;
      t_it++)
    {
      // state of the multiplied automaton
      pair<int, int> tmp(it->first, t_it->first);
      /* map the state of the multiplied automaton with a new state of the
       * trimmed transducer
       */
      states_multiplied_trimmed.insert(make_pair(tmp, trimmed_t.newState()));
    }
  }

  for(map<int, multimap<int, int> >::iterator state_it = transitions.begin(),
                                              state_limit = transitions.end();
    state_it != state_limit;
    state_it++)
  {
    for(map<int, multimap<int, int> >::iterator t_state_it
        = t.transitions.begin(),
                                                t_state_limit
        = t.transitions.end();
      t_state_it != t_state_limit;
      t_state_it++)
    {
      for(multimap<int, int>::iterator transition_it = state_it->second.begin(),
                                       transition_limit
          = state_it->second.end();
        transition_it != transition_limit;
        transition_it++)
      {
        for(multimap<int, int>::iterator t_transition_it
            = t_state_it->second.begin(),
                                         t_transition_limit
            = t_state_it->second.end();
          t_transition_it != t_transition_limit;
          t_transition_it++)
        {
          wstring left_monolingual = L"",
                  right_monolingual = L"",
                  left_bilingual = L"";
          my_a.getSymbol(left_monolingual,
            my_a.decode(transition_it->first).first);
          my_a.getSymbol(right_monolingual,
            my_a.decode(transition_it->first).second);
          t_a.getSymbol(left_bilingual,
            t_a.decode(t_transition_it->first).first);
          /* check if the input tag of this class is equal to the output tag of
           * the transducer t
           */
          if(right_monolingual == left_bilingual)
          {
            // source state of the multiplied automaton
            pair<int, int> multiplied_source(state_it->first,
              t_state_it->first);
            // target state of the multiplied automaton
            pair<int, int> multiplied_target(transition_it->second,
              t_transition_it->second);
            wcerr << L"linking (("
                  << multiplied_source.first
                  << L", "
                  << multiplied_source.second
                  << L") -> "
                  << states_multiplied_trimmed[multiplied_source]
                  << L") to (("
                  << multiplied_target.first
                  << L", "
                  << multiplied_target.second
                  << L") -> "
                  << states_multiplied_trimmed[multiplied_target]
                  << L") with ("
                  << left_monolingual
                  << L", "
                  << right_monolingual
                  << L")"<<endl;
            /* link the source and target states of the trimmed transducer with
             * the symbol of this class
             */
            trimmed_t.linkStates(states_multiplied_trimmed[multiplied_source],
              states_multiplied_trimmed[multiplied_target],
              transition_it->first);
          }
          else
          {
            // source state of the multiplied automaton
            pair<int, int> multiplied_source(state_it->first,
              t_state_it->first);
            // target state of the multiplied automaton
            pair<int, int> multiplied_target(transition_it->second,
              t_transition_it->second);
            wstring right_bilingual = L"";
            t_a.getSymbol(right_bilingual,
              t_a.decode(t_transition_it->first).second);
            wcerr << L"not linking (("
              << multiplied_source.first
              << L", "
              << multiplied_source.second
              << L") -> "
              << states_multiplied_trimmed[multiplied_source]
              << L") to (("
              << multiplied_target.first
              << L", "
              << multiplied_target.second
              << L") -> "
              << states_multiplied_trimmed[multiplied_target]
              << L") with ("
              << left_monolingual
              << L", "
              << right_monolingual
              << L") because the symbol ("
              << left_monolingual
              << L", "
              << right_monolingual
              << L") cannot be multiplied with ("
              << left_bilingual
              << L", "
              << right_bilingual
              << L")"<<endl;
            continue;
          }
        }
      }
    }
  }

  for(set<int>::iterator it = finals.begin(),
                         limit = finals.end();
    it != limit;
    it++)
  {
    for(set<int>::iterator t_it = t.finals.begin(),
                           t_limit = t.finals.end();
      t_it != t_limit;
      t_it++)
    {
      // final state of the multiplied automaton
      pair<int, int> tmp(*it, *t_it);
      // insert the final state of the trimmed transducer
      trimmed_t.finals.insert(states_multiplied_trimmed[tmp]);
    }
  }

  // initial state of the multiplied automaton
  pair<int, int> tmp(initial, t.initial);
  // set the initial state of the trimmed transducer
  trimmed_t.initial = states_multiplied_trimmed[tmp];
  wcerr << L"initial state: " << trimmed_t.getInitial()<<endl;
  trimmed_t.show(my_a);
  trimmed_t.wideConsoleErrorFinals();
  wcerr << L"trimmed_t.minimize();";
  cin.ignore();
  // minimize the trimmed transducer
  trimmed_t.minimize();

  return trimmed_t;
}

void
Transducer::wideConsoleErrorFinals()
{
  wcerr << L"set of " << finals.size() << L" final states"<<endl;
  wcerr << L"{"<<endl;
  for(set<int>::iterator it = finals.begin(),
                         limit = finals.end();
    it != limit;
    it++)
  {
    wcerr << *it<<endl;
  }
  wcerr << L"}"<<endl;
}
