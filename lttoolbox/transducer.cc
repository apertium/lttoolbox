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
#include <lttoolbox/transducer.h>
#include <lttoolbox/compression.h>
#include <lttoolbox/alphabet.h>
#include <lttoolbox/lttoolbox_config.h>
#include <lttoolbox/my_stdio.h>
#include <lttoolbox/deserialiser.h>
#include <lttoolbox/serialiser.h>

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

Transducer::Transducer():
default_weight(0.0000)
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
Transducer::insertSingleTransduction(int const tag, int const source, double const weight)
{
  if(transitions.find(source) != transitions.end())
  {
    if(transitions[source].count(tag) == 1)
    {
      pair<multimap<int,pair<int, double> >::iterator, multimap<int,pair<int, double> >::iterator > range;
      range = transitions[source].equal_range(tag);
      return range.first->second.first;
    }
    else if(transitions[source].count(tag) == 0)
    {
      // new state
      int state = newState();
      transitions[source].insert(pair<int, pair<int, double> >(tag, make_pair(state, weight)));
      return state;
    }
    else if(transitions[source].count(tag) == 2)
    {
      // there's a local cycle, must be ignored and treated like in '1'
      pair<multimap<int,pair<int, double>>::iterator, multimap<int,pair<int, double>>::iterator> range;
      range = transitions[source].equal_range(tag);
      for(; range.first != range.second; range.first++)
      {
        if(range.first->second.first != source)
        {
          return range.first->second.first;
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
Transducer::insertNewSingleTransduction(int const tag, int const source, double const weight)
{
  int state = newState();
  transitions[source].insert(pair<int, pair<int, double> >(tag, make_pair(state, weight)));
  return state;
}

int
Transducer::insertTransducer(int const source, Transducer &t,
                            int const epsilon_tag)
{
  map<int, int> relation;

  t.joinFinals(epsilon_tag);

  for(map<int, multimap<int, pair<int, double> > >::const_iterator it = t.transitions.begin(),
                                                                  limit = t.transitions.end();
      it != limit; it++)
  {
    relation[it->first] = newState();
  }

  for(map<int, multimap<int, pair<int, double> > >::const_iterator it = t.transitions.begin();
      it != t.transitions.end(); it++)
  {
    for(multimap<int, pair<int, double> >::const_iterator it2 = it->second.begin(),
                                                        limit2 = (it->second).end();
        it2 != limit2; it2++)
    {
      transitions[relation[it->first]].insert(pair<int, pair<int, double> >(it2->first,
                                                                            make_pair(relation[it2->second.first],
                                                                            it2->second.second)));
    }
  }

  transitions[source].insert(pair<int, pair<int, double> >(epsilon_tag,
                                                          make_pair(relation[t.initial], default_weight)));

  return relation[t.finals.begin()->first];
}

void
Transducer::linkStates(int const source, int const target,
                       int const tag, double const weight)
{

  if(transitions.find(source) != transitions.end() &&
     transitions.find(target) != transitions.end())
  {
    // new code
    pair<multimap<int, pair<int, double> >::iterator, multimap<int, pair<int, double> >::iterator> range;
    range = transitions[source].equal_range(tag);
    for(;range.first != range.second; range.first++)
    {
      if(range.first->first == tag && range.first->second.first == target)
      {
        return;
      }
    }
    // end of new code
    transitions[source].insert(pair<int, pair<int, double> >(tag, make_pair(target, weight)));
  }
  else
  {
    wcerr << L"Error: Trying to link nonexistent states (" << source;
    wcerr << L", " << target << L", " << tag << L")" << endl;
    exit(EXIT_FAILURE);
  }
}

bool
Transducer::isFinal(int const state) const
{
  return finals.find(state) != finals.end();
}

void
Transducer::setFinal(int const state, double const weight, bool value)
{
/*
  int initial_copy = getInitial();
  if(state == initial_copy)
  {
    wcerr << L"Setting initial state to final" << endl;
  }
*/
  if(value)
  {
    finals.insert(pair <int, double>(state, weight));
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
    pair<multimap<int, pair<int, double> >::iterator, multimap<int, pair<int, double> >::iterator> range;
    range = transitions[auxest].equal_range(epsilon_tag);
    while(range.first != range.second)
    {
      if(result.find(range.first->second.first) == result.end())
      {
        result.insert(range.first->second.first);
        nonvisited.insert(range.first->second.first);
      }
      range.first++;
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

    for(map<int, double>::iterator it = finals.begin(), limit = finals.end();
        it != limit; it++)
    {
      linkStates(it->first, state, epsilon_tag, it->second);
    }

    finals.clear();
    finals.insert(pair <int, double>(state, default_weight));
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

  map<int, multimap<int, pair<int, double> > > transitions_prima;

  unsigned int talla_Q_prima = 0;
  Q_prima[0] = closure(initial, epsilon_tag);

  Q_prima_inv[Q_prima[0]] = 0;
  R[0].insert(0);

  int initial_prima = 0;
  map<int, double> finals_prima;

  if(isFinal(initial))
  {
    finals_prima.insert(pair<int, double>(0, default_weight));
  }

  int t = 0;

  while(talla_Q_prima != Q_prima.size())
  {
    talla_Q_prima = Q_prima.size();
    R[(t+1)%2].clear();

    for(set<int>::iterator it = R[t].begin(), limit = R[t].end();
        it != limit; it++)
    {
      set<int> finals_state;
      for(map<int, double>::const_iterator it2 = finals.begin(); it2 != finals.end(); it2++)
      {
        finals_state.insert(it2->first);
      }
      if(!isEmptyIntersection(Q_prima[*it], finals_state))
      {
        finals_prima.insert(pair<int, double>(*it, default_weight));
      }

      map<int, set<int> > mymap;

      for(set<int>::iterator it2 = Q_prima[*it].begin(),
                             limit2 = Q_prima[*it].end();
          it2 != limit2; it2++)
      {
        for(multimap<int, pair<int, double> >::iterator it3 = transitions[*it2].begin(),
                                                      limit3 = transitions[*it2].end();
            it3 != limit3; it3++)
        {
          if(it3->first != epsilon_tag)
          {
            set<int> c = closure(it3->second.first, epsilon_tag);

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
        transitions_prima[*it].insert(pair<int, pair<int, double> >(it2->first,
                                                                    make_pair(Q_prima_inv[it2->second],
                                                                              default_weight)));
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
  linkStates(state, initial, epsilon_tag, default_weight);
  initial = state;

  state = newState();
  linkStates(finals.begin()->first, state, epsilon_tag, finals.begin()->second);
  finals.clear();
  finals.insert(pair <int, double>(state, default_weight));
  linkStates(initial, state, epsilon_tag, default_weight);
}

void
Transducer::oneOrMore(int const epsilon_tag)
{
  joinFinals(epsilon_tag);
  int state = newState();
  linkStates(state, initial, epsilon_tag, default_weight);
  initial = state;

  state = newState();
  linkStates(finals.begin()->first, state, epsilon_tag, finals.begin()->second);
  finals.clear();
  finals.insert(pair <int, double>(state, default_weight));
  linkStates(state, initial, epsilon_tag, default_weight);
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

bool
Transducer::hasNoFinals() const
{
  return finals.size() == 0;
}

map<int, multimap<int, pair<int, double> > >&
Transducer::getTransitions()
{
  return transitions;
}

map<int, double>
Transducer::getFinals() const
{
  return map<int, double>(finals);
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

  for(map<int, multimap<int, pair<int, double> > >::const_iterator it = transitions.begin(),
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
  map<int, multimap<int, pair<int, double> > >::const_iterator it;

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
  double cost = default_weight;
  for(map<int, double>::iterator it = finals.begin(), limit = finals.end();
      it != limit; it++)
  {
    Compression::multibyte_write(it->first - base, output);
    base = it->first;
    Compression::long_multibyte_write(it->second, output);
  }

  base = transitions.size();
  Compression::multibyte_write(base, output);
  for(map<int, multimap<int, pair<int, double> > >::iterator it = transitions.begin(),
                                                             limit = transitions.end();
      it != limit; it++)
  {
    Compression::multibyte_write(it->second.size(), output);
    int tagbase = 0;
    double tagcost = default_weight;
    for(multimap<int, pair<int, double> >::iterator it2 = it->second.begin(),
                                                          limit2 = it->second.end();
        it2 != limit2; it2++)
    {
      Compression::multibyte_write(it2->first-tagbase+decalage, output);
      tagbase = it2->first;

      if(it2->second.first >= it->first)
      {
        Compression::multibyte_write(it2->second.first-it->first, output);
      }
      else
      {
        Compression::multibyte_write(it2->second.first+base-it->first, output);
      }
      Compression::long_multibyte_write(it2->second.second, output);
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
  double base_weight = default_weight;
  while(finals_size > 0)
  {
    finals_size--;

    base += Compression::multibyte_read(input);
    base_weight = Compression::long_multibyte_read(input);
    new_t.finals.insert(pair <int, double>(base, base_weight));
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
      base_weight = Compression::long_multibyte_read(input);
      if(new_t.transitions.find(state) == new_t.transitions.end())
      {
        new_t.transitions[state].clear(); // force create
      }
      new_t.transitions[current_state].insert(pair<int, pair<int, double> >(tagbase, make_pair(state, base_weight)));
    }
    number_of_states--;
    current_state++;
  }

  *this = new_t;
}

void
Transducer::serialise(std::ostream &serialised) const
{
  Serialiser<int>::serialise(initial, serialised);
  set<int> finals_state;
  for(map<int, double>::const_iterator it = finals.begin(); it != finals.end(); it++)
  {
    finals_state.insert(it->first);
  }
  Serialiser<set<int> >::serialise(finals_state, serialised);
  Serialiser<map<int, multimap<int, pair<int, double> > > >::serialise(transitions, serialised);
}

void
Transducer::deserialise(std::istream &serialised)
{
  initial = Deserialiser<int>::deserialise(serialised);
  for(set<int>::const_iterator D_it = Deserialiser<set<int> >::deserialise(serialised).begin();
      D_it != Deserialiser<set<int> >::deserialise(serialised).end(); D_it++)
  {
    finals.insert(pair <int, double>(*D_it, default_weight));
  }
  transitions = Deserialiser<map<int, multimap<int, pair<int, double> > > >::deserialise(serialised);
}

void
Transducer::copy(Transducer const &t)
{
  initial = t.initial;
  default_weight = t.default_weight;
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

  map<int, multimap<int, pair<int, double> > > tmp_transitions;

  for(map<int, multimap<int, pair<int, double> > >::reverse_iterator it = transitions.rbegin(); it != transitions.rend(); it++)
  {
    multimap<int, pair<int, double> > aux = it->second;
    it->second.clear();
    for(multimap<int, pair<int, double> >::iterator it2 = aux.begin(), limit2 = aux.end();
        it2 != limit2; it2++)
    {
      if(it2->second.first >= it->first)
      {
        transitions[it2->second.first].insert(pair<int, pair<int, double> >(it2->first, make_pair(it->first, it2->second.second)));
      }
      else
      {
        tmp_transitions[it2->second.first].insert(pair<int, pair<int, double> >(it2->first, make_pair(it->first, it2->second.second)));
      }
    }
    if(tmp_transitions.find(it->first) != tmp_transitions.end())
    {
      (it->second).insert(tmp_transitions[it->first].begin(), tmp_transitions[it->first].end());
      tmp_transitions.erase(it->first);
    }
  }

  for(map<int, multimap<int, pair<int, double> > >::reverse_iterator it = tmp_transitions.rbegin(),
                                                      limit = tmp_transitions.rend();
      it != limit; it++)
  {
    for(multimap<int, pair<int, double> >::iterator it2 = it->second.begin(),
                                     limit2 = it->second.end();
        it2 != limit2; it2++)
    {
      transitions[it->first].insert(pair<int, pair<int, double> >(it2->first, it2->second));
    }
  }

  int tmp = initial;
  initial = finals.begin()->first;
  finals.clear();
  finals.insert(pair <int, double>(tmp, default_weight));
}

void
Transducer::show(Alphabet const &alphabet, FILE *output, int const epsilon_tag) const
{
  map<int, multimap<int, pair<int, double> > > tmp_transitions;

  for(map<int, multimap<int, pair<int, double> > >::const_iterator it = transitions.begin(); it != transitions.end(); it++)
  {
    multimap<int, pair<int, double> > aux = it->second;

    for(multimap<int, pair<int, double> >::const_iterator it2 = aux.begin(); it2 != aux.end(); it2++)
    {
      pair<int, int> t = alphabet.decode(it2->first);
      fwprintf(output, L"%d\t", it->first);
      fwprintf(output, L"%d\t", it2->second.first);
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
      fwprintf(output, L"%f\t", it2->second.second);
      fwprintf(output, L"\n");
    }
  }

  for(map<int, double>::const_iterator it3 = finals.begin(); it3 != finals.end(); it3++)
  {
    fwprintf(output, L"%d\t", it3->first);
    fwprintf(output, L"%f\n", it3->second);
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
Transducer::recognise(wstring pattern, Alphabet &a, FILE *err)
{
  bool accepted = false;
  set<int> states;

  set<int> myclosure1 = closure(getInitial(), 0);
  states.insert(myclosure1.begin(), myclosure1.end());
  // For each of the characters in the input string
  for(wstring::iterator it = pattern.begin(); it != pattern.end(); it++)
  {
    set<int> new_state;        //Transducer::closure(int const state, int const epsilon_tag)
    int sym = *it;
    // For each of the current alive states
    //fwprintf(err, L"step: %S %C (%d)\n", pattern.c_str(), *it, sym);
    for(set<int>::iterator it2 = states.begin(); it2 != states.end(); it2++)
    {
      multimap<int, pair<int, double> > p = transitions[*it2];
      // For each of the transitions in the state

      for(multimap<int, pair<int, double> >::iterator it3 = p.begin(); it3 != p.end(); it3++)
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
          set<int> myclosure = closure(it3->second.first, 0);
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
Transducer::unionWith(Alphabet &my_a,
  Transducer &t,
  int const epsilon_tag)
{
  finals.insert(pair <int, double>(insertTransducer(initial, t, epsilon_tag), default_weight));
}

Transducer
Transducer::appendDotStar(set<int> const &loopback_symbols, int const epsilon_tag)
{
  Transducer prefix_transducer(*this);

  for(map<int, double>::iterator prefix_it = prefix_transducer.finals.begin(),
                                prefix_limit = prefix_transducer.finals.end();
      prefix_it != prefix_limit;
      prefix_it++)
  {
    for(set<int>::iterator loopback_it = loopback_symbols.begin(),
                           loopback_limit = loopback_symbols.end();
    loopback_it != loopback_limit;
    loopback_it++)
    {
      if((*loopback_it) != epsilon_tag) // TODO: Necessary? Minimization should remove epsilon loopbacks anyway
      {
        prefix_transducer.linkStates(prefix_it->first, prefix_it->first, *loopback_it, prefix_it->second);
      }
    }
  }

  return prefix_transducer;
}

Transducer
Transducer::copyWithTagsFirst(int start,
                              int group_label,
                              Alphabet const &alphabet,
                              int const epsilon_tag)
{
  Transducer new_t;
  Transducer lemq;

  map<int, int> states_this_new;
  states_this_new.insert(make_pair(start, new_t.initial));
  map<int, int> states_this_lemq;
  states_this_new.insert(make_pair(start, lemq.initial));

  typedef std::pair<int, int> SearchState;
  // Each searchstate in the stack is a transition in this FST, along
  // with the last reached state of the lemq
  std::list<SearchState> todo;
  std::set<SearchState> seen;
  std::set<SearchState> finally;
  SearchState current;
  todo.push_front(make_pair(start,start));

  while(todo.size() > 0) {
    current = todo.front();
    todo.pop_front();
    seen.insert(current);
    int this_src = current.first, this_lemqlast = current.second;

    for(multimap<int, pair<int, double> >::iterator trans_it = transitions[this_src].begin(),
                                                  trans_limit = transitions[this_src].end();
        trans_it != trans_limit;
        trans_it++)
    {
      int label = trans_it->first, this_trg = trans_it->second.first;
      double this_wt = trans_it->second.second;
      int left_symbol = alphabet.decode(label).first;

      // Anything after the first tag goes before the lemq, whether
      // epsilon or alphabetic (might be a hack to force trimming). If
      // the lemqlast state is already set to something other than
      // this_src, then we've seen the first tag (and are done reading
      // lemq).
      if(alphabet.isTag(left_symbol) || (this_src != this_lemqlast))
      {
        int new_src;
        if(this_src == this_lemqlast)
        {
          // We've reached the first tag
          new_src = states_this_new[start];
          lemq.finals.insert(pair <int, double>(this_lemqlast, default_weight));
        }
        else
        {
          if(states_this_new.find(this_src) == states_this_new.end())
          {
            states_this_new.insert(make_pair(this_src, new_t.newState()));
          }
          new_src = states_this_new[this_src];
        }

        if(states_this_new.find(this_trg) == states_this_new.end())
        {
          states_this_new.insert(make_pair(this_trg, new_t.newState()));
        }
        int new_trg = states_this_new[this_trg];
        new_t.linkStates(new_src, new_trg, label, default_weight);

        if(isFinal(this_src))
        {
          finally.insert(make_pair(this_src, this_lemqlast));
        }

        if(seen.find(make_pair(this_trg, this_lemqlast)) == seen.end())
        {
          todo.push_front(make_pair(this_trg, this_lemqlast));
        }
      }
      else
      {
        // We're still reading the lemq, append label to current one:
        int lemq_src = states_this_lemq[this_src];
        if(states_this_lemq.find(this_trg) == states_this_lemq.end())
        {
          states_this_lemq.insert(make_pair(this_trg, lemq.newState()));
        }
        int lemq_trg = states_this_lemq[this_trg];
        lemq.linkStates(lemq_src, lemq_trg, label, default_weight);
        if(seen.find(make_pair(this_trg, this_trg)) == seen.end())
        {
          todo.push_front(make_pair(this_trg, this_trg));
        }
      }
    } // end for transitions
  } // end while todo

  for(set<SearchState>::iterator it = finally.begin(), limit = finally.end();
      it != limit;
      it++)
  {
    int last_tag = it->first,
      this_lemqlast = it->second;
    // copy lemq, letting this_lemqlast be the only final state in newlemq
    Transducer newlemq = Transducer(lemq);
    newlemq.finals.clear();
    newlemq.finals.insert(pair <int, double>(states_this_lemq[this_lemqlast], default_weight));
    newlemq.minimize();

    int group_start = new_t.newState();
    new_t.linkStates(states_this_new[last_tag], group_start, group_label, default_weight);

    // append newlemq into the group after last_tag:
    new_t.finals.insert(pair <int, double>(
      new_t.insertTransducer(group_start,
                             newlemq),
      default_weight
      ));
  }

  return new_t;
}

Transducer
Transducer::moveLemqsLast(Alphabet const &alphabet,
                          int const epsilon_tag)
{
  // TODO: These should be in file which is included by both
  // fst_processor.cc and compiler.cc:
  wstring COMPILER_GROUP_ELEM = L"#";

  Transducer new_t;
  typedef int SearchState;
  std::set<SearchState> seen;
  std::list<SearchState> todo;
  todo.push_front(initial);

  map<int, int> states_this_new;
  states_this_new.insert(make_pair(initial, new_t.initial));

  while(todo.size() > 0)
  {
    int this_src = todo.front();
    todo.pop_front();
    seen.insert(this_src);
    for(multimap<int, pair<int, double> >::iterator trans_it = transitions[this_src].begin(),
                                                  trans_limit = transitions[this_src].end();
        trans_it != trans_limit;
        trans_it++)
    {
      int label = trans_it->first,
       this_trg = trans_it->second.first;
      double this_wt = trans_it->second.second;
      wstring left = L"";
      alphabet.getSymbol(left, alphabet.decode(label).first);
      int new_src = states_this_new[this_src];

      if(left == COMPILER_GROUP_ELEM)
      {
        Transducer tagsFirst = copyWithTagsFirst(this_trg, label, alphabet, epsilon_tag);
        new_t.finals.insert(pair <int, double>(
          new_t.insertTransducer(new_src, tagsFirst, epsilon_tag), default_weight
          ));
      }
      else
      {
        if(states_this_new.find(this_trg) == states_this_new.end())
        {
          states_this_new.insert(make_pair(this_trg, new_t.newState()));
        }
        int new_trg = states_this_new[this_trg];
        new_t.linkStates(new_src, new_trg, label, default_weight);
        if(seen.find(this_trg) == seen.end())
        {
          todo.push_front(this_trg);
        }
      }
    }
  }

  for(map<int, double>::iterator it = finals.begin(), limit = finals.end();
      it != limit;
      it++)
   {
     new_t.finals.insert(pair <int, double>(states_this_new[it->first], it->second));
   }

  return new_t;
}


Transducer
Transducer::intersect(Transducer &trimmer,
                      Alphabet const &this_a,
                      Alphabet const &trimmer_a,
                      int const epsilon_tag)
{
  joinFinals(epsilon_tag);
  /**
   * this ∩ trimmer = trimmed
   *
   * The trimmer is typically a bidix passed through appendDotStar.
   */

  // TODO: These should be in file which is included by both
  // fst_processor.cc and compiler.cc:
  wstring compoundOnlyLSymbol = L"<compound-only-L>";
  wstring compoundRSymbol = L"<compound-R>";
  wstring COMPILER_JOIN_ELEM = L"+";
  wstring COMPILER_GROUP_ELEM = L"#";

  // When searching, we need to record (this, (trimmer, trimmer_pre_plus))
  typedef std::pair<int, std::pair<int, int > > SearchState;
  // first: currently searched state in this;
  // second.first: currently matched trimmer state;
  // second.second: last matched trimmer state before a + restart (or the same second.first if no + is seen yet).
  // When several trimmer-states match from one this-state, we just get several triplets.

  // State numbers will differ in thisXtrimmer transducers and the trimmed:
  Transducer trimmed;
  std::map<SearchState, int> states_this_trimmed;

  std::list<SearchState> todo;
  std::set<SearchState> seen;
  SearchState current;
  SearchState next = make_pair(initial, make_pair(trimmer.initial,
                                                  trimmer.initial));
  todo.push_front(next);
  states_this_trimmed.insert(make_pair(next, trimmed.initial));

  while(todo.size() > 0)
  {
    current = todo.front();
    todo.pop_front();
    seen.insert(current);
    int this_src        = current.first,
        trimmer_src     = current.second.first,
        trimmer_preplus = current.second.second,
        trimmer_preplus_next = trimmer_preplus;

    if(states_this_trimmed.find(current) == states_this_trimmed.end()) {
      wcerr <<L"Error: couldn't find "<<this_src<<L","<<trimmer_src<<L" in state map"<<endl;
      exit(EXIT_FAILURE);
    }
    int trimmed_src = states_this_trimmed[current];

    // First loop through _epsilon_ transitions of trimmer
    for(multimap<int, pair<int, double> >::iterator trimmer_trans_it = trimmer.transitions.at(trimmer_src).begin(),
          trimmer_trans_limit = trimmer.transitions.at(trimmer_src).end();
        trimmer_trans_it != trimmer_trans_limit;
        trimmer_trans_it++) {
      int trimmer_label = trimmer_trans_it->first,
          trimmer_trg   = trimmer_trans_it->second.first;
      double trimmer_wt = trimmer_trans_it->second.second;
      wstring trimmer_left = L"";
      trimmer_a.getSymbol(trimmer_left, trimmer_a.decode(trimmer_label).first);

      if(trimmer_preplus == trimmer_src) {
        // Keep the old preplus state if it was set; equal to current trimmer state means unset:
        trimmer_preplus_next = trimmer_trg;
      }

      if(trimmer_left == L"")
      {
        next = make_pair(this_src, make_pair(trimmer_trg, trimmer_preplus_next));
        if(seen.find(next) == seen.end())
        {
          todo.push_front(next);
          states_this_trimmed.insert(make_pair(next, trimmed.newState()));
        }
        int trimmed_trg = states_this_trimmed[next];
        trimmed.linkStates(trimmed_src,
                           trimmed_trg,
                           epsilon_tag,
                           default_weight);
      }
    }

    // Loop through arcs from this_src; when our arc matches an arc
    // from live_trimmer_states, add that to (the front of) todo:
    for(multimap<int, pair<int, double> >::iterator trans_it = transitions[this_src].begin(),
                                                  trans_limit = transitions[this_src].end();
        trans_it != trans_limit;
        trans_it++)
    {
      int this_label = trans_it->first,
          this_trg   = trans_it->second.first;
      double this_wt = trans_it->second.second;
      wstring this_right = L"";
      this_a.getSymbol(this_right, this_a.decode(this_label).second);

      if(this_right == COMPILER_JOIN_ELEM)
      {
        if(trimmer_preplus == trimmer_src) {
          // Keep the old preplus state if it was set; equal to current trimmer state means unset:
          trimmer_preplus_next = trimmer_src; // not _trg when join!
        }
        // Go to the start in trimmer, but record where we restarted from in case we later see a #:
        next = make_pair(this_trg, make_pair(trimmer.initial, trimmer_preplus_next));
        if(seen.find(next) == seen.end())
        {
          todo.push_front(next);
        }
        if(states_this_trimmed.find(next) == states_this_trimmed.end())
        {
          states_this_trimmed.insert(make_pair(next, trimmed.newState()));
        }
        int trimmed_trg = states_this_trimmed[next];
        trimmed.linkStates(trimmed_src, // fromState
                           trimmed_trg, // toState
                           this_label, // symbol-pair, using this alphabet
                           default_weight); //weight of transduction
      }
      else if ( this_right == compoundOnlyLSymbol
                || this_right == compoundRSymbol
                || this_right == L"" )
      {
        // Stay put in the trimmer FST
        int trimmer_trg = trimmer_src;

        if(trimmer_preplus == trimmer_src) {
          // Keep the old preplus state if it was set; equal to current trimmer state means unset:
          trimmer_preplus_next = trimmer_trg;
        }

        next = make_pair(this_trg, make_pair(trimmer_trg, trimmer_preplus_next));
        if(seen.find(next) == seen.end())
        {
          todo.push_front(next);
        }
        if(states_this_trimmed.find(next) == states_this_trimmed.end())
        {
          states_this_trimmed.insert(make_pair(next, trimmed.newState()));
        }
        int trimmed_trg = states_this_trimmed[next];
        trimmed.linkStates(trimmed_src, // fromState
                           trimmed_trg, // toState
                           this_label, // symbol-pair, using this alphabet
                           default_weight); //weight of transduction
      }
      else
      {
        // Loop through non-epsilon arcs from the live state of trimmer

        // If we see a hash/group, we may have to rewind our trimmer state first:
        if(this_right == COMPILER_GROUP_ELEM && trimmer_preplus != trimmer_src)
        {
          states_this_trimmed.insert(make_pair(make_pair(this_src, make_pair(trimmer_preplus,
                                                                             trimmer_preplus)),
                                               trimmed_src));
          trimmer_src = trimmer_preplus;
        }

        for(multimap<int, pair<int, double> >::iterator trimmer_trans_it = trimmer.transitions.at(trimmer_src).begin(),
              trimmer_trans_limit = trimmer.transitions.at(trimmer_src).end();
            trimmer_trans_it != trimmer_trans_limit;
            trimmer_trans_it++)
        {
          int trimmer_label = trimmer_trans_it->first,
              trimmer_trg   = trimmer_trans_it->second.first;
          double trimmer_wt = trimmer_trans_it->second.second;
          wstring trimmer_left = L"";
          trimmer_a.getSymbol(trimmer_left, trimmer_a.decode(trimmer_label).first);

          if(trimmer_preplus == trimmer_src) {
            // Keep the old preplus state if it was set; equal to current trimmer state means unset:
            trimmer_preplus_next = trimmer_trg;
          }

          if(trimmer_left != L"" && this_right == trimmer_left) // we've already dealt with trimmer epsilons
          {
            next = make_pair(this_trg, make_pair(trimmer_trg, trimmer_preplus_next));
            if(seen.find(next) == seen.end())
            {
              todo.push_front(next);
            }
            if(states_this_trimmed.find(next) == states_this_trimmed.end())
            {
              states_this_trimmed.insert(make_pair(next, trimmed.newState()));
            }
            int trimmed_trg = states_this_trimmed[next];
            trimmed.linkStates(trimmed_src, // fromState
                               trimmed_trg, // toState
                               this_label, // symbol-pair, using this alphabet
                               default_weight); //weight of transduction
          }
        } // end loop arcs from trimmer_src
      } // end if JOIN else
    } // end loop arcs from this_src
  } // end while todo

  for(map<SearchState, int >::iterator it = states_this_trimmed.begin(),
        limit = states_this_trimmed.end();
      it != limit;
      it++)
  {
    int s_this = it->first.first;
    int s_trimmer = it->first.second.first; // ignore the preplus here
    int s_trimmed = it->second;
    if(isFinal(s_this) && trimmer.isFinal(s_trimmer))
    {
      trimmed.finals.insert(pair <int, double>(s_trimmed, default_weight));
    }
  }

  // We do not minimize here, in order to let lt_trim print a warning
  // (instead of exiting the whole program) if no finals.
  return trimmed;
}
