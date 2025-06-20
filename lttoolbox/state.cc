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
#include <lttoolbox/state.h>
#include <lttoolbox/string_utils.h>

#include <cstring>
#include <climits>
#include <algorithm>

//debug//
//#include <iostream>
//debug//

State::State()
{
}

State::~State()
{
  destroy();
}

State::State(State const &s)
{
  copy(s);
}

State &
State::operator =(State const &s)
{
  if(this != &s)
  {
    destroy();
    copy(s);
  }

  return *this;
}

void
State::destroy()
{
  for(size_t i = 0, limit = state.size(); i != limit; i++)
  {
    delete state[i].sequence;
  }

  state.clear();

  for (auto& it : sequence_pool) {
    delete it;
  }
  sequence_pool.clear();
}

std::vector<std::pair<int, double>>*
State::new_sequence() {
  if (sequence_pool.empty()) {
    sequence_pool.push_back(new std::vector<std::pair<int, double>>());
  }
  auto ret = sequence_pool.back();
  sequence_pool.pop_back();
  return ret;
}

void
State::free_sequence(std::vector<std::pair<int, double>>* seq) {
  sequence_pool.push_back(seq);
}

void
State::copy(State const &s)
{
  // release references
  for(size_t i = 0, limit = state.size(); i != limit; i++)
  {
    delete state[i].sequence;
  }
  for (auto& it : sequence_pool) {
    delete it;
  }

  state = s.state;

  for(size_t i = 0, limit = state.size(); i != limit; i++)
  {
    auto tmp = new_sequence();
    *tmp = *(state[i].sequence);
    state[i].sequence = tmp;
  }
}

size_t
State::size() const
{
  return state.size();
}

void
State::init(Node *initial)
{
  state.clear();
  state.push_back(TNodeState(initial, new_sequence(), false));
  state[0].sequence->clear();
  epsilonClosure();
}

bool
State::apply_into(std::vector<TNodeState>* new_state, int const input, int index, bool dirty)
{
  std::map<int, Dest>::const_iterator it;
  it = state[index].where->transitions.find(input);
  if(it != state[index].where->transitions.end())
  {
    for(int j = 0; j != it->second.size; j++)
    {
      auto new_v = new_sequence();
      *new_v = *(state[index].sequence);
      if(it->first != 0)
      {
        new_v->push_back({it->second.out_tag[j], it->second.out_weight[j]});
      }
      new_state->push_back(TNodeState(it->second.dest[j], new_v, state[index].dirty||dirty));
    }
    return true;
  }
  return false;
}

bool
State::apply_into_override(std::vector<TNodeState>* new_state, int const input, int const old_sym, int const new_sym, int index, bool dirty)
{
  std::map<int, Dest>::const_iterator it;
  it = state[index].where->transitions.find(input);
  if(it != state[index].where->transitions.end())
  {
    for(int j = 0; j != it->second.size; j++)
    {
      auto new_v = new_sequence();
      *new_v = *(state[index].sequence);
      if(it->first != 0)
      {
        if(it->second.out_tag[j] == old_sym)
        {
          new_v->push_back({new_sym, it->second.out_weight[j]});
        }
        else
        {
          new_v->push_back({it->second.out_tag[j], it->second.out_weight[j]});
        }
      }
      new_state->push_back(TNodeState(it->second.dest[j], new_v, state[index].dirty||dirty));
    }
    return true;
  }
  return false;
}

void
State::apply(int const input)
{
  if(input == 0)
  {
    destroy();
    return;
  }

  std::vector<TNodeState> new_state;
  for(size_t i = 0, limit = state.size(); i != limit; i++)
  {
    apply_into(&new_state, input, i, false);
    free_sequence(state[i].sequence);
  }

  state = new_state;
}

void
State::apply_override(int const input, int const old_sym, int const new_sym)
{
  if(input == 0 || old_sym == 0)
  {
    destroy();
    return;
  }

  std::vector<TNodeState> new_state;
  for(size_t i = 0, limit = state.size(); i != limit; i++)
  {
    apply_into_override(&new_state, input, old_sym, new_sym, i, false);
    apply_into_override(&new_state, old_sym, old_sym, new_sym, i, true);
    free_sequence(state[i].sequence);
  }

  state = new_state;
}

void
State::apply_override(int const input, int const alt, int const old_sym, int const new_sym)
{
  if(input == alt)
  {
    apply_override(input, old_sym, new_sym);
    return;
  }

  if(input == 0 || old_sym == 0)
  {
    destroy();
    return;
  }

  std::vector<TNodeState> new_state;
  for(size_t i = 0, limit = state.size(); i != limit; i++)
  {
    apply_into_override(&new_state, input, old_sym, new_sym, i, false);
    apply_into_override(&new_state, alt, old_sym, new_sym, i, true);
    apply_into_override(&new_state, old_sym, old_sym, new_sym, i, true);
    free_sequence(state[i].sequence);
  }

  state = new_state;
}

void
State::apply(int const input, int const alt)
{
  if(input == 0 || alt == 0)
  {
    destroy();
    return;
  }

  std::vector<TNodeState> new_state;
  if(input == alt)
  {
    apply(input);
    return;
  }

  for(size_t i = 0, limit = state.size(); i != limit; i++)
  {
    apply_into(&new_state, input, i, false);
    apply_into(&new_state, alt, i, true);
    free_sequence(state[i].sequence);
  }

  state = new_state;
}

void
State::apply_careful(int const input, int const alt)
{
  if(input == 0 || alt == 0)
  {
    destroy();
    return;
  }

  std::vector<TNodeState> new_state;
  for(size_t i = 0, limit = state.size(); i != limit; i++)
  {
    if(!apply_into(&new_state, input, i, false))
    {
      apply_into(&new_state, alt, i, true);
    }
    free_sequence(state[i].sequence);
  }

  state = new_state;
}

void
State::epsilonClosure()
{
  for(size_t i = 0; i != state.size(); i++)
  {
    auto it2 = state[i].where->transitions.find(0);
    if(it2 != state[i].where->transitions.end())
    {
      for(int j = 0 ; j != it2->second.size; j++)
      {
        auto tmp = new_sequence();
        *tmp = *(state[i].sequence);
        if(it2->second.out_tag[j] != 0)
        {
          tmp->push_back({it2->second.out_tag[j], it2->second.out_weight[j]});
        }
        state.push_back(TNodeState(it2->second.dest[j], tmp, state[i].dirty));
      }
    }
  }
}

void
State::apply(int const input, int const alt1, int const alt2)
{
  std::vector<TNodeState> new_state;
  if(input == 0 || alt1 == 0 || alt2 == 0)
  {
    state = new_state;
    return;
  }

  if(input == alt1)
  {
    apply(input, alt2);
    return;
  }
  else if(input == alt2)
  {
    apply(input, alt1);
    return;
  }

  for(size_t i = 0, limit = state.size(); i != limit; i++)
  {
    apply_into(&new_state, input, i, false);
    apply_into(&new_state, alt1, i, true);
    apply_into(&new_state, alt2, i, true);
    free_sequence(state[i].sequence);
  }

  state = new_state;
}

void
State::apply(int const input, std::set<int> const alts)
{
  std::vector<TNodeState> new_state;
  bool has_null = false;
  for(auto sit = alts.begin(); sit != alts.end(); sit++)
  {
    if(*sit == 0)
    {
      has_null = true;
    }
  }
  if(input == 0 || has_null)
  {
    state = new_state;
    return;
  }

  for(size_t i = 0, limit = state.size(); i != limit; i++)
  {
    apply_into(&new_state, input, i, false);
    for(auto sit = alts.begin(); sit != alts.end(); sit++)
    {
      if(*sit == input) continue;
      apply_into(&new_state, *sit, i, true);
    }

    free_sequence(state[i].sequence);
  }

  state = new_state;
}

void
State::step(int const input)
{
  apply(input);
  epsilonClosure();
}

void
State::step(int const input, int const alt)
{
  apply(input, alt);
  epsilonClosure();
}

void
State::step_override(int const input, int const old_sym, int const new_sym)
{
  apply_override(input, old_sym, new_sym);
  epsilonClosure();
}

void
State::step_override(int const input, int const alt, int const old_sym, int const new_sym)
{
  apply_override(input, alt, old_sym, new_sym);
  epsilonClosure();
}

void
State::step_careful(int const input, int const alt)
{
  apply_careful(input, alt);
  epsilonClosure();
}

void
State::step(int const input, int const alt1, int const alt2)
{
  apply(input, alt1, alt2);
  epsilonClosure();
}

void
State::step(int const input, std::set<int> const alts)
{
  apply(input, alts);
  epsilonClosure();
}

void
State::step_case(UChar32 val, UChar32 val2, bool caseSensitive)
{
  if (!u_isupper(val) || caseSensitive) {
    step(val, val2);
  } else if(val != u_tolower(val)) {
    step(val, u_tolower(val), val2);
  } else {
    step(val, val2);
  }
}


void
State::step_case(UChar32 val, bool caseSensitive)
{
  if (!u_isupper(val) || caseSensitive) {
    step(val);
  } else {
    step(val, u_tolower(val));
  }
}


void
State::step_case_override(UChar32 val, bool caseSensitive)
{
  if (!u_isupper(val) || caseSensitive) {
    step(val);
  } else {
    step_override(val, u_tolower(val), u_tolower(val), val);
  }
}

void
State::step_optional(UChar32 val)
{
  if (val == 0) return;
  std::vector<TNodeState> new_state;
  for (size_t i = 0; i < state.size(); i++) {
    apply_into(&new_state, val, i, false);
  }
  new_state.swap(state);
  epsilonClosure();
  new_state.swap(state);
  state.insert(state.end(), new_state.begin(), new_state.end());
}

bool
State::isFinal(std::map<Node *, double> const &finals) const
{
  for(size_t i = 0, limit = state.size(); i != limit; i++)
  {
    if(finals.find(state[i].where) != finals.end())
    {
      return true;
    }
  }

  return false;
}


std::vector<std::pair< UString, double >>
State::NFinals(std::vector<std::pair<UString, double>> lf, int maxAnalyses, int maxWeightClasses) const
{
  std::vector<std::pair<UString, double>> result;

  sort(lf.begin(), lf.end(), sort_weights<UString, double>());

  for(auto it = lf.begin(); it != lf.end(); it++)
  {
    double last_weight = 0.0000;
    if(maxAnalyses > 0 && maxWeightClasses > 0)
    {
      result.push_back({it->first, it->second});
      maxAnalyses--;
      if(last_weight!=it->second)
      {
        maxWeightClasses--;
      }
    }
    else break;
  }
  return result;
}

void
State::filterFinalsArray(std::vector<UString>& result,
                         std::map<Node *, double> const &finals,
                         Alphabet const &alphabet,
                         std::set<UChar32> const &escaped_chars,
                         bool display_weights,
                         int max_analyses, int max_weight_classes,
                         bool uppercase, bool firstupper, int firstchar) const
{
  std::vector<std::pair< UString, double >> response;
  UString temp;
  double cost = 0.0000;

  for (auto& it : state) {
    auto fin = finals.find(it.where);
    if (fin == finals.end()) continue;
    temp.clear();
    cost = fin->second;
    for (auto& step : *(it.sequence)) {
      if (escaped_chars.find(step.first) != escaped_chars.end()) temp += '\\';
      alphabet.getSymbol(temp, step.first, it.dirty && uppercase);
      cost += step.second;
    }
    if (it.dirty && firstupper) {
      int loc = firstchar;
      if (temp[loc] == '~') loc++; // skip post-generation mark
      temp[loc] = u_toupper(temp[loc]);
    }
    response.push_back({temp, cost});
  }

  response = NFinals(response, max_analyses, max_weight_classes);

  result.clear();
  sorted_vector<UString> seen;
  for (auto& it : response) {
    if (!seen.insert(it.first).second) continue;
    result.push_back(it.first);
    if (display_weights) {
      UChar w[16]{};
      // if anyone wants a weight of 10000, this will not be enough
      u_sprintf(w, "<W:%f>", it.second);
      result.back() += w;
    }
  }
}

UString
State::filterFinals(std::map<Node *, double> const &finals,
                    Alphabet const &alphabet,
                    std::set<UChar32> const &escaped_chars,
                    bool display_weights, int max_analyses, int max_weight_classes,
                    bool uppercase, bool firstupper, int firstchar) const
{
  std::vector<UString> result;
  filterFinalsArray(result, finals, alphabet, escaped_chars, display_weights,
                    max_analyses, max_weight_classes, uppercase, firstupper,
                    firstchar);

  UString ret;
  for (auto& it : result) {
    ret += '/';
    ret += it;
  }

  return ret;
}


std::set<std::pair<UString, std::vector<UString> > >
State::filterFinalsLRX(std::map<Node *, double> const &finals,
                       Alphabet const &alphabet,
                       std::set<UChar32> const &escaped_chars,
                       bool uppercase, bool firstupper, int firstchar) const
{
  std::set<std::pair<UString, std::vector<UString> > > results;

  std::vector<UString> current_result;
  UString rule_id;

  for(size_t i = 0, limit = state.size(); i != limit; i++)
  {
    if(finals.find(state[i].where) != finals.end())
    {
      current_result.clear();
      rule_id.clear();
      UString current_word;
      for(size_t j = 0, limit2 = state[i].sequence->size(); j != limit2; j++)
      {
        if(escaped_chars.find(((*(state[i].sequence))[j]).first) != escaped_chars.end())
        {
          current_word += '\\';
        }
        UString sym;
        alphabet.getSymbol(sym, ((*(state[i].sequence))[j]).first, uppercase);
        if(sym == u"<$>"_uv)
        {
          if(!current_word.empty())
          {
            current_result.push_back(current_word);
          }
          current_word.clear();
        }
        else
        {
          current_word += sym;
        }
      }
      rule_id = current_word;
      results.insert({rule_id, current_result});
    }
  }

  return results;
}


UString
State::filterFinalsSAO(std::map<Node *, double> const &finals,
                       Alphabet const &alphabet,
                       std::set<UChar32> const &escaped_chars,
                       bool uppercase, bool firstupper, int firstchar) const
{
  UString result;
  UString annot;

  for(size_t i = 0, limit = state.size(); i != limit; i++)
  {
    if(finals.find(state[i].where) != finals.end())
    {
      result += '/';
      unsigned int const first_char = result.size() + firstchar;
      for(size_t j = 0, limit2 = state[i].sequence->size(); j != limit2; j++)
      {
        if(escaped_chars.find(((*(state[i].sequence))[j]).first) != escaped_chars.end())
        {
          result += '\\';
        }
        if(alphabet.isTag(((*(state[i].sequence))[j]).first))
        {
          annot.clear();
          alphabet.getSymbol(annot, ((*(state[i].sequence))[j]).first);
          result += '&';
          result += annot.substr(1,annot.length()-2);
          result += ';';
        }
        else
        {
          alphabet.getSymbol(result, ((*(state[i].sequence))[j]).first, uppercase);
        }
      }
      if(firstupper)
      {
        if(result[first_char] == '~')
        {
          // skip post-generation mark
          result[first_char+1] = u_toupper(result[first_char+1]);
        }
        else
        {
          result[first_char] = u_toupper(result[first_char]);
        }
      }
    }
  }

  return result;
}

UString
State::filterFinalsTM(std::map<Node *, double> const &finals,
                      Alphabet const &alphabet,
                      std::set<UChar32> const &escaped_chars,
                      std::queue<UString> &blankqueue, std::vector<UString> &numbers) const
{
  UString result;

  for(size_t i = 0, limit = state.size(); i != limit; i++)
  {
    if(finals.find(state[i].where) != finals.end())
    {
      result += '/';
      for(size_t j = 0, limit2 = state[i].sequence->size(); j != limit2; j++)
      {
        if(escaped_chars.find((*(state[i].sequence))[j].first) != escaped_chars.end())
        {
          result += '\\';
        }
        alphabet.getSymbol(result, (*(state[i].sequence))[j].first);
      }
    }
  }


  UString result2;
  std::vector<UString> fragment;
  fragment.push_back(""_u);

  for(unsigned int i = 0, limit = result.size(); i != limit ; i++)
  {
    if(result[i] == ')')
    {
      fragment.push_back(""_u);
    }
    else
    {
      fragment[fragment.size()-1] += result[i];
    }
  }

  for(unsigned int i = 0, limit = fragment.size(); i != limit; i++)
  {
    if(i != limit -1)
    {
      if(fragment[i].size() >=2 && StringUtils::endswith(fragment[i], u"(#"))
      {
        UString whitespace = " "_u;
        if(blankqueue.size() != 0)
        {
          whitespace = blankqueue.front().substr(1);
          blankqueue.pop();
          whitespace = whitespace.substr(0, whitespace.size() - 1);
        }
        fragment[i] = fragment[i].substr(0, fragment[i].size()-2) +
                        whitespace;
      }
      else
      {
        bool substitute = false;
        for(int j = fragment[i].size() - 1; j >= 0; j--)
        {
          if(fragment[i].size()-j > 3 && fragment[i][j] == '\\' &&
             fragment[i][j+1] == '@' && fragment[i][j+2] == '(')
          {
            int num = 0;
            bool correct = true;
            for(unsigned int k = (unsigned int) j+3, limit2 = fragment[i].size();
                k != limit2; k++)
            {
              if(u_isdigit(fragment[i][k]))
              {
                num = num * 10;
                num += (int) fragment[i][k] - 48;
              }
              else
              {
                correct = false;
                break;
              }
            }
            if(correct)
            {
              fragment[i] = fragment[i].substr(0, j) + numbers[num - 1];
              substitute = true;
              break;
            }
          }
        }
        if(substitute == false)
        {
          fragment[i] += ')';
        }
      }
    }
  }

  result.clear();

  for(unsigned int i = 0, limit = fragment.size(); i != limit; i++)
  {
    result += fragment[i];
  }

  return result;
}



void
State::pruneCompounds(int requiredSymbol, int separationSymbol, int compound_max_elements)
{
  int minNoOfCompoundElements = compound_max_elements;
  int *noOfCompoundElements = new int[state.size()];

  for(unsigned int i = 0; i<state.size(); i++)
  {
    std::vector<std::pair<int, double>> seq = *state.at(i).sequence;

    if(lastPartHasRequiredSymbol(seq, requiredSymbol, separationSymbol))
    {
      int this_noOfCompoundElements = 0;
      for (int j = seq.size()-2; j>0; j--) if ((seq.at(j)).first==separationSymbol) this_noOfCompoundElements++;
      noOfCompoundElements[i] = this_noOfCompoundElements;
      minNoOfCompoundElements = (minNoOfCompoundElements < this_noOfCompoundElements) ?
                        minNoOfCompoundElements : this_noOfCompoundElements;
    }
    else
    {
      noOfCompoundElements[i] = INT_MAX;
    }
  }

  // remove states with more than minimum number of compounds (or without the requiered symbol in the last part)
  auto it = state.begin();
  int i=0;
  while(it != state.end())
  {
    if(noOfCompoundElements[i] > minNoOfCompoundElements)
    {
      free_sequence((*it).sequence);
      it = state.erase(it);
    }
    else
    {
      it++;
    }
    i++;
  }

  delete[] noOfCompoundElements;
}



void
State::pruneStatesWithForbiddenSymbol(int forbiddenSymbol)
{
  auto it = state.begin();
  while(it != state.end())
  {
    std::vector<std::pair<int, double>> *seq = (*it).sequence;
    bool found = false;
    for(int i = seq->size()-1; i>=0; i--)
    {
      if((seq->at(i)).first == forbiddenSymbol)
      {
        i=-1;
        free_sequence((*it).sequence);
        it = state.erase(it);
        found = true;
      }
    }
    if(!found)
    {
      it++;
    }
  }
}


bool
State::hasSymbol(int requiredSymbol)
{
  for(size_t i = 0; i<state.size(); i++)
  {
    // loop through sequence – we can't just check that the last tag is cp-L, there may be other tags after it:
    std::vector<std::pair<int, double>>* seq = state.at(i).sequence;
    if(seq != NULL) for (unsigned int j=0; j<seq->size(); j++)
    {
      int symbol=(seq->at(j)).first;
      if(symbol == requiredSymbol)
      {
        return true;
      }
    }
  }
  return false;
}


bool
State::lastPartHasRequiredSymbol(const std::vector<std::pair<int, double>> &seq, int requiredSymbol, int separationSymbol)
{
  // state is final - it should be restarted it with all elements in stateset restart_state, with old symbols conserved
  bool restart=false;
  for(int n=seq.size()-1; n>=0; n--)
  {
    int symbol=(seq.at(n)).first;
    if(symbol==requiredSymbol)
    {
      restart=true;
      break;
    }
    if(symbol==separationSymbol)
    {
      break;
    }
  }
  return restart;
}


void
State::restartFinals(const std::map<Node *, double> &finals, int requiredSymbol, State *restart_state, int separationSymbol)
{

  for(unsigned int i=0;  i<state.size(); i++)
  {
    TNodeState state_i = state.at(i);
    // A state can be a possible final state and still have transitions

    if(finals.count(state_i.where) > 0)
    {
      bool restart = lastPartHasRequiredSymbol(*(state_i.sequence), requiredSymbol, separationSymbol);
      if(restart)
      {
        if(restart_state != NULL)
        {
          for(unsigned int j=0; j<restart_state->state.size(); j++)
          {
            TNodeState initst = restart_state->state.at(j);
            auto tnvec = new_sequence();
            tnvec->clear();

            for(unsigned int k=0; k < state_i.sequence->size(); k++)
            {
              tnvec->push_back(state_i.sequence->at(k));
            }
            TNodeState tn(initst.where, tnvec, state_i.dirty);
            tn.sequence->push_back({separationSymbol, 0.0});
            state.push_back(tn);
          }
        }
      }
    }
  }
}



UString
State::getReadableString(const Alphabet &a)
{
  UString retval;
  retval += '[';

  for(unsigned int i=0; i<state.size(); i++)
  {
    std::vector<std::pair<int, double>>* seq = state.at(i).sequence;
    if(seq != NULL) for (unsigned int j=0; j<seq->size(); j++)
    {
      UString ws;
      a.getSymbol(ws, (seq->at(j)).first);
      retval.append(ws);
    }

    if(i+1 < state.size())
    {
      retval += ',';
      retval += ' ';
    }
  }
  retval += ']';
  return retval;
}

void
State::merge(const State& other)
{
  for (auto& it : other.state) {
    auto tmp = new_sequence();
    *tmp = *(it.sequence);
    TNodeState ns(it.where, tmp, it.dirty);
    this->state.push_back(std::move(ns));
  }
}
