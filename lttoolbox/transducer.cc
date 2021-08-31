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
#include <lttoolbox/endian_util.h>
#include <lttoolbox/old_binary.h>

#include <cstdlib>
#include <iostream>
#include <vector>
#include <cstring>

UString const Transducer::HFST_EPSILON_SYMBOL_SHORT   = "@0@"_u;
UString const Transducer::HFST_EPSILON_SYMBOL_LONG    = "@_EPSILON_SYMBOL_@"_u;
// could extend the ""_u helper to include u""_u
// this is the only place that needs it
UString const Transducer::LTTB_EPSILON_SYMBOL         = UString(1, (UChar)0x3B5);
                                                   // = "ε"_u;
UString const Transducer::HFST_SPACE_SYMBOL           = "@_SPACE_@"_u;
UString const Transducer::HFST_TAB_SYMBOL             = "@_TAB_@"_u;
UString const Transducer::GROUP_SYMBOL                = "#"_u;
UString const Transducer::JOIN_SYMBOL                 = "+"_u;
UString const Transducer::ANY_TAG_SYMBOL              = "<ANY_TAG>"_u;
UString const Transducer::ANY_CHAR_SYMBOL             = "<ANY_CHAR>"_u;
UString const Transducer::LSX_BOUNDARY_SYMBOL         = "<$>"_u;
UString const Transducer::COMPOUND_ONLY_L_SYMBOL      = "<compound-only-L>"_u;
UString const Transducer::COMPOUND_R_SYMBOL           = "<compound-R>"_u;


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
      transitions[source].insert(make_pair(tag, make_pair(state, weight)));
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
  transitions[source].insert(make_pair(tag, make_pair(state, weight)));
  return state;
}

int
Transducer::insertTransducer(int const source, Transducer &t,
                            int const epsilon_tag)
{
  map<int, int> relation;

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
      transitions[relation[it.first]].insert(make_pair(it2.first,
                                                        make_pair(relation[it2.second.first],
                                                                  it2.second.second)));
    }
  }

  transitions[source].insert(make_pair(epsilon_tag,
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
    auto range = transitions[source].equal_range(tag);
    for(;range.first != range.second; range.first++)
    {
      if(range.first->first == tag && range.first->second.first == target)
      {
        return;
      }
    }
    // end of new code
    transitions[source].insert(make_pair(tag, make_pair(target, weight)));
  }
  else
  {
    cerr << "Error: Trying to link nonexistent states (" << source;
    cerr << ", " << target << ", " << tag << ")" << endl;
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
    cerr << "Setting initial state to final" << endl;
  }
*/
  if(value)
  {
    finals.insert(make_pair(state, weight));
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
Transducer::closure(int const state, int const epsilon_tag) const
{
  return closure(state, set<int>({epsilon_tag}));
}

set<int>
Transducer::closure(int const state, set<int> const &epsilon_tags) const
{
  set<int> nonvisited, result;

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
      } catch (out_of_range const &e) {
        // No transition from any of the epsilon_tags – this is fine
      }
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

    for(auto& it : finals)
    {
      linkStates(it.first, state, epsilon_tag, it.second);
    }

    finals.clear();
    finals.insert(make_pair(state, default_weight));
  }
  else if(finals.size() == 0)
  {
    cerr << "Error: empty set of final states" <<endl;
    exit(EXIT_FAILURE);
  }
}

bool
Transducer::isEmptyIntersection(set<int> const &s1, set<int> const &s2)
{

  if(s1.size() < s2.size())
  {
    for(auto& it : s1)
    {
      if(s2.count(it))
      {
        return false;
      }
    }
  }
  else
  {
    for(auto& it : s2)
    {
      if(s1.count(it))
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
  map<int, set<int> > Q_prime;
  map<set<int>, int> Q_prime_inv;

  map<int, multimap<int, pair<int, double> > > transitions_prime;

  unsigned int size_Q_prime = 0;
  Q_prime[0] = closure(initial, epsilon_tag);

  Q_prime_inv[Q_prime[0]] = 0;
  R[0].insert(0);

  int initial_prime = 0;
  map<int, double> finals_prime;

  if(isFinal(initial))
  {
    finals_prime.insert(make_pair(0, default_weight));
  }

  int t = 0;

  set<int> finals_state;
  for(auto& it : finals) {
    finals_state.insert(it.first);
  }

  while(size_Q_prime != Q_prime.size())
  {
    size_Q_prime = Q_prime.size();
    R[(t+1)%2].clear();

    for(auto& it : R[t])
    {
      if(!isEmptyIntersection(Q_prime[it], finals_state))
      {
        double w = default_weight;
        auto it3 = finals.find(it);
        if (it3 != finals.end()) {
          w = it3->second;
        }
        finals_prime.insert(make_pair(it, w));
      }

      map<pair<int, double>, set<int> > mymap;

      for(auto& it2 : Q_prime[it])
      {
        for(auto& it3 : transitions[it2])
        {
          if(it3.first != epsilon_tag)
          {
            auto c = closure(it3.second.first, epsilon_tag);

            for(auto& it4 : c)
            {
              mymap[make_pair(it3.first, it3.second.second)].insert(it4);
            }
          }
        }
      }

      // adding new states
      for(auto& it2 : mymap)
      {
        if(Q_prime_inv.find(it2.second) == Q_prime_inv.end())
        {
          int tag = Q_prime.size();
          Q_prime[tag] = it2.second;
          Q_prime_inv[it2.second] = tag;
          R[(t+1)%2].insert(Q_prime_inv[it2.second]);
          transitions_prime[tag].clear();
        }
        transitions_prime[it].insert(make_pair(it2.first.first,
                                                make_pair(Q_prime_inv[it2.second], it2.first.second)));
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
  finals.insert(make_pair(state, default_weight));
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
  finals.insert(make_pair(state, default_weight));
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
bool Transducer::weighted() {
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

  new_t.initial = OldBinary::read_int(input, true);
  int finals_size = OldBinary::read_int(input, true);

  int base = 0;
  double base_weight = default_weight;
  while(finals_size > 0)
  {
    finals_size--;

    base += OldBinary::read_int(input, true);
    if(read_weights)
    {
      base_weight = OldBinary::read_double(input, true);
    }
    new_t.finals.insert(make_pair(base, base_weight));
  }

  base = OldBinary::read_int(input, true);
  int number_of_states = base;
  int current_state = 0;
  while(number_of_states > 0)
  {
    int number_of_local_transitions = OldBinary::read_int(input, true);
    int tagbase = 0;
    while(number_of_local_transitions > 0)
    {
      number_of_local_transitions--;
      tagbase += OldBinary::read_int(input, true) - decalage;
      int state = (current_state + OldBinary::read_int(input, true)) % base;
      if(read_weights)
      {
        base_weight = OldBinary::read_double(input, true);
      }
      if(new_t.transitions.find(state) == new_t.transitions.end())
      {
        new_t.transitions[state].clear(); // force create
      }
      new_t.transitions[current_state].insert(make_pair(tagbase, make_pair(state, base_weight)));
    }
    number_of_states--;
    current_state++;
  }

  *this = new_t;
}

void
Transducer::read_mmap(FILE* in, Alphabet& alpha)
{
  char header[4]{};
  auto r = fread_unlocked(header, 1, 4, in);
  if (r == 4 && strncmp(header, HEADER_TRANSDUCER, 4) == 0) {
    auto features = read_le_64(in);
    if (features >= TDF_UNKNOWN) {
      throw std::runtime_error("Transducer has features that are unknown to this version of lttoolbox - upgrade!");
    }
  } else {
    throw std::runtime_error("Unable to read transducer header!");
  }

  read_le_64(in); // total size
  initial = read_le_64(in);
  uint64_t state_count = read_le_64(in);
  uint64_t final_count = read_le_64(in);
  uint64_t trans_count = read_le_64(in);

  if (transitions.size() > state_count) {
    transitions.clear();
    // if transitions.size() <= state_count, they'll get cleared
    // when we read in the offsets, so don't bother here
  }
  finals.clear();

  for (uint64_t i = 0; i < final_count; i++) {
    uint64_t s = read_le_64(in);
    double w = read_le_double(in);
    finals.insert(make_pair(s, w));
  }

  vector<uint64_t> offsets;
  offsets.reserve(state_count+1);
  for (uint64_t i = 0; i < state_count; i++) {
    transitions[i].clear();
    offsets.push_back(read_le_64(in));
  }
  offsets.push_back(read_le_64(in));

  uint64_t state = 0;
  for (uint64_t i = 0; i < trans_count; i++) {
    while (i == offsets[state+1]) {
      state++;
    }
    int32_t isym = read_le_s32(in);
    int32_t osym = read_le_s32(in);
    int32_t sym = alpha(isym, osym);
    uint64_t dest = read_le_64(in);
    double wght = read_le_double(in);
    transitions[state].insert(make_pair(sym, make_pair(dest, wght)));
  }
}

void
Transducer::write_mmap(FILE* out, const Alphabet& alpha)
{
  fwrite_unlocked(HEADER_TRANSDUCER, 1, 4, out);
  uint64_t features = 0;
  features |= TDF_WEIGHTS;
  features |= TDF_MMAP;
  write_le_64(out, features);

  uint64_t tr_count = 0;
  vector<uint64_t> offsets;
  offsets.reserve(transitions.size()+1);
  for (auto& it : transitions) {
    offsets.push_back(tr_count);
    tr_count += it.second.size();
  }
  offsets.push_back(tr_count);

  // TODO: which things should be smaller than u64?

  uint64_t total_size =
    ( transitions.size() + 1 +  // offset of each state
      (tr_count * 3) +          // each transition
      (finals.size() * 2) +     // final states
      4 );                      // initial state + length of each section

  write_le_64(out, total_size*8);       // number of bytes after this
  write_le_64(out, initial);            // initial state
  write_le_64(out, transitions.size()); // number of states
  write_le_64(out, finals.size());      // number of finals
  write_le_64(out, tr_count);           // number of transitions

  for (auto& it : finals) {
    write_le_64(out, it.first);
    write_le_double(out, it.second);
  }

  for (auto& it : offsets) {
    write_le_64(out, it);
  }

  for (auto& it : transitions) {
    // we want to make sure the transitions are sorted by input symbol
    map<int, set<int>> symbols;
    for (auto& it2 : it.second) {
      symbols[alpha.decode(it2.first).first].insert(it2.first);
    }
    for (auto& s_in : symbols) {
      for (auto& s : s_in.second) {
        auto range = it.second.equal_range(s);
        for (auto tr = range.first; tr != range.second; ++tr) {
          auto sym = alpha.decode(tr->first);
          write_le_s32(out, sym.first); // input symbol
          write_le_s32(out, sym.second); // output symbol
          write_le_64(out, tr->second.first); // destination
          write_le_double(out, tr->second.second); // weight
        }
      }
    }
  }
}

void
Transducer::serialise(std::ostream &serialised) const
{
  Serialiser<int>::serialise(initial, serialised);
  Serialiser<map<int, double> >::serialise(finals, serialised);
  Serialiser<map<int, multimap<int, pair<int, double> > > >::serialise(transitions, serialised);
}

void
Transducer::deserialise(std::istream &serialised)
{
  initial = Deserialiser<int>::deserialise(serialised);
  finals = Deserialiser<map<int, double> >::deserialise(serialised);
  transitions = Deserialiser<map<int, multimap<int, pair<int, double> > > >::deserialise(serialised);
}

void
Transducer::read_serialised(FILE* in)
{
  initial = OldBinary::read_int(in, false);
  for (uint64_t i = OldBinary::read_int(in, false); i > 0; i--) {
    int s = OldBinary::read_int(in, false);
    finals.insert(make_pair(s, OldBinary::read_double(in, false)));
  }
  for (uint64_t i = OldBinary::read_int(in, false); i > 0; i--) {
    int src = OldBinary::read_int(in, false);
    multimap<int, pair<int, double> > st;
    for (uint64_t j = OldBinary::read_int(in, false); j > 0; j--) {
      int sym = OldBinary::read_int(in, false);
      int dest = OldBinary::read_int(in, false);
      double w = OldBinary::read_double(in, false);
      st.insert(make_pair(sym, make_pair(dest, w)));
    }
    transitions.insert(make_pair(src, st));
  }
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

  map<int, multimap<int, pair<int, double> > > tmp_transitions;

  for(map<int, multimap<int, pair<int, double> > >::reverse_iterator it = transitions.rbegin(); it != transitions.rend(); it++)
  {
    auto aux = it->second;
    it->second.clear();
    for(auto& it2 : aux)
    {
      if(it2.second.first >= it->first)
      {
        transitions[it2.second.first].insert(make_pair(it2.first, make_pair(it->first, it2.second.second)));
      }
      else
      {
        tmp_transitions[it2.second.first].insert(make_pair(it2.first, make_pair(it->first, it2.second.second)));
      }
    }
    if(tmp_transitions.find(it->first) != tmp_transitions.end())
    {
      it->second.insert(tmp_transitions[it->first].begin(), tmp_transitions[it->first].end());
      tmp_transitions.erase(it->first);
    }
  }

  for(map<int, multimap<int, pair<int, double> > >::reverse_iterator it = tmp_transitions.rbegin(),
                                                                  limit = tmp_transitions.rend();
      it != limit; it++)
  {
    for(auto& it2 : it->second)
    {
      transitions[it->first].insert(make_pair(it2.first, it2.second));
    }
  }

  int tmp = initial;
  initial = finals.begin()->first;
  finals.clear();
  finals.insert(make_pair(tmp, default_weight));
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
  else if(hfst && symbol == " "_u)
  {
    symbol = HFST_SPACE_SYMBOL;
  }
  else if(hfst && symbol == "\t"_u)
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
      u_fprintf(output, "%f\t\n", it2.second.second);
    }
  }

  for(auto& it3 : finals)
  {
    u_fprintf(output, "%d\t%f\n", it3.first, it3.second);
  }
}

void
Transducer::show(Alphabet const &alphabet, UFILE *output, int const epsilon_tag) const
{
  return show(alphabet, output, epsilon_tag, false);
}

int
Transducer::getStateSize(int const state)
{
 set<int> states;
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
Transducer::recognise(UString pattern, Alphabet &a, FILE *err)
{
  bool accepted = false;
  set<int> states;

  auto myclosure1 = closure(getInitial(), 0);
  states.insert(myclosure1.begin(), myclosure1.end());
  // For each of the characters in the input string
  for(auto& it : pattern)
  {
    set<int> new_state;        //Transducer::closure(int const state, int const epsilon_tag)
    // For each of the current alive states
    //fprintf(err, "step: %ls %lc (%d)\n", pattern.c_str(), *it, sym);
    for(auto& it2 : states)
    {
      auto& p = transitions[it2];
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
          //cerr << "Before closure alives: " <<new_state.size() << endl;
          new_state.insert(myclosure.begin(), myclosure.end());
          //cerr << "After closure alives: " <<new_state.size() << endl;
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
  finals.insert(make_pair(insertTransducer(initial, t, epsilon_tag), default_weight));
}

Transducer
Transducer::appendDotStar(set<int> const &loopback_symbols, int const epsilon_tag)
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
          lemq.finals.insert(make_pair(this_lemqlast, default_weight));
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
        new_t.linkStates(new_src, new_trg, label, this_wt);

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
        lemq.linkStates(lemq_src, lemq_trg, label, this_wt);
        if(seen.find(make_pair(this_trg, this_trg)) == seen.end())
        {
          todo.push_front(make_pair(this_trg, this_trg));
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
    newlemq.finals.insert(make_pair(states_this_lemq[this_lemqlast], default_weight));
    newlemq.minimize();

    int group_start = new_t.newState();
    new_t.linkStates(states_this_new[last_tag], group_start, group_label, default_weight);

    // append newlemq into the group after last_tag:
    new_t.finals.insert(make_pair(new_t.insertTransducer(group_start, newlemq), default_weight));
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
  std::list<SearchState> todo;
  todo.push_front(initial);

  map<int, int> states_this_new;
  states_this_new.insert(make_pair(initial, new_t.initial));

  while(todo.size() > 0)
  {
    int this_src = todo.front();
    todo.pop_front();
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
        new_t.finals.insert(make_pair(
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

  for(auto& it : finals)
   {
     new_t.finals.insert(make_pair(states_this_new[it.first], it.second));
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
      cerr <<"Error: couldn't find "<<this_src<<","<<trimmer_src<<" in state map"<<endl;
      exit(EXIT_FAILURE);
    }
    int trimmed_src = states_this_trimmed[current];

    // First loop through _epsilon_ transitions of trimmer
    for(auto& trimmer_trans_it : trimmer.transitions.at(trimmer_src)) {
      int trimmer_label = trimmer_trans_it.first,
          trimmer_trg   = trimmer_trans_it.second.first;
      double trimmer_wt = trimmer_trans_it.second.second;
      UString trimmer_left;
      trimmer_a.getSymbol(trimmer_left, trimmer_a.decode(trimmer_label).first);

      if(trimmer_preplus == trimmer_src) {
        // Keep the old preplus state if it was set; equal to current trimmer state means unset:
        trimmer_preplus_next = trimmer_trg;
      }

      if(trimmer_left.empty())
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
      UString this_right;
      this_a.getSymbol(this_right, this_a.decode(this_label).second);

      if(this_right == JOIN_SYMBOL || this_right == LSX_BOUNDARY_SYMBOL)
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
                           this_wt); //weight of transduction
        if(this_right == LSX_BOUNDARY_SYMBOL && isFinal(this_trg))
        {
          trimmed.setFinal(trimmed_trg, default_weight);
        }
      }
      else if ( this_right == COMPOUND_ONLY_L_SYMBOL
                || this_right == COMPOUND_R_SYMBOL
                || this_right.empty() )
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
                           this_wt); //weight of transduction
      }
      else
      {
        // Loop through non-epsilon arcs from the live state of trimmer

        // If we see a hash/group, we may have to rewind our trimmer state first:
        if(this_right == GROUP_SYMBOL && trimmer_preplus != trimmer_src)
        {
          states_this_trimmed.insert(make_pair(make_pair(this_src, make_pair(trimmer_preplus,
                                                                             trimmer_preplus)),
                                               trimmed_src));
          trimmer_src = trimmer_preplus;
        }

        for(auto& trimmer_trans_it : trimmer.transitions.at(trimmer_src))
        {
          int trimmer_label = trimmer_trans_it.first,
              trimmer_trg   = trimmer_trans_it.second.first;
          UString trimmer_left;
          trimmer_a.getSymbol(trimmer_left, trimmer_a.decode(trimmer_label).first);

          if(trimmer_preplus == trimmer_src) {
            // Keep the old preplus state if it was set; equal to current trimmer state means unset:
            trimmer_preplus_next = trimmer_trg;
          }

          if(!trimmer_left.empty() && // we've already dealt with trimmer epsilons
             (this_right == trimmer_left ||
              (this_right == ((trimmer_left[0] == '<') ? ANY_TAG_SYMBOL : ANY_CHAR_SYMBOL))))
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
      trimmed.finals.insert(make_pair(s_trimmed, default_weight));
    }
  }

  // We do not minimize here, in order to let lt_trim print a warning
  // (instead of exiting the whole program) if no finals.
  return trimmed;
}

void
read_transducer_set(FILE* input, UString& letters, Alphabet& alpha,
                    map<UString, Transducer>& trans)
{
  fpos_t pos;
  bool mmap = false;
  if (fgetpos(input, &pos) == 0) {
    char header[4]{};
    auto r = fread_unlocked(header, 1, 4, input);
    if (r == 4 && strncmp(header, HEADER_LTTOOLBOX, 4) == 0) {
      auto features = read_le_64(input);
      if (features >= LTF_UNKNOWN) {
        throw std::runtime_error("FST has features that are unknown to this version of lttoolbox - upgrade!");
      }
      mmap = features & LTF_MMAP;
    }
    else {
      // Old binary format
      fsetpos(input, &pos);
    }
  }

  if (mmap) {
    // make copies of all the strings we get from StringWriter
    // because it gets deallocated when the function returns
    StringWriter sw;
    sw.read(input);

    // letters
    uint32_t s = read_le_32(input);
    uint32_t c = read_le_32(input);
    letters = UString{sw.get(s, c)};

    // symbols
    alpha.read_mmap(input, sw);

    uint64_t tr_count = read_le_64(input);
    for (uint64_t i = 0; i < tr_count; i++) {
      uint32_t s = read_le_32(input);
      uint32_t c = read_le_32(input);
      UString name = UString{sw.get(s, c)};
      trans[name].read_mmap(input, alpha);
    }
  } else {
    // letters
    OldBinary::read_ustr(input, letters, true);

    // symbols
    alpha.read(input);

    int len = OldBinary::read_int(input, true);

    while(len > 0) {
      UString name;
      OldBinary::read_ustr(input, name, true);
      trans[name].read(input);

      len--;
    }
  }
}

uint64_t
write_transducer_set(FILE* output, UString_view letters, Alphabet& alpha,
                     map<UString, Transducer>& trans,
                     bool skip_empty)
{
  fwrite_unlocked(HEADER_LTTOOLBOX, 1, 4, output);
  uint64_t features = 0;
  features |= LTF_MMAP;
  write_le_64(output, features);

  uint64_t transducer_count = trans.size();

  StringWriter sw;
  StringRef letter_loc = sw.add(letters);
  for (auto& it : alpha.getTags()) {
    sw.add(it);
  }
  for (auto& it : trans) {
    if (skip_empty && it.second.isEmpty()) {
      transducer_count--;
      continue;
    }
    sw.add(it.first);
  }
  sw.write(output);

  // letters
  write_le_32(output, letter_loc.start);
  write_le_32(output, letter_loc.count);

  // symbols
  alpha.write_mmap(output, sw);

  // transducers
  write_le_64(output, transducer_count);
  for (auto& it : trans) {
    if (skip_empty && it.second.isEmpty()) {
      continue;
    }
    cout << it.first << " " << it.second.size();
    cout << " " << it.second.numberOfTransitions() << endl;
    StringRef loc = sw.add(it.first);
    write_le_32(output, loc.start);
    write_le_32(output, loc.count);
    it.second.write_mmap(output, alpha);
  }

  return transducer_count;
}
