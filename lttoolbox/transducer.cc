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
 * along with this program; if not, see <https://www.gnu.org/licenses/>.
 */
#include <lttoolbox/transducer.h>
#include <lttoolbox/compression.h>
#include <lttoolbox/alphabet.h>
#include <lttoolbox/my_stdio.h>
#include <lttoolbox/deserialiser.h>
#include <lttoolbox/serialiser.h>

#include <cstdlib>
#include <iostream>
#include <vector>
#include <cstring>


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
Transducer::insertSingleTransduction(int const tag, int const source, double const weight)
{
  if(transitions.find(source) != transitions.end())
  {
    if(transitions[source].count(tag) == 1)
    {
      auto range = transitions[source].equal_range(tag);
      return range.first->second.first;
    }
    else if(transitions[source].count(tag) == 0)
    {
      // new state
      int state = newState();
      transitions[source].insert({tag, std::make_pair(state, weight)});
      return state;
    }
    else if(transitions[source].count(tag) == 2)
    {
      // there's a local cycle, must be ignored and treated like in '1'
      auto range = transitions[source].equal_range(tag);
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
  transitions[source].insert({tag, std::make_pair(state, weight)});
  return state;
}

int
Transducer::insertTransducer(int const source, Transducer &t,
                            int const epsilon_tag)
{
  std::map<int, int> relation;

  if(t.transitions.empty())
  {
	return source;
  }

  t.joinFinals(epsilon_tag);

  for(auto& it : t.transitions)
  {
    relation[it.first] = newState();
  }

  for(auto& it : t.transitions)
  {
    for(auto& it2 : it.second)
    {
      transitions[relation[it.first]].insert({it2.first, std::make_pair(relation[it2.second.first], it2.second.second)});
    }
  }

  transitions[source].insert({epsilon_tag, std::make_pair(relation[t.initial], default_weight)});

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
    auto range = transitions[source].equal_range(tag);
    for(;range.first != range.second; range.first++)
    {
      if(range.first->first == tag && range.first->second.first == target)
      {
        return;
      }
    }
    // end of new code
    transitions[source].insert({tag, std::make_pair(target, weight)});
  }
  else
  {
    std::cerr << "Error: Trying to link nonexistent states (" << source;
    std::cerr << ", " << target << ", " << tag << ")" << std::endl;
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
    std::cerr << "Setting initial state to final" << std::endl;
  }
*/
  if(value)
  {
    finals.insert({state, weight});
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

std::set<int>
Transducer::closure(int const state, int const epsilon_tag) const
{
  return closure(state, std::set<int>({epsilon_tag}));
}

std::set<int>
Transducer::closure(int const state, std::set<int> const &epsilon_tags) const
{
  std::set<int> nonvisited, result;

  nonvisited.insert(state);
  result.insert(state);

  while (nonvisited.size() > 0) {
    int auxest = *nonvisited.begin();
    for (const int epsilon_tag : epsilon_tags) {
      try {
        auto range = transitions.at(auxest).equal_range(epsilon_tag);
        while (range.first != range.second) {
          if (result.find(range.first->second.first) == result.end()) {
            result.insert(range.first->second.first);
            nonvisited.insert(range.first->second.first);
          }
          range.first++;
        }
      } catch (std::out_of_range const &e) {
        // No transition from any of the epsilon_tags – this is fine
      }
    }
    nonvisited.erase(auxest);
  }

  return result;
}

std::vector<sorted_vector<int>>
Transducer::closure_all(const int epsilon_tag) const
{
  std::vector<sorted_vector<int>> ret;
  ret.reserve(transitions.size());
  std::vector<std::vector<int>> reversed;
  reversed.resize(transitions.size());
  sorted_vector<int> todo;
  for (size_t i = 0; i < transitions.size(); i++) {
    sorted_vector<int> c;
    c.insert(i);
    auto range = transitions.at(i).equal_range(epsilon_tag);
    for (; range.first != range.second; range.first++) {
      if (range.first->second.second != default_weight) continue;
      c.insert(range.first->second.first);
      reversed[range.first->second.first].push_back(i);
    }
    if (c.size() > 1) todo.insert(i);
    ret.push_back(c);
  }
  while (!todo.empty()) {
    sorted_vector<int> new_todo;
    for (auto& it : todo) {
      sorted_vector<int> temp = ret[it];
      for (auto& it2 : temp) {
        ret[it].insert(ret[it2].begin(), ret[it2].end());
      }
      if (ret[it].size() > temp.size())
        new_todo.insert(reversed[it].begin(), reversed[it].end());
    }
    todo.swap(new_todo);
  }
  return ret;
}

void
Transducer::joinFinals(int const epsilon_tag)
{
  if(finals.size() > 1)
  {
    int state = newState();

    for(auto& it : finals)
    {
      linkStates(it.first, state, epsilon_tag, it.second);
    }

    finals.clear();
    finals.insert({state, default_weight});
  }
  else if(finals.size() == 0)
  {
    std::cerr << "Error: empty set of final states" << std::endl;
    exit(EXIT_FAILURE);
  }
}

void
Transducer::determinize(int const epsilon_tag)
{
  std::vector<sorted_vector<int>> R(2);
  std::vector<sorted_vector<int>> Q_prime;
  std::map<sorted_vector<int>, int> Q_prime_inv;

  std::map<int, std::multimap<int, std::pair<int, double> > > transitions_prime;

  // We're almost certainly going to need the closure of (nearly) every
  // state, and we're often going to need the closure several times,
  // so it's faster to precompute.
  std::vector<sorted_vector<int>> all_closures = closure_all(epsilon_tag);

  unsigned int size_Q_prime = 0;
  Q_prime.push_back(all_closures[initial]);

  Q_prime_inv[Q_prime[0]] = 0;
  R[0].insert(0);

  int initial_prime = 0;
  std::map<int, double> finals_prime;

  if(isFinal(initial))
  {
    finals_prime.insert({0, default_weight});
  }

  int t = 0;

  sorted_vector<int> finals_state;
  for(auto& it : finals) {
    finals_state.insert(it.first);
  }

  while(size_Q_prime != Q_prime.size())
  {
    size_Q_prime = Q_prime.size();
    R[(t+1)%2].clear();

    for(auto& it : R[t])
    {
      if (Q_prime[it].intersects(finals_state)) {
        double w = default_weight;
        auto it3 = finals.find(it);
        if (it3 != finals.end()) {
          w = it3->second;
        }
        finals_prime.insert({it, w});
      }

      std::map<std::pair<int, double>, sorted_vector<int> > mymap;

      for(auto& it2 : Q_prime[it])
      {
        for(auto& it3 : transitions[it2])
        {
          if(it3.first != epsilon_tag || it3.second.second != default_weight)
          {
            auto& it4 = all_closures[it3.second.first];
            mymap[std::make_pair(it3.first, it3.second.second)].insert(it4.begin(), it4.end());
          }
        }
      }

      // adding new states
      auto& state_prime = transitions_prime[it];
      for(auto& it2 : mymap)
      {
        int tag;
        auto loc = Q_prime_inv.find(it2.second);
        if(loc == Q_prime_inv.end()) {
          tag = Q_prime.size();
          Q_prime.push_back(it2.second);
          Q_prime_inv[it2.second] = tag;
          R[(t+1)%2].insert(tag);
          transitions_prime[tag].clear();
        } else {
          tag = loc->second;
        }
        state_prime.insert({it2.first.first, std::make_pair(tag, it2.first.second)});
      }
    }

    t = (t+1)%2;
  }

  transitions.swap(transitions_prime);
  finals.swap(finals_prime);
  initial = initial_prime;
}


void
Transducer::minimize(int const epsilon_tag)
{
  if (finals.empty()) return;
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
  finals.insert({state, default_weight});
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
  finals.insert({state, default_weight});
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

std::map<int, std::multimap<int, std::pair<int, double> > >&
Transducer::getTransitions()
{
  return transitions;
}

std::map<int, double>
Transducer::getFinals() const
{
  return std::map<int, double>(finals);
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

  for(auto& it : transitions)
  {
    counter += it.second.size();
  }

  return counter;
}

bool
Transducer::isEmpty(int const state) const
{

  auto it = transitions.find(state);
  if(it != transitions.end())
  {
    if(it->second.size() > 0)
    {
      return false;
    }
  }

  return true;
}

// Determine whether any weights are non-default (0)
bool Transducer::weighted() const {
  for (auto& it : finals) {
    if (it.second != default_weight) {
      return true;
    }
  }
  for (auto& it : transitions) {
    for (auto& it2 : it.second) {
      if (it2.second.second != default_weight) {
        return true;
      }
    }
  }
  return false;
}

void
Transducer::write(FILE *output, int const decalage)
{
  fwrite_unlocked(HEADER_TRANSDUCER, 1, 4, output);

  bool write_weights = weighted();

  uint64_t features = 0;
  if (write_weights) {
      features |= TDF_WEIGHTS;
  }
  write_le(output, features);

  Compression::multibyte_write(initial, output);
  Compression::multibyte_write(finals.size(), output);

  int base = 0;
  for(auto& it : finals)
  {
    Compression::multibyte_write(it.first - base, output);
    base = it.first;
    if(write_weights)
    {
      Compression::long_multibyte_write(it.second, output);
    }
  }

  base = transitions.size();
  Compression::multibyte_write(base, output);
  for(auto& it : transitions)
  {
    Compression::multibyte_write(it.second.size(), output);
    int tagbase = 0;
    for(auto& it2 : it.second)
    {
      Compression::multibyte_write(it2.first - tagbase + decalage, output);
      tagbase = it2.first;

      if(it2.second.first >= it.first)
      {
        Compression::multibyte_write(it2.second.first - it.first, output);
      }
      else
      {
        Compression::multibyte_write(it2.second.first + base - it.first, output);
      }
      if(write_weights)
      {
        Compression::long_multibyte_write(it2.second.second, output);
      }
    }
  }
}

void
Transducer::read(FILE *input, int const decalage)
{
  Transducer new_t;

  bool read_weights = false;

  fpos_t pos;
  if (fgetpos(input, &pos) == 0) {
      char header[4]{};
      fread_unlocked(header, 1, 4, input);
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

  new_t.initial = Compression::multibyte_read(input);
  int finals_size = Compression::multibyte_read(input);

  int base = 0;
  double base_weight = default_weight;
  while(finals_size > 0)
  {
    finals_size--;

    base += Compression::multibyte_read(input);
    if(read_weights)
    {
      base_weight = Compression::long_multibyte_read(input);
    }
    new_t.finals.insert({base, base_weight});
  }

  base = Compression::multibyte_read(input);
  int number_of_states = base;
  int current_state = 0;
  while(number_of_states > 0)
  {
    int number_of_local_transitions = Compression::multibyte_read(input);
    int tagbase = 0;
    if (new_t.transitions.find(current_state) == new_t.transitions.end()) {
      new_t.transitions[current_state].clear(); // force create
    }
    while(number_of_local_transitions > 0)
    {
      number_of_local_transitions--;
      tagbase += Compression::multibyte_read(input) - decalage;
      int state = (current_state + Compression::multibyte_read(input)) % base;
      if(read_weights)
      {
        base_weight = Compression::long_multibyte_read(input);
      }
      if(new_t.transitions.find(state) == new_t.transitions.end())
      {
        new_t.transitions[state].clear(); // force create
      }
      new_t.transitions[current_state].insert({tagbase, std::make_pair(state, base_weight)});
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
  Serialiser<std::map<int, double> >::serialise(finals, serialised);
  Serialiser<std::map<int, std::multimap<int, std::pair<int, double> > > >::serialise(transitions, serialised);
}

void
Transducer::deserialise(std::istream &serialised)
{
  initial = Deserialiser<int>::deserialise(serialised);
  finals = Deserialiser<std::map<int, double> >::deserialise(serialised);
  transitions = Deserialiser<std::map<int, std::multimap<int, std::pair<int, double> > > >::deserialise(serialised);
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

  std::map<int, std::multimap<int, std::pair<int, double> > > tmp_transitions;

  for(std::map<int, std::multimap<int, std::pair<int, double> > >::reverse_iterator it = transitions.rbegin(); it != transitions.rend(); it++)
  {
    auto aux = it->second;
    it->second.clear();
    for(auto& it2 : aux)
    {
      if(it2.second.first >= it->first)
      {
        transitions[it2.second.first].insert({it2.first, std::make_pair(it->first, it2.second.second)});
      }
      else
      {
        tmp_transitions[it2.second.first].insert({it2.first, std::make_pair(it->first, it2.second.second)});
      }
    }
    if(tmp_transitions.find(it->first) != tmp_transitions.end())
    {
      it->second.insert(tmp_transitions[it->first].begin(), tmp_transitions[it->first].end());
      tmp_transitions.erase(it->first);
    }
  }

  for(std::map<int, std::multimap<int, std::pair<int, double> > >::reverse_iterator it = tmp_transitions.rbegin(),
                                                                  limit = tmp_transitions.rend();
      it != limit; it++)
  {
    for(auto& it2 : it->second)
    {
      transitions[it->first].insert({it2.first, it2.second});
    }
  }

  int tmp = initial;
  initial = finals.begin()->first;
  finals.clear();
  finals.insert({tmp, default_weight});
}

void
Transducer::escapeSymbol(UString& symbol, bool hfst) const
{
  if(symbol.empty()) // If it's an epsilon
  {
    if(hfst)
    {
      symbol = HFST_EPSILON_SYMBOL_SHORT;
    }
    else
    {
      symbol = LTTB_EPSILON_SYMBOL;
    }
  }
  else if(hfst && symbol == u" "_uv)
  {
    symbol = HFST_SPACE_SYMBOL;
  }
  else if(hfst && symbol == u"\t"_uv)
  {
    symbol = HFST_TAB_SYMBOL;
  }
}

void
Transducer::show(Alphabet const &alphabet, UFILE *output, int const epsilon_tag, bool hfst) const
{
  for(auto& it : transitions)
  {
    for(auto& it2 : it.second)
    {
      auto t = alphabet.decode(it2.first);
      u_fprintf(output, "%d\t%d\t", it.first, it2.second.first);
      UString l;
      alphabet.getSymbol(l, t.first);
      escapeSymbol(l, hfst);
      u_fprintf(output, "%S\t", l.c_str());
      UString r;
      alphabet.getSymbol(r, t.second);
      escapeSymbol(r, hfst);
      u_fprintf(output, "%S\t", r.c_str());
      u_fprintf(output, "%f\n", it2.second.second);
    }
  }

  for(auto& it3 : finals)
  {
    u_fprintf(output, "%d\t%f\n", it3.first, it3.second);
  }
}

int
Transducer::getStateSize(int const state)
{
 std::set<int> states;
 auto myclosure1 = closure(state, 0);
 states.insert(myclosure1.begin(), myclosure1.end());
 int num_transitions = 0;

 for(auto& it2 : states)
 {
   num_transitions += transitions[it2].size();
 }

 return num_transitions;
}

bool
Transducer::recognise(UStringView pattern, Alphabet &a, FILE *err) const
{
  bool accepted = false;
  std::set<int> states;

  auto myclosure1 = closure(getInitial(), 0);
  states.insert(myclosure1.begin(), myclosure1.end());
  // For each of the characters in the input string
  for(auto& it : pattern)
  {
    std::set<int> new_state;        //Transducer::closure(int const state, int const epsilon_tag)
    // For each of the current alive states
    //fprintf(err, "step: %ls %lc (%d)\n", pattern.c_str(), *it, sym);
    for(auto& it2 : states)
    {
      auto& p = transitions.at(it2);
      // For each of the transitions in the state

      for(auto& it3 : p)
      {

        auto t = a.decode(it3.first);
        UString l;
        a.getSymbol(l, t.first);
        //UString r;
        //a.getSymbol(r, t.second);

        //fprintf(err, "  -> state: %d, trans: %ls:%ls, targ: %d\n", *it2, (l.empty()) ?  "ε" : l.c_str(),  (r.empty()) ?  "ε" : r.c_str(), it3->second);
        //if(l.find(*it) != UString::npos || l.empty() )
        if(l.find(it) != UString::npos)
        {
          auto myclosure = closure(it3.second.first, 0);
          //std::cerr << "Before closure alives: " <<new_state.size() << std::endl;
          new_state.insert(myclosure.begin(), myclosure.end());
          //std::cerr << "After closure alives: " <<new_state.size() << std::endl;
        }
      }
    }
    states = new_state;
  }
  for(auto& it4 : states)
  {
    if(isFinal(it4))
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
  finals.insert({insertTransducer(initial, t, epsilon_tag), default_weight});
}

Transducer
Transducer::appendDotStar(std::set<int> const &loopback_symbols, int const epsilon_tag)
{
  Transducer prefix_transducer(*this);

  for(auto& prefix_it : prefix_transducer.finals)
  {
    for(auto& loopback_it : loopback_symbols)
    {
      if(loopback_it != epsilon_tag) // TODO: Necessary? Minimization should remove epsilon loopbacks anyway
      {
        prefix_transducer.linkStates(prefix_it.first, prefix_it.first, loopback_it, prefix_it.second);
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

  std::map<int, int> states_this_new;
  states_this_new.insert({start, new_t.initial});
  std::map<int, int> states_this_lemq;
  states_this_new.insert({start, lemq.initial});

  typedef std::pair<int, int> SearchState;
  // Each searchstate in the stack is a transition in this FST, along
  // with the last reached state of the lemq
  std::vector<SearchState> todo;
  std::set<SearchState> seen;
  std::set<SearchState> finally;
  SearchState current;
  todo.push_back({start,start});

  while(!todo.empty()) {
    current = todo.back();
    todo.pop_back();
    seen.insert(current);
    int this_src = current.first, this_lemqlast = current.second;

    for(auto& trans_it : transitions[this_src])
    {
      int label = trans_it.first, this_trg = trans_it.second.first;
      double this_wt = trans_it.second.second;
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
          lemq.finals.insert({this_lemqlast, default_weight});
        }
        else
        {
          if(states_this_new.find(this_src) == states_this_new.end())
          {
            states_this_new.insert({this_src, new_t.newState()});
          }
          new_src = states_this_new[this_src];
        }

        if(states_this_new.find(this_trg) == states_this_new.end())
        {
          states_this_new.insert({this_trg, new_t.newState()});
        }
        int new_trg = states_this_new[this_trg];
        new_t.linkStates(new_src, new_trg, label, this_wt);

        if(isFinal(this_src))
        {
          finally.insert({this_src, this_lemqlast});
        }

        if(seen.find(std::make_pair(this_trg, this_lemqlast)) == seen.end())
        {
          todo.push_back({this_trg, this_lemqlast});
        }
      }
      else
      {
        // We're still reading the lemq, append label to current one:
        int lemq_src = states_this_lemq[this_src];
        if(states_this_lemq.find(this_trg) == states_this_lemq.end())
        {
          states_this_lemq.insert({this_trg, lemq.newState()});
        }
        int lemq_trg = states_this_lemq[this_trg];
        lemq.linkStates(lemq_src, lemq_trg, label, this_wt);
        if(seen.find({this_trg, this_trg}) == seen.end())
        {
          todo.push_back({this_trg, this_trg});
        }
      }
    } // end for transitions
  } // end while todo

  for(auto& it : finally)
  {
    int last_tag = it.first,
      this_lemqlast = it.second;
    // copy lemq, letting this_lemqlast be the only final state in newlemq
    Transducer newlemq = Transducer(lemq);
    newlemq.finals.clear();
    newlemq.finals.insert({states_this_lemq[this_lemqlast], default_weight});
    newlemq.minimize();

    int group_start = new_t.newState();
    new_t.linkStates(states_this_new[last_tag], group_start, group_label, default_weight);

    // append newlemq into the group after last_tag:
    new_t.finals.insert({new_t.insertTransducer(group_start, newlemq), default_weight});
  }

  return new_t;
}

Transducer
Transducer::moveLemqsLast(Alphabet const &alphabet,
                          int const epsilon_tag)
{
  Transducer new_t;
  typedef int SearchState;
  std::set<SearchState> seen;
  std::vector<SearchState> todo;
  todo.push_back(initial);

  std::map<int, int> states_this_new;
  states_this_new.insert({initial, new_t.initial});

  while(!todo.empty()) {
    int this_src = todo.back();
    todo.pop_back();
    seen.insert(this_src);
    for(auto& trans_it : transitions[this_src])
    {
      int label = trans_it.first,
       this_trg = trans_it.second.first;
      UString left;
      alphabet.getSymbol(left, alphabet.decode(label).first);
      int new_src = states_this_new[this_src];

      if(left == GROUP_SYMBOL)
      {
        Transducer tagsFirst = copyWithTagsFirst(this_trg, label, alphabet, epsilon_tag);
        new_t.finals.insert({new_t.insertTransducer(new_src, tagsFirst, epsilon_tag), default_weight});
      }
      else
      {
        if(states_this_new.find(this_trg) == states_this_new.end())
        {
          states_this_new.insert({this_trg, new_t.newState()});
        }
        int new_trg = states_this_new[this_trg];
        new_t.linkStates(new_src, new_trg, label, default_weight);
        if(seen.find(this_trg) == seen.end())
        {
          todo.push_back(this_trg);
        }
      }
    }
  }

  for(auto& it : finals)
   {
     new_t.finals.insert({states_this_new[it.first], it.second});
   }

  return new_t;
}


Transducer
Transducer::trim(Transducer &trimmer,
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

  // When searching, we need to record (this, (trimmer, trimmer_pre_plus))
  typedef std::pair<int, std::pair<int, int > > SearchState;
  // first: currently searched state in this;
  // second.first: currently matched trimmer state;
  // second.second: last matched trimmer state before a + restart (or the same second.first if no + is seen yet).
  // When several trimmer-states match from one this-state, we just get several triplets.

  // State numbers will differ in thisXtrimmer transducers and the trimmed:
  Transducer trimmed;
  std::map<SearchState, int> states_this_trimmed;

  std::vector<SearchState> todo;
  std::set<SearchState> seen;
  SearchState current;
  SearchState next{initial, {trimmer.initial, trimmer.initial}};
  todo.push_back(next);
  states_this_trimmed.insert({next, trimmed.initial});

  sorted_vector<int32_t> sym_wb, sym_lsx, sym_cmp_or_eps;
  {
    if (this_a.isSymbolDefined(LSX_BOUNDARY_SYMBOL))
      sym_lsx.insert(this_a(LSX_BOUNDARY_SYMBOL));
    if (this_a.isSymbolDefined(LSX_BOUNDARY_SPACE_SYMBOL))
      sym_lsx.insert(this_a(LSX_BOUNDARY_SPACE_SYMBOL));
    if (this_a.isSymbolDefined(LSX_BOUNDARY_NO_SPACE_SYMBOL))
      sym_lsx.insert(this_a(LSX_BOUNDARY_NO_SPACE_SYMBOL));
    sym_wb = sym_lsx;
    sym_wb.insert(static_cast<int32_t>('+')); // JOIN_SYMBOL

    if (this_a.isSymbolDefined(COMPOUND_ONLY_L_SYMBOL))
      sym_cmp_or_eps.insert(this_a(COMPOUND_ONLY_L_SYMBOL));
    if (this_a.isSymbolDefined(COMPOUND_R_SYMBOL))
      sym_cmp_or_eps.insert(this_a(COMPOUND_R_SYMBOL));
    sym_cmp_or_eps.insert(0); // epsilon
  }

  while(!todo.empty()) {
    current = todo.back();
    todo.pop_back();
    seen.insert(current);
    int this_src        = current.first,
        trimmer_src     = current.second.first,
        trimmer_preplus = current.second.second,
        trimmer_preplus_next = trimmer_preplus;

    if(states_this_trimmed.find(current) == states_this_trimmed.end()) {
      std::cerr <<"Error: couldn't find "<<this_src<<","<<trimmer_src<<" in state map"<< std::endl;
      exit(EXIT_FAILURE);
    }
    int trimmed_src = states_this_trimmed[current];

    // First loop through _epsilon_ transitions of trimmer
    for(auto& trimmer_trans_it : trimmer.transitions[trimmer_src]) {
      int trimmer_label = trimmer_trans_it.first,
          trimmer_trg   = trimmer_trans_it.second.first;
      double trimmer_wt = trimmer_trans_it.second.second;
      int32_t trimmer_left = trimmer_a.decode(trimmer_label).first;

      if(trimmer_preplus == trimmer_src) {
        // Keep the old preplus state if it was set; equal to current trimmer state means unset:
        trimmer_preplus_next = trimmer_trg;
      }

      if(trimmer_left == 0)
      {
        next = std::make_pair(this_src, std::make_pair(trimmer_trg, trimmer_preplus_next));
        if(seen.find(next) == seen.end())
        {
          todo.push_back(next);
          states_this_trimmed.insert({next, trimmed.newState()});
        }
        int trimmed_trg = states_this_trimmed[next];
        trimmed.linkStates(trimmed_src,
                           trimmed_trg,
                           epsilon_tag,
                           trimmer_wt);
      }
    }

    // Loop through arcs from this_src; when our arc matches an arc
    // from live_trimmer_states, add that to (the front of) todo:
    for(auto& trans_it : transitions[this_src])
    {
      int this_label = trans_it.first,
          this_trg   = trans_it.second.first;
      double this_wt = trans_it.second.second;
      int32_t this_right = this_a.decode(this_label).second;

      bool special = false;

      if (sym_wb.count(this_right)) {
        special = (this_right != static_cast<int32_t>('+'));
        if(trimmer_preplus == trimmer_src) {
          // Keep the old preplus state if it was set; equal to current trimmer state means unset:
          trimmer_preplus_next = trimmer_src; // not _trg when join!
        }
        // Go to the start in trimmer, but record where we restarted from in case we later see a #:
        next = std::make_pair(this_trg, std::make_pair(trimmer.initial, trimmer_preplus_next));
        if(seen.find(next) == seen.end())
        {
          todo.push_back(next);
        }
        if(states_this_trimmed.find(next) == states_this_trimmed.end())
        {
          states_this_trimmed.insert({next, trimmed.newState()});
        }
        int trimmed_trg = states_this_trimmed[next];
        trimmed.linkStates(trimmed_src, // fromState
                           trimmed_trg, // toState
                           this_label, // symbol-pair, using this alphabet
                           this_wt); //weight of transduction
        if (sym_lsx.count(this_right) && isFinal(this_trg)) {
          trimmed.setFinal(trimmed_trg, default_weight);
        }
      }
      else if (sym_cmp_or_eps.count(this_right)) {
        special = true;
        // Stay put in the trimmer FST
        int trimmer_trg = trimmer_src;

        if(trimmer_preplus == trimmer_src) {
          // Keep the old preplus state if it was set; equal to current trimmer state means unset:
          trimmer_preplus_next = trimmer_trg;
        }

        next = std::make_pair(this_trg, std::make_pair(trimmer_trg, trimmer_preplus_next));
        if(seen.find(next) == seen.end())
        {
          todo.push_back(next);
        }
        if(states_this_trimmed.find(next) == states_this_trimmed.end())
        {
          states_this_trimmed.insert({next, trimmed.newState()});
        }
        int trimmed_trg = states_this_trimmed[next];
        trimmed.linkStates(trimmed_src, // fromState
                           trimmed_trg, // toState
                           this_label, // symbol-pair, using this alphabet
                           this_wt); //weight of transduction
      }

      // if we're at a normal symbol or a + that might be part of a lemma
      if (!special) {
        // Loop through non-epsilon arcs from the live state of trimmer

        // If we see a hash/group, we may have to rewind our trimmer state first:
        if(this_right == static_cast<int32_t>('#') &&
           trimmer_preplus != trimmer_src)
        {
          states_this_trimmed.insert({std::make_pair(this_src, std::make_pair(trimmer_preplus, trimmer_preplus)), trimmed_src});
          trimmer_src = trimmer_preplus;
        }

        for(auto& trimmer_trans_it : trimmer.transitions.at(trimmer_src))
        {
          int trimmer_label = trimmer_trans_it.first,
              trimmer_trg   = trimmer_trans_it.second.first;
          int32_t trimmer_left = trimmer_a.decode(trimmer_label).first;

          if(trimmer_preplus == trimmer_src) {
            // Keep the old preplus state if it was set; equal to current trimmer state means unset:
            trimmer_preplus_next = trimmer_trg;
          }

          if (trimmer_left != 0 && // we've already dealt with trimmer epsilons
              this_a.sameSymbol(this_right, trimmer_a, trimmer_left, true)) {
            next = std::make_pair(this_trg, std::make_pair(trimmer_trg, trimmer_preplus_next));
            if(seen.find(next) == seen.end())
            {
              todo.push_back(next);
            }
            if(states_this_trimmed.find(next) == states_this_trimmed.end())
            {
              states_this_trimmed.insert(std::make_pair(next, trimmed.newState()));
            }
            int trimmed_trg = states_this_trimmed[next];
            trimmed.linkStates(trimmed_src, // fromState
                               trimmed_trg, // toState
                               this_label, // symbol-pair, using this alphabet
                               this_wt); //weight of transduction
          }
        } // end loop arcs from trimmer_src
      } // end if JOIN else
    } // end loop arcs from this_src
  } // end while todo

  for(auto& it : states_this_trimmed)
  {
    int s_this = it.first.first;
    int s_trimmer = it.first.second.first; // ignore the preplus here
    int s_trimmed = it.second;
    if(isFinal(s_this) && trimmer.isFinal(s_trimmer))
    {
      trimmed.finals.insert(std::make_pair(s_trimmed, finals[s_this]));
    }
  }

  // We do not minimize here, in order to let lt_trim print a warning
  // (instead of exiting the whole program) if no finals.
  return trimmed;
}

void
Transducer::updateAlphabet(Alphabet& old_alpha, Alphabet& new_alpha,
                           bool has_pairs)
{
  std::set<int32_t> symbol_pairs;
  std::set<int32_t> symbols;
  for (auto& it : transitions) {
    for (auto& it2 : it.second) {
      if (!has_pairs && it2.first < 0) {
        symbols.insert(it2.first);
      } else {
        symbol_pairs.insert(it2.first);
        int32_t l = old_alpha.decode(it2.first).first;
        int32_t r = old_alpha.decode(it2.first).second;
        if (l < 0) {
          symbols.insert(l);
        }
        if (r < 0) {
          symbols.insert(r);
        }
      }
    }
  }
  std::map<int32_t, int32_t> symbol_update;
  for (auto& it : symbols) {
    UString s;
    old_alpha.getSymbol(s, it);
    new_alpha.includeSymbol(s);
    symbol_update[it] = new_alpha(s);
  }
  if (has_pairs) {
    std::map<int32_t, int32_t> pair_update;
    for (auto& it : symbol_pairs) {
      int32_t l1 = old_alpha.decode(it).first;
      int32_t r1 = old_alpha.decode(it).second;
      int32_t l2 = (l1 < 0 ? symbol_update[l1] : l1);
      int32_t r2 = (r1 < 0 ? symbol_update[r1] : r1);
      pair_update[it] = new_alpha(l2, r2);
    }
    symbol_update.swap(pair_update);
  }
  std::map<int, std::multimap<int, std::pair<int, double> > > new_trans;
  for (auto& it : transitions) {
    new_trans[it.first].clear();
    for (auto& it2 : it.second) {
      int32_t s = it2.first;
      if (symbol_update.find(s) != symbol_update.end()) {
        s = symbol_update[s];
      }
      new_trans[it.first].insert({s, std::make_pair(it2.second.first, it2.second.second)});
    }
  }
  transitions.swap(new_trans);
}

void
Transducer::invert(Alphabet& alpha)
{
  std::map<int, std::multimap<int, std::pair<int, double>>> tmp_trans;
  for (auto& it : transitions) {
    std::multimap<int, std::pair<int, double>> tmp_state;
    for (auto& it2 : it.second) {
      auto pr = alpha.decode(it2.first);
      int new_sym = alpha(pr.second, pr.first);
      tmp_state.insert({new_sym, it2.second});
    }
    tmp_trans.insert({it.first, tmp_state});
  }
  transitions.swap(tmp_trans);
}

void
Transducer::deleteSymbols(const sorted_vector<int32_t>& syms)
{
  for (auto& state : transitions) {
    for (auto& sym : syms) {
      state.second.erase(sym);
    }
  }
}

void
Transducer::epsilonizeSymbols(const sorted_vector<int32_t>& syms)
{
  for (auto& state: transitions) {
    for (auto& sym : syms) {
      auto pr = state.second.equal_range(sym);
      for (auto it = pr.first; it != pr.second; it++) {
        state.second.insert({0, it->second});
      }
      state.second.erase(sym);
    }
  }
}

void
Transducer::applyACX(Alphabet& alpha,
                     const std::map<int32_t, sorted_vector<int32_t>>& acx)
{
  for (auto& state : transitions) {
    std::vector<std::pair<int, std::pair<int, double>>> to_insert;
    for (auto& it : state.second) {
      auto pr = alpha.decode(it.first);
      auto loc = acx.find(pr.first);
      if (loc != acx.end()) {
        for (auto& sym : loc->second) {
          to_insert.push_back({alpha(sym, pr.second), it.second});
        }
      }
    }
    for (auto& it : to_insert) {
      state.second.insert(it);
    }
  }
}


Transducer
Transducer::compose(Transducer const &g,
                    Alphabet &f_a, // this alphabet
                    Alphabet const &g_a, // alphabet of g
                    bool f_inverted,
                    bool g_anywhere,
                    int const epsilon_tag)
{
  /**
   * g ∘ f = composed
   * where f = this
   *
   * The basic algorithm is:
   *
   * Transducer gf;             // composition
   * queue = [ (f.init, g.init, gf.init) ]
   * while(!queue.empty()):
   *   f_src, g_src, gf_src = queue.pop()
   *   for (f_input, f_output, f_trg) in state_f.transitions:
   *     for (g_input, g_output, g_trg) in state_g.transitions:
   *       if g_input == f_output:
   *         gf_trg = gf.add_state()
   *         gf.add_transition(gf_src,  f_input, g_output, gf_trg)
   *         queue.add(f_trg, g_trg, gf_trg)
   *     gf_trg = gf.add_state()
   *     gf.add_transition(gf_src, f_input, f_output, gf_trg)
   *     queue.add(f_trg, g.init, gf_trg)
   *
   * with some added complications:
   *
   * 1. Since we can have loops, we need to keep track of which pairs
   *    (f_src, g_src) we've seen, so we don't keep re-adding them. And
   *    on seeing a previously seen pair, we need to know which state
   *    in gf was added from that pair.
   *
   * 2. We have to be able to skip epsilons on the input-side of g and
   *    the output-side of f.
   *
   * 3. When f_inverted, we swap f_left/f_right in matching.
   *
   * 4. If composing g_anywhere, we can add initials from g anywhere
   *    in gf, and finals in gf can be initials in g.
   */
  joinFinals(epsilon_tag);

  // (f_state, g_state)
  typedef std::pair<int, int> SearchState;

  // State numbers will differ in fXg transducers and gf:
  Transducer gf;
  std::map<SearchState, int> states_f_g_gf;

  std::vector<SearchState> todo;
  std::set<SearchState> seen;
  SearchState current;
  SearchState next{initial, g.initial};
  todo.push_back(next);
  states_f_g_gf.insert({next, gf.initial});

  while(!todo.empty()) {
    current = todo.back();
    todo.pop_back();
    seen.insert(current);
    int f_src  = current.first,
        g_src     = current.second;

    if(states_f_g_gf.find(current) == states_f_g_gf.end()) {
      std::cerr <<"Error: couldn't find "<<f_src<<","<<g_src<<" in state map"<< std::endl;
      exit(EXIT_FAILURE);
    }
    int gf_src = states_f_g_gf[current];

    // First loop through _epsilon_ transitions of g (input side)
    for(const auto &g_trans_it : g.transitions.at(g_src)) {
      int g_label = g_trans_it.first,
          g_trg   = g_trans_it.second.first;
      double g_wt = g_trans_it.second.second;
      std::pair<int32_t, int32_t> g_leftright = g_a.decode(g_label);
      int32_t g_left = g_leftright.first,
              g_right = g_leftright.second;

      if(g_left == 0)
      {
        next = std::make_pair(f_src, g_trg);
        if (seen.find(next) == seen.end()) {
          todo.push_back(next);
        }
        if (states_f_g_gf.find(next) == states_f_g_gf.end()) {
          states_f_g_gf.insert({next, gf.newState()});
        }
        int gf_trg = states_f_g_gf[next];
        int32_t gf_label = composeLabel(f_a, g_a, 0, g_right, f_inverted);
        gf.linkStates(gf_src,
                      gf_trg,
                      gf_label,
                      g_wt);
      }
    }

    // Loop through arcs from f_src; when the right-side of our arc
    // matches left-side of an arc from g states, add that to todo:
    for(auto& trans_it : transitions[f_src])
    {
      int f_label = trans_it.first,
          f_trg   = trans_it.second.first;
      double f_wt = trans_it.second.second;
      std::pair<int32_t, int32_t> f_leftright = f_a.decode(f_label);
      int32_t f_input, f_output;
      if(f_inverted) {
        f_input = f_leftright.second;
        f_output = f_leftright.first;
      }
      else {
        f_input = f_leftright.first;
        f_output = f_leftright.second;
      }

      // Loop through non-epsilon arcs from the live state of g
      for (auto &g_trans_it : g.transitions.at(g_src)) {
        int g_label = g_trans_it.first,
            g_trg = g_trans_it.second.first;
        double g_wt = g_trans_it.second.second;
        std::pair<int32_t, int32_t> g_leftright = g_a.decode(g_label);
        const int32_t g_left = g_leftright.first,
                      g_right = g_leftright.second;

        // output of f same as input of g?
        // label becomes input of f and output of g
        if (g_left != 0 && // we've already dealt with g epsilons
            f_a.sameSymbol(f_output, g_a, g_left, true)) {
          next = std::make_pair(f_trg, g_trg);
          if (seen.find(next) == seen.end()) {
            todo.push_back(next);
          }
          if (states_f_g_gf.find(next) == states_f_g_gf.end()) {
            states_f_g_gf.insert({next, gf.newState()});
          }
          int gf_trg = states_f_g_gf[next];
          int32_t gf_label = composeLabel(f_a, g_a, f_input, g_right, f_inverted);
          gf.linkStates(gf_src,   // fromState
                        gf_trg,   // toState
                        gf_label, // symbol-pair, using f alphabet
                        f_wt + g_wt); // weight of transduction – composition adds weights!
        }
      }
      if(g_anywhere && g_src == g.initial) {
        // If g_anywhere, all g entries are optional – we always add
        // the transitions that were already in f:
        next = std::make_pair(f_trg, g_src);
        if (seen.find(next) == seen.end()) {
          todo.push_back(next);
        }
        if (states_f_g_gf.find(next) == states_f_g_gf.end()) {
          states_f_g_gf.insert({next, gf.newState()});
        }
        int gf_trg = states_f_g_gf[next];
        gf.linkStates(gf_src,  // fromState
                      gf_trg,  // toState
                      f_label, // symbol-pair, using f alphabet
                      f_wt);   // weight of transduction
      }
      // If f has an epsilon, also add a transition not to g.initial but g_src:
      if(f_output == 0) {      // will be the left if f_inverted
        next = std::make_pair(f_trg, g_src);
        if (seen.find(next) == seen.end()) {
          todo.push_back(next);
        }
        if (states_f_g_gf.find(next) == states_f_g_gf.end()) {
          states_f_g_gf.insert({next, gf.newState()});
        }
        int gf_trg = states_f_g_gf[next];
        gf.linkStates(gf_src,  // fromState
                      gf_trg,  // toState
                      f_label, // symbol-pair, using f alphabet
                      f_wt);   // weight of transduction
      }
    } // end loop arcs from f_src
  } // end while todo

  for(auto& it : states_f_g_gf)
  {
    int s_f = it.first.first;
    int s_g = it.first.second;
    int s_gf = it.second;
    if(isFinal(s_f) && (g.isFinal(s_g)
                        // if we're in anywhere mode, every state will be paired with g.initial if it's not paired with something in the middle of g
                        || (g_anywhere && g.initial == s_g)))
    {
      double wt_gf = finals[s_f] + (g.isFinal(s_g) ? g.finals.at(s_g)
                                                   : default_weight);
      gf.finals.insert({s_gf, wt_gf});
    }
  }

  // We do not minimize here, in order to let lt_compose print a warning
  // (instead of exiting the whole program) if no finals.
  return gf;
}
