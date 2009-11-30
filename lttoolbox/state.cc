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
#include <lttoolbox/state.h>

#include <cstring>
#include <cwctype>

State::State(Pool<vector<int> > *p)
{
  pool = p;
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
  // release references
  for(size_t i = 0, limit = state.size(); i != limit; i++)
  {
    pool->release(state[i].sequence);
  }

  state.clear();
}

void
State::copy(State const &s)
{
  // release references
  for(size_t i = 0, limit = state.size(); i != limit; i++)
  {
    pool->release(state[i].sequence);
  }

  state = s.state;
  pool = s.pool;

  for(size_t i = 0, limit = state.size(); i != limit; i++)
  {
    vector<int> *tmp = pool->get();
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
  state.push_back(TNodeState(initial,pool->get(),false));
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
        vector<int> *new_v = pool->get();
        *new_v = *(state[i].sequence);
        if(it->first != 0)
        {
          new_v->push_back(it->second.out_tag[j]);
        }
        new_state.push_back(TNodeState(it->second.dest[j], new_v, state[i].dirty||false));
      }
    }
    pool->release(state[i].sequence);
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
        vector<int> *new_v = pool->get();
        *new_v = *(state[i].sequence);
        if(it->first != 0)
        {
          new_v->push_back(it->second.out_tag[j]);
        }
        new_state.push_back(TNodeState(it->second.dest[j], new_v, state[i].dirty||false));
      }
    }
    it = state[i].where->transitions.find(alt);
    if(it != state[i].where->transitions.end())
    {
      for(int j = 0; j != it->second.size; j++)
      {
        vector<int> *new_v = pool->get();
        *new_v = *(state[i].sequence);
        if(it->first != 0)
        {
          new_v->push_back(it->second.out_tag[j]);
        }
        new_state.push_back(TNodeState(it->second.dest[j], new_v, true));
      }
    }
    pool->release(state[i].sequence);
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
        vector<int> *tmp = pool->get();
        *tmp = *(state[i].sequence);
        if(it2->second.out_tag[j] != 0)
        {
	  tmp->push_back(it2->second.out_tag[j]);
        }
        state.push_back(TNodeState(it2->second.dest[j], tmp, state[i].dirty));
      }          
    }
  }
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

bool
State::isFinal(set<Node *> const &finals) const
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

wstring
State::filterFinals(set<Node *> const &finals, 
		    Alphabet const &alphabet,
		    set<wchar_t> const &escaped_chars,
		    bool uppercase, bool firstupper, int firstchar) const
{
  wstring result = L"";

  for(size_t i = 0, limit = state.size(); i != limit; i++)
  {
    if(finals.find(state[i].where) != finals.end())
    {
      if(state[i].dirty)
      {
        result += L'/';
        unsigned int const first_char = result.size() + firstchar;
        for(size_t j = 0, limit2 = state[i].sequence->size(); j != limit2; j++)
        {
          if(escaped_chars.find((*(state[i].sequence))[j]) != escaped_chars.end())
          {
            result += L'\\';
          }
          alphabet.getSymbol(result, (*(state[i].sequence))[j], uppercase);
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
        result += L'/';
        for(size_t j = 0, limit2 = state[i].sequence->size(); j != limit2; j++)
        {
          if(escaped_chars.find((*(state[i].sequence))[j]) != escaped_chars.end())
          {
            result += L'\\';
          }
          alphabet.getSymbol(result, (*(state[i].sequence))[j]);
        }
      }
    }
  }
  
  return result;
}

wstring
State::filterFinalsSAO(set<Node *> const &finals, 
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
        if(escaped_chars.find((*(state[i].sequence))[j]) != escaped_chars.end())
        {
          result += L'\\';
        }
        if(alphabet.isTag((*(state[i].sequence))[j]))
        {
          annot = L"";
          alphabet.getSymbol(annot, (*(state[i].sequence))[j]);
          result += L'&'+annot.substr(1,annot.length()-2)+L';';
        }
        else
        {
          alphabet.getSymbol(result, (*(state[i].sequence))[j], uppercase);
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
State::filterFinalsTM(set<Node *> const &finals, 
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
        if(escaped_chars.find((*(state[i].sequence))[j]) != escaped_chars.end())
        {
          result += L'\\';
        }
        alphabet.getSymbol(result, (*(state[i].sequence))[j]);
      }
    }
  }


  wstring result2 = L"";
  vector<wstring> fragmentos;
  fragmentos.push_back(L"");
 
  for(unsigned int i = 0, limit = result.size(); i != limit ; i++)
  {
    if(result[i] == L')')
    {
      fragmentos.push_back(L"");
    }
    else
    {
      fragmentos[fragmentos.size()-1] += result[i];
    }
  }
  
  for(unsigned int i = 0, limit = fragmentos.size(); i != limit; i++)
  {
    if(i != limit -1)
    {
      if(fragmentos[i].size() >=2 && fragmentos[i].substr(fragmentos[i].size()-2) == L"(#")
      {
        wstring whitespace = L" ";
        if(blankqueue.size() != 0)
	{
          whitespace = blankqueue.front().substr(1);
	  blankqueue.pop();
	  whitespace = whitespace.substr(0, whitespace.size() - 1);
        }  
        fragmentos[i] = fragmentos[i].substr(0, fragmentos[i].size()-2) +
	                whitespace;
      }
      else
      {
        bool sustituido = false;
	for(int j = fragmentos[i].size() - 1; j >= 0; j--)
	{
	  if(fragmentos[i].size()-j > 3 && fragmentos[i][j] == L'\\' && 
	     fragmentos[i][j+1] == L'@' && fragmentos[i][j+2] == L'(')
	  {
	    int num = 0;
	    bool correcto = true;
	    for(unsigned int k = (unsigned int) j+3, limit2 = fragmentos[i].size();
		k != limit2; k++)
	    {
	      if(iswdigit(fragmentos[i][k]))
	      {
		num = num * 10;
		num += (int) fragmentos[i][k] - 48;	
	      }
	      else
	      {
		correcto = false;
		break;
	      }
	    }
	    if(correcto)
	    {
	      fragmentos[i] = fragmentos[i].substr(0, j) + numbers[num - 1];
	      sustituido = true;
	      break;
	    }
	  }
	}
	if(sustituido == false)
	{
	  fragmentos[i] += L')';
	}
      }
    }    
  }
  
  result = L"";

  for(unsigned int i = 0, limit = fragmentos.size(); i != limit; i++)
  {
    result += fragmentos[i];
  }
  
  return result;
}
