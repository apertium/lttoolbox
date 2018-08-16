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
#include <lttoolbox/state.h>

#include <cstring>
#include <cwctype>
#include <climits>
#include <algorithm>

//debug//
//#include <iostream>
//using namespace std;
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
}

void
State::copy(State const &s)
{
  // release references
  for(size_t i = 0, limit = state.size(); i != limit; i++)
  {
    delete state[i].sequence;
  }

  state = s.state;

  for(size_t i = 0, limit = state.size(); i != limit; i++)
  {
    vector<pair<int, double>> *tmp = new vector<pair<int, double>>();
    *tmp = *(state[i].sequence);
    state[i].sequence = tmp;
  }
}

int
State::size() const
{
  return state.size();
}

void
State::init(Node *initial)
{
  state.clear();
  state.push_back(TNodeState(initial, new vector<pair<int, double>>(), false));
  state[0].sequence->clear();
  epsilonClosure();
}

void
State::apply(int const input)
{
  vector<TNodeState> new_state;
  if(input == 0)
  {
    state = new_state;
    return;
  }

  for(size_t i = 0, limit = state.size(); i != limit; i++)
  {
    map<int, Dest>::const_iterator it;
    it = state[i].where->transitions.find(input);
    if(it != state[i].where->transitions.end())
    {
      for(int j = 0; j != it->second.size; j++)
      {
        vector<pair<int, double>> *new_v = new vector<pair<int, double>>();
        *new_v = *(state[i].sequence);
        if(it->first != 0)
        {
          new_v->push_back(make_pair(it->second.out_tag[j], it->second.out_weight[j]));
        }
        new_state.push_back(TNodeState(it->second.dest[j], new_v, state[i].dirty||false));
      }
    }
    delete state[i].sequence;
  }

  state = new_state;
}

void
State::apply_override(int const input, int const old_sym, int const new_sym)
{
  vector<TNodeState> new_state;
  if(input == 0 || old_sym == 0)
  {
    state = new_state;
    return;
  }


  for(size_t i = 0, limit = state.size(); i != limit; i++)
  {
    map<int, Dest>::const_iterator it;
    it = state[i].where->transitions.find(input);
    if(it != state[i].where->transitions.end())
    {
      for(int j = 0; j != it->second.size; j++)
      {
        vector<pair<int, double>> *new_v = new vector<pair<int, double>>();
        *new_v = *(state[i].sequence);
        if(it->first != 0)
        {
          if(it->second.out_tag[j] == old_sym)
          {
            new_v->push_back(make_pair(new_sym, it->second.out_weight[j]));
          }
          else
          {
            new_v->push_back(make_pair(it->second.out_tag[j], it->second.out_weight[j]));
          }
        }
        new_state.push_back(TNodeState(it->second.dest[j], new_v, state[i].dirty||false));
      }
    }
    it = state[i].where->transitions.find(old_sym);
    if(it != state[i].where->transitions.end())
    {
      for(int j = 0; j != it->second.size; j++)
      {
        vector<pair<int, double>> *new_v = new vector<pair<int, double>>();
        *new_v = *(state[i].sequence);
        if(it->first != 0)
        {
          if(it->second.out_tag[j] == old_sym)
          {
            new_v->push_back(make_pair(new_sym, it->second.out_weight[j]));
          }
          else
          {
            new_v->push_back(make_pair(it->second.out_tag[j], it->second.out_weight[j]));
          }
        }
        new_state.push_back(TNodeState(it->second.dest[j], new_v, true));
      }
    }
    delete state[i].sequence;
  }

  state = new_state;
}



void
State::apply(int const input, int const alt)
{
  vector<TNodeState> new_state;
  if(input == 0 || alt == 0)
  {
    state = new_state;
    return;
  }


  for(size_t i = 0, limit = state.size(); i != limit; i++)
  {
    map<int, Dest>::const_iterator it;
    it = state[i].where->transitions.find(input);
    if(it != state[i].where->transitions.end())
    {
      for(int j = 0; j != it->second.size; j++)
      {
        vector<pair<int, double>> *new_v = new vector<pair<int, double>>();
        *new_v = *(state[i].sequence);
        if(it->first != 0)
        {
          new_v->push_back(make_pair(it->second.out_tag[j], it->second.out_weight[j]));
        }
        new_state.push_back(TNodeState(it->second.dest[j], new_v, state[i].dirty||false));
      }
    }
    it = state[i].where->transitions.find(alt);
    if(it != state[i].where->transitions.end())
    {
      for(int j = 0; j != it->second.size; j++)
      {
        vector<pair<int, double>> *new_v = new vector<pair<int, double>>();
        *new_v = *(state[i].sequence);
        if(it->first != 0)
        {
          new_v->push_back(make_pair(it->second.out_tag[j], it->second.out_weight[j]));
        }
        new_state.push_back(TNodeState(it->second.dest[j], new_v, true));
      }
    }
    delete state[i].sequence;
  }

  state = new_state;
}

void
State::apply_careful(int const input, int const alt)
{
  vector<TNodeState> new_state;
  if(input == 0 || alt == 0)
  {
    state = new_state;
    return;
  }


  for(size_t i = 0, limit = state.size(); i != limit; i++)
  {
    map<int, Dest>::const_iterator it;
    it = state[i].where->transitions.find(input);
    if(it != state[i].where->transitions.end())
    {
      for(int j = 0; j != it->second.size; j++)
      {
        vector<pair<int, double>> *new_v = new vector<pair<int, double>>();
        *new_v = *(state[i].sequence);
        if(it->first != 0)
        {
          new_v->push_back(make_pair(it->second.out_tag[j], it->second.out_weight[j]));
        }
        new_state.push_back(TNodeState(it->second.dest[j], new_v, state[i].dirty||false));
      }
    }
    else
    {
      it = state[i].where->transitions.find(alt);
      if(it != state[i].where->transitions.end())
      {
        for(int j = 0; j != it->second.size; j++)
        {
          vector<pair<int, double>> *new_v = new vector<pair<int, double>>();
          *new_v = *(state[i].sequence);
          if(it->first != 0)
          {
            new_v->push_back(make_pair(it->second.out_tag[j], it->second.out_weight[j]));
          }
          new_state.push_back(TNodeState(it->second.dest[j], new_v, true));
        }
      }
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
    map<int, Dest>::iterator it2;
    it2 = state[i].where->transitions.find(0);
    if(it2 != state[i].where->transitions.end())
    {
      for(int j = 0 ; j != it2->second.size; j++)
      {
        vector<pair<int, double>> *tmp = new vector<pair<int, double>>();
        *tmp = *(state[i].sequence);
        if(it2->second.out_tag[j] != 0)
        {
          tmp->push_back(make_pair(it2->second.out_tag[j], it2->second.out_weight[j]));
        }
        state.push_back(TNodeState(it2->second.dest[j], tmp, state[i].dirty));
      }
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

  for(size_t i = 0, limit = state.size(); i != limit; i++)
  {
    map<int, Dest>::const_iterator it;
    it = state[i].where->transitions.find(input);
    if(it != state[i].where->transitions.end())
    {
      for(int j = 0; j != it->second.size; j++)
      {
        vector<pair<int, double>> *new_v = new vector<pair<int, double>>();
        *new_v = *(state[i].sequence);
        if(it->first != 0)
        {
          new_v->push_back(make_pair(it->second.out_tag[j], it->second.out_weight[j]));
        }
        new_state.push_back(TNodeState(it->second.dest[j], new_v, state[i].dirty||false));
      }
    }
    it = state[i].where->transitions.find(alt1);
    if(it != state[i].where->transitions.end())
    {
      for(int j = 0; j != it->second.size; j++)
      {
        vector<pair<int, double>> *new_v = new vector<pair<int, double>>();
        *new_v = *(state[i].sequence);
        if(it->first != 0)
        {
          new_v->push_back(make_pair(it->second.out_tag[j], it->second.out_weight[j]));
        }
        new_state.push_back(TNodeState(it->second.dest[j], new_v, true));
      }
    }
    it = state[i].where->transitions.find(alt2);
    if(it != state[i].where->transitions.end())
    {
      for(int j = 0; j != it->second.size; j++)
      {
        vector<pair<int, double>> *new_v = new vector<pair<int, double>>();
        *new_v = *(state[i].sequence);
        if(it->first != 0)
        {
          new_v->push_back(make_pair(it->second.out_tag[j], it->second.out_weight[j]));
        }
        new_state.push_back(TNodeState(it->second.dest[j], new_v, true));
      }
    }

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
    map<int, Dest>::const_iterator it;
    it = state[i].where->transitions.find(input);
    if(it != state[i].where->transitions.end())
    {
      for(int j = 0; j != it->second.size; j++)
      {
        vector<pair<int, double>> *new_v = new vector<pair<int, double>>();
        *new_v = *(state[i].sequence);
        if(it->first != 0)
        {
          new_v->push_back(make_pair(it->second.out_tag[j], it->second.out_weight[j]));
        }
        new_state.push_back(TNodeState(it->second.dest[j], new_v, state[i].dirty||false));
      }
    }
    for(set<int>::iterator sit = alts.begin(); sit != alts.end(); sit++)
    {
      it = state[i].where->transitions.find(*sit);
      if(it != state[i].where->transitions.end())
      {
        for(int j = 0; j != it->second.size; j++)
        {
          vector<pair<int, double>> *new_v = new vector<pair<int, double>>();
          *new_v = *(state[i].sequence);
          if(it->first != 0)
          {
            new_v->push_back(make_pair(it->second.out_tag[j], it->second.out_weight[j]));
          }
          new_state.push_back(TNodeState(it->second.dest[j], new_v, true));
        }
      }
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
State::step_case(wchar_t val, wchar_t val2, bool caseSensitive)
{
  if (!iswupper(val) || caseSensitive) {
    step(val, val2);
  } else if(val != towlower(val)) {
    step(val, towlower(val), val2);
  } else {
    step(val, val2);
  }
}


void
State::step_case(wchar_t val, bool caseSensitive)
{
  if (!iswupper(val) || caseSensitive) {
    step(val);
  } else {
    step(val, towlower(val));
  }
}


bool
State::isFinal(map<Node *, double> const &finals) const
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


vector<pair< wstring, double >>
State::NFinals(vector<pair<wstring, double>> lf, int maxAnalyses, int maxWeightClasses) const
{
  vector<pair<wstring, double>> result;

  sort(lf.begin(), lf.end(), sort_weights<wstring, double>());

  for(vector<pair<wstring, double> >::iterator it = lf.begin(); it != lf.end(); it++)
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


wstring
State::filterFinals(map<Node *, double> const &finals,
                    Alphabet const &alphabet,
                    set<wchar_t> const &escaped_chars,
                    bool display_weights, int max_analyses, int max_weight_classes,
                    bool uppercase, bool firstupper, int firstchar) const
{
  vector<pair< wstring, double >> response;

  wstring result = L"";
  double cost = 0.0000;

  for(size_t i = 0, limit = state.size(); i != limit; i++)
  {
    if(finals.find(state[i].where) != finals.end())
    {
      if(state[i].dirty)
      {
        result.clear();
        cost = 0.0000;
        unsigned int const first_char = result.size() + firstchar;
        for(size_t j = 0, limit2 = state[i].sequence->size(); j != limit2; j++)
        {
          if(escaped_chars.find(((*(state[i].sequence))[j]).first) != escaped_chars.end())
          {
            result += L'\\';
          }
          alphabet.getSymbol(result, ((*(state[i].sequence))[j]).first, uppercase);
          cost += ((*(state[i].sequence))[j]).second;
        }
        if(firstupper)
        {
          if(result[first_char] == L'~')
          {
            // skip post-generation mark
            result[first_char+1] = towupper(result[first_char+1]);
          }
          else
          {
            result[first_char] = towupper(result[first_char]);
          }
        }
      }
      else
      {
        result.clear();
        cost = 0.0000;
        for(size_t j = 0, limit2 = state[i].sequence->size(); j != limit2; j++)
        {
          if(escaped_chars.find(((*(state[i].sequence))[j]).first) != escaped_chars.end())
          {
            result += L'\\';
          }
          alphabet.getSymbol(result, ((*(state[i].sequence))[j]).first);
          cost += ((*(state[i].sequence))[j]).second;
        }
      }
      response.push_back(make_pair(result, cost));
    }
  }

  response = NFinals(response, max_analyses, max_weight_classes);

  result = L"";
  for(vector<pair<wstring, double>>::iterator it = response.begin(); it != response.end(); it++)
  {
    result += L'/';
    result += it->first;
    if(display_weights)
    {
      result += L"<W:";
      result += to_wstring(it->second);
      result += L">";
    }
  }

  return result;
}


set<pair<wstring, vector<wstring> > >
State::filterFinalsLRX(map<Node *, double> const &finals,
                       Alphabet const &alphabet,
                       set<wchar_t> const &escaped_chars,
                       bool uppercase, bool firstupper, int firstchar) const
{
  set<pair<wstring, vector<wstring> > > results;

  vector<wstring> current_result;
  wstring rule_id = L"";

  for(size_t i = 0, limit = state.size(); i != limit; i++)
  {
    if(finals.find(state[i].where) != finals.end())
    {
      current_result.clear();
      rule_id = L"";
      wstring current_word = L"";
      for(size_t j = 0, limit2 = state[i].sequence->size(); j != limit2; j++)
      {
        if(escaped_chars.find(((*(state[i].sequence))[j]).first) != escaped_chars.end())
        {
          current_word += L'\\';
        }
        wstring sym = L"";
        alphabet.getSymbol(sym, ((*(state[i].sequence))[j]).first, uppercase);
        if(sym == L"<$>")
        {
          if(current_word != L"")
          {
            current_result.push_back(current_word);
          }
          current_word = L"";
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


wstring
State::filterFinalsSAO(map<Node *, double> const &finals,
                       Alphabet const &alphabet,
                       set<wchar_t> const &escaped_chars,
                       bool uppercase, bool firstupper, int firstchar) const
{
  wstring result = L"";
  wstring annot = L"";

  for(size_t i = 0, limit = state.size(); i != limit; i++)
  {
    if(finals.find(state[i].where) != finals.end())
    {
      result += L'/';
      unsigned int const first_char = result.size() + firstchar;
      for(size_t j = 0, limit2 = state[i].sequence->size(); j != limit2; j++)
      {
        if(escaped_chars.find(((*(state[i].sequence))[j]).first) != escaped_chars.end())
        {
          result += L'\\';
        }
        if(alphabet.isTag(((*(state[i].sequence))[j]).first))
        {
          annot = L"";
          alphabet.getSymbol(annot, ((*(state[i].sequence))[j]).first);
          result += L'&'+annot.substr(1,annot.length()-2)+L';';
        }
        else
        {
          alphabet.getSymbol(result, ((*(state[i].sequence))[j]).first, uppercase);
        }
      }
      if(firstupper)
      {
        if(result[first_char] == L'~')
        {
          // skip post-generation mark
          result[first_char+1] = towupper(result[first_char+1]);
        }
        else
        {
          result[first_char] = towupper(result[first_char]);
        }
      }
    }
  }

  return result;
}

wstring
State::filterFinalsTM(map<Node *, double> const &finals,
                      Alphabet const &alphabet,
                      set<wchar_t> const &escaped_chars,
                      queue<wstring> &blankqueue, vector<wstring> &numbers) const
{
  wstring result = L"";

  for(size_t i = 0, limit = state.size(); i != limit; i++)
  {
    if(finals.find(state[i].where) != finals.end())
    {
      result += L'/';
      for(size_t j = 0, limit2 = state[i].sequence->size(); j != limit2; j++)
      {
        if(escaped_chars.find((*(state[i].sequence))[j].first) != escaped_chars.end())
        {
          result += L'\\';
        }
        alphabet.getSymbol(result, (*(state[i].sequence))[j].first);
      }
    }
  }


  wstring result2 = L"";
  vector<wstring> fragment;
  fragment.push_back(L"");

  for(unsigned int i = 0, limit = result.size(); i != limit ; i++)
  {
    if(result[i] == L')')
    {
      fragment.push_back(L"");
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
      if(fragment[i].size() >=2 && fragment[i].substr(fragment[i].size()-2) == L"(#")
      {
        wstring whitespace = L" ";
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
          if(fragment[i].size()-j > 3 && fragment[i][j] == L'\\' &&
             fragment[i][j+1] == L'@' && fragment[i][j+2] == L'(')
          {
            int num = 0;
            bool correct = true;
            for(unsigned int k = (unsigned int) j+3, limit2 = fragment[i].size();
                k != limit2; k++)
            {
              if(iswdigit(fragment[i][k]))
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
          fragment[i] += L')';
        }
      }
    }
  }

  result = L"";

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
    vector<pair<int, double>> seq = *state.at(i).sequence;

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
State::restartFinals(const map<Node *, double> &finals, int requiredSymbol, State *restart_state, int separationSymbol)
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
            vector<pair<int, double>> *tnvec = new vector<pair<int, double>>;

            for(unsigned int k=0; k < state_i.sequence->size(); k++)
            {
              tnvec->push_back(state_i.sequence->at(k));
            }
            TNodeState tn(initst.where, tnvec, state_i.dirty);
            tn.sequence->push_back(make_pair(separationSymbol, 0.0000));
            state.push_back(tn);
          }
        }
      }
    }
  }
}



wstring
State::getReadableString(const Alphabet &a)
{
  wstring retval = L"[";

  for(unsigned int i=0; i<state.size(); i++)
  {
    vector<pair<int, double>>* seq = state.at(i).sequence;
    if(seq != NULL) for (unsigned int j=0; j<seq->size(); j++)
    {
      wstring ws = L"";
      a.getSymbol(ws, (seq->at(j)).first);
      retval.append(ws);
    }

    if(i+1 < state.size())
    {
      retval.append(L", ");
    }
  }
  retval.append(L"]");
  return retval;
}
