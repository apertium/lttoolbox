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

#include <cstring>
#include <algorithm>

//debug//
//#include <iostream>
//using namespace std;
//debug//

State::State()
{}

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
  copy(s);
  return *this;
}

void
State::destroy()
{
  for (auto& it : state) {
    delete it.sequence;
  }

  state.clear();
}

void
State::copy(State const &s)
{
  if (this == &s) {
    return;
  }
  destroy();

  state = s.state;

  for (auto& it : state) {
    TPath* tmp = new TPath();
    *tmp = *(it.sequence);
    it.sequence = tmp;
  }
}

int
State::size() const
{
  return state.size();
}

void
State::init(const set<TransducerExe*>& exes)
{
  destroy();
  for (auto& it : exes) {
    state.push_back(TNodeState(it, it->initial, new TPath(), false));
  }
  epsilonClosure();
}

bool
State::apply_into(std::vector<TNodeState>* new_state, const int32_t input,
                  int index, bool dirty)
{
  uint64_t start, end;
  bool any = false;
  TransducerExe* trans = state[index].where;
  trans->get_range(state[index].state, input, start, end);
  for (uint64_t i = start; i < end; i++) {
    TPath* new_v = new TPath();
    *new_v = *(state[index].sequence);
    if (input != 0) {
      new_v->push_back(make_pair(trans->transitions[i].osym,
                                 trans->transitions[i].weight));
    }
    new_state->push_back(TNodeState(trans, trans->transitions[i].dest, new_v,
                                    state[index].dirty || dirty));
    any = true;
  }
  return any;
}

bool
State::apply_into_override(std::vector<TNodeState>* new_state,
                           const int32_t input,
                           const int32_t old_sym, const int32_t new_sym,
                           int index, bool dirty)
{
  uint64_t start, end;
  bool any = false;
  TransducerExe* trans = state[index].where;
  trans->get_range(state[index].state, input, start, end);
  for (uint64_t i = start; i < end; i++) {
    TPath* new_v = new TPath();
    *new_v = *(state[index].sequence);
    if (input != 0) {
      int32_t s = trans->transitions[i].osym;
      if (s == old_sym) {
        s = new_sym;
      }
      new_v->push_back(make_pair(s, trans->transitions[i].weight));
    }
    new_state->push_back(TNodeState(trans, trans->transitions[i].dest, new_v,
                                    state[index].dirty || dirty));
    any = true;
  }
  return any;
}

void
State::apply(int const input)
{
  if(input == 0)
  {
    destroy();
    return;
  }

  vector<TNodeState> new_state;
  for(size_t i = 0, limit = state.size(); i != limit; i++)
  {
    apply_into(&new_state, input, i, false);
    delete state[i].sequence;
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

  vector<TNodeState> new_state;
  for(size_t i = 0, limit = state.size(); i != limit; i++)
  {
    apply_into_override(&new_state, input, old_sym, new_sym, i, false);
    apply_into_override(&new_state, old_sym, old_sym, new_sym, i, true);
    delete state[i].sequence;
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

  vector<TNodeState> new_state;
  for(size_t i = 0, limit = state.size(); i != limit; i++)
  {
    apply_into_override(&new_state, input, old_sym, new_sym, i, false);
    apply_into_override(&new_state, alt, old_sym, new_sym, i, true);
    apply_into_override(&new_state, old_sym, old_sym, new_sym, i, true);
    delete state[i].sequence;
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

  vector<TNodeState> new_state;
  if(input == alt)
  {
    apply(input);
    return;
  }

  for(size_t i = 0, limit = state.size(); i != limit; i++)
  {
    apply_into(&new_state, input, i, false);
    apply_into(&new_state, alt, i, true);
    delete state[i].sequence;
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

  vector<TNodeState> new_state;
  for(size_t i = 0, limit = state.size(); i != limit; i++)
  {
    if(!apply_into(&new_state, input, i, false))
    {
      apply_into(&new_state, alt, i, true);
    }
    delete state[i].sequence;
  }

  state = new_state;
}

void
State::epsilonClosure()
{
  for(size_t i = 0; i != state.size(); i++)
  {
    TransducerExe* trans = state[i].where;
    uint64_t start, end;
    trans->get_range(state[i].state, 0, start, end);
    for (uint64_t j = start; j < end; j++) {
      TPath* tmp = new TPath();
      *tmp = *(state[i].sequence);
      if (trans->transitions[j].osym != 0) {
        tmp->push_back(make_pair(trans->transitions[j].osym,
                                 trans->transitions[j].weight));
      }
      state.push_back(TNodeState(trans, trans->transitions[j].dest, tmp,
                                 state[i].dirty));
    }
  }
}

void
State::apply(int const input, int const alt1, int const alt2)
{
  vector<TNodeState> new_state;
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
    delete state[i].sequence;
  }

  state = new_state;
}

void
State::apply(int const input, set<int> const alts)
{
  vector<TNodeState> new_state;
  bool has_null = false;
  for(set<int>::iterator sit = alts.begin(); sit != alts.end(); sit++)
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
    for(set<int>::iterator sit = alts.begin(); sit != alts.end(); sit++)
    {
      if(*sit == input) continue;
      apply_into(&new_state, *sit, i, true);
    }

    delete state[i].sequence;
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
State::step(int const input, set<int> const alts)
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


bool
State::isFinal(const set<TransducerExe*>& finals) const
{
  for (auto& it : state) {
    if(finals.find(it.where) != finals.end() && it.where->is_final(it.state)) {
      return true;
    }
  }

  return false;
}


vector<pair< UString, double >>
State::NFinals(vector<pair<UString, double>> lf, int maxAnalyses, int maxWeightClasses) const
{
  vector<pair<UString, double>> result;

  sort(lf.begin(), lf.end(), sort_weights<UString, double>());

  for(vector<pair<UString, double> >::iterator it = lf.begin(); it != lf.end(); it++)
  {
    double last_weight = 0.0000;
    if(maxAnalyses > 0 && maxWeightClasses > 0)
    {
      result.push_back(make_pair(it->first, it->second));
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


UString
State::filterFinals(const set<TransducerExe*>& finals,
                    AlphabetExe const &alphabet,
                    set<UChar32> const &escaped_chars,
                    bool display_weights, int max_analyses, int max_weight_classes,
                    bool uppercase, bool firstupper, int firstchar) const
{
  vector<pair< UString, double >> response;

  UString result;
  double cost = 0.0000;

  for (auto& st : state) {
    if(finals.find(st.where) != finals.end() && st.where->is_final(st.state))
    {
      if(st.dirty)
      {
        result.clear();
        cost = 0.0000;
        unsigned int const first_char = result.size() + firstchar;
        for(size_t j = 0, limit2 = st.sequence->size(); j != limit2; j++)
        {
          if(escaped_chars.find(((*(st.sequence))[j]).first) != escaped_chars.end())
          {
            result += '\\';
          }
          alphabet.getSymbol(result, ((*(st.sequence))[j]).first, uppercase);
          cost += ((*(st.sequence))[j]).second;
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
      else
      {
        result.clear();
        cost = 0.0000;
        for(size_t j = 0, limit2 = st.sequence->size(); j != limit2; j++)
        {
          if(escaped_chars.find(((*(st.sequence))[j]).first) != escaped_chars.end())
          {
            result += '\\';
          }
          alphabet.getSymbol(result, ((*(st.sequence))[j]).first);
          cost += ((*(st.sequence))[j]).second;
        }
      }

      // Add the weight of the final state
      double temp;
      st.where->find_final(st.state, temp);
      cost += temp;
      response.push_back(make_pair(result, cost));
    }
  }

  response = NFinals(response, max_analyses, max_weight_classes);

  result.clear();
  for(vector<pair<UString, double>>::iterator it = response.begin(); it != response.end(); it++)
  {
    result += '/';
    result += it->first;
    if(display_weights)
    {
      UChar temp[16]{};
      // if anyone wants a weight of 10000, this will not be enough
      u_sprintf(temp, "<W:%f>", it->second);
      result += temp;
    }
  }

  return result;
}


set<pair<UString, vector<UString> > >
State::filterFinalsLRX(const set<TransducerExe*>& finals,
                       AlphabetExe const &alphabet,
                       set<UChar32> const &escaped_chars,
                       bool uppercase, bool firstupper, int firstchar) const
{
  set<pair<UString, vector<UString> > > results;

  vector<UString> current_result;
  UString rule_id;

  for (auto& st : state) {
    if(finals.find(st.where) != finals.end() && st.where->is_final(st.state))
    {
      current_result.clear();
      rule_id.clear();
      UString current_word;
      for(size_t j = 0, limit2 = st.sequence->size(); j != limit2; j++)
      {
        if(escaped_chars.find(((*(st.sequence))[j]).first) != escaped_chars.end())
        {
          current_word += '\\';
        }
        UString sym;
        alphabet.getSymbol(sym, ((*(st.sequence))[j]).first, uppercase);
        if(sym == "<$>"_u)
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
      results.insert(make_pair(rule_id, current_result));
    }
  }

  return results;
}


UString
State::filterFinalsSAO(const set<TransducerExe*>& finals,
                       AlphabetExe const &alphabet,
                       set<UChar32> const &escaped_chars,
                       bool uppercase, bool firstupper, int firstchar) const
{
  UString result;
  UString annot;

  for (auto& st : state) {
    if(finals.find(st.where) != finals.end() && st.where->is_final(st.state))
    {
      result += '/';
      unsigned int const first_char = result.size() + firstchar;
      for(size_t j = 0, limit2 = st.sequence->size(); j != limit2; j++)
      {
        if(escaped_chars.find(((*(st.sequence))[j]).first) != escaped_chars.end())
        {
          result += '\\';
        }
        if(alphabet.isTag(((*(st.sequence))[j]).first))
        {
          annot.clear();
          alphabet.getSymbol(annot, ((*(st.sequence))[j]).first);
          result += '&';
          result += annot.substr(1,annot.length()-2);
          result += ';';
        }
        else
        {
          alphabet.getSymbol(result, ((*(st.sequence))[j]).first, uppercase);
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
State::filterFinalsTM(const set<TransducerExe*>& finals,
                      AlphabetExe const &alphabet,
                      set<UChar32> const &escaped_chars,
                      queue<UString> &blankqueue, vector<UString> &numbers) const
{
  UString result;

  for (auto& st : state) {
    if(finals.find(st.where) != finals.end() && st.where->is_final(st.state))
    {
      result += '/';
      for(size_t j = 0, limit2 = st.sequence->size(); j != limit2; j++)
      {
        if(escaped_chars.find((*(st.sequence))[j].first) != escaped_chars.end())
        {
          result += '\\';
        }
        alphabet.getSymbol(result, (*(st.sequence))[j].first);
      }
    }
  }


  UString result2;
  vector<UString> fragment;
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
      if(fragment[i].size() >=2 && fragment[i].substr(fragment[i].size()-2) == "(#"_u)
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
    vector<pair<int, double>> seq = *state[i].sequence;

    if(lastPartHasRequiredSymbol(seq, requiredSymbol, separationSymbol))
    {
      int this_noOfCompoundElements = 0;
      for (int j = seq.size()-2; j>0; j--) if ((seq[j]).first==separationSymbol) this_noOfCompoundElements++;
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
  vector<TNodeState>::iterator it = state.begin();
  int i=0;
  while(it != state.end())
  {
    if(noOfCompoundElements[i] > minNoOfCompoundElements)
    {
      delete (*it).sequence;
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
  vector<TNodeState>::iterator it = state.begin();
  while(it != state.end())
  {
    vector<pair<int, double>> *seq = (*it).sequence;
    bool found = false;
    for(int i = seq->size()-1; i>=0; i--)
    {
      if((seq->at(i)).first == forbiddenSymbol)
      {
        i=-1;
        delete (*it).sequence;
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
State::lastPartHasRequiredSymbol(const vector<pair<int, double>> &seq, int requiredSymbol, int separationSymbol)
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
State::restartFinals(const set<TransducerExe*>& finals, int requiredSymbol, State *restart_state, int separationSymbol)
{
  for (auto& st : state) {
    // A state can be a possible final state and still have transitions

    if (finals.find(st.where) != finals.end() && st.where->is_final(st.state)) {
      bool restart = lastPartHasRequiredSymbol(*(st.sequence), requiredSymbol, separationSymbol);
      if(restart && restart_state != NULL) {
        for (auto& initst : restart_state->state) {
          TPath* tnvec = new TPath();
          for (auto& it : *(st.sequence)) {
            tnvec->push_back(it);
          }
          TNodeState tn(initst.where, initst.state, tnvec, st.dirty);
          tn.sequence->push_back(make_pair(separationSymbol, 0.0000));
          state.push_back(tn);
        }
      }
    }
  }
}



UString
State::getReadableString(const AlphabetExe &a)
{
  UString retval;
  retval += '[';

  for(unsigned int i=0; i<state.size(); i++)
  {
    vector<pair<int, double>>* seq = state.at(i).sequence;
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
