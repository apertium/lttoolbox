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
#include <lttoolbox/pattern_list.h>
#include <lttoolbox/compression.h>

#include <cstdlib>
#include <iostream>

wstring const PatternList::ANY_CHAR  = L"<ANY_CHAR>";
wstring const PatternList::ANY_TAG   = L"<ANY_TAG>";
wstring const PatternList::QUEUE = L"<QUEUE>";

void
PatternList::copy(PatternList const &o)
{
  sequence = o.sequence;
  sequence_data = o.sequence_data;
  patterns = o.patterns;
  alphabet = o.alphabet;
  transducer = o.transducer;
  final_type = o.final_type;
  sequence_id = o.sequence_id;
}

void
PatternList::destroy()
{
}

PatternList::PatternList()
{
  sequence = false;
  alphabet.includeSymbol(ANY_TAG);
  alphabet.includeSymbol(ANY_CHAR);
  alphabet.includeSymbol(QUEUE);
}

PatternList::~PatternList()
{
  destroy();
}

PatternList::PatternList(PatternList const &o)
{
  copy(o);
}

PatternList &
PatternList::operator =(PatternList const &o)
{
  if(this != &o)
  {
    destroy();
    copy(o);
  }
  return *this;
}

void
PatternList::beginSequence()
{
  if(sequence)
  {
    wcerr << L"Error: opening an unended sequence" << endl;
    exit(EXIT_FAILURE);
  }
  sequence = true;
  sequence_data.clear();
}

void
PatternList::endSequence()
{
  if(!sequence)
  {
    wcerr << L"Error: ending an unopened sequence" << endl;
    exit(EXIT_FAILURE);
  }
  sequence = false;
  
  for(list<vector<int> >::iterator it = sequence_data.begin(),
	limit = sequence_data.end();
      it != limit; it++)
  {
    it->push_back(alphabet(QUEUE));
    patterns.insert(pair<int, vector<int> >(sequence_id, *it));
  }
}

void
PatternList::insertOutOfSequence(wstring const &lemma, wstring const &tags,
				 vector<int> &result)
{
  if(lemma == L"")
  {
    result.push_back(alphabet(ANY_CHAR));
  }
  else
  {
    for(unsigned int i = 0, limit = lemma.size(); i < limit; i++)
    {
      if(lemma[i] == L'*')
      {
        result.push_back(alphabet(ANY_CHAR));
      }
      else
      { 
        result.push_back(int((unsigned char) lemma[i]));
      }
    }
  }
  if(tags == L"")
  {
    result.push_back(alphabet(ANY_TAG));
  }
  else
  {
    for(unsigned int i = 0, limit = tagCount(tags); i < limit; i++)
    {
      wstring tag = L"<" + tagAt(tags, i) + L">";
      
      if(tag == L"<*>")
      {
	result.push_back(alphabet(ANY_TAG));
      }
      else
      {
	alphabet.includeSymbol(tag);
	result.push_back(alphabet(tag));
      }
    } 
  }
}

void
PatternList::insertIntoSequence(int const id, wstring const &lemma, 
				wstring const &tags)
{
  sequence_id = id;

  if(sequence_data.size() == 0)
  {
    vector<int> new_vector;
    insertOutOfSequence(lemma, tags, new_vector);
    sequence_data.push_back(new_vector);
  }
  else
  {
    list<vector<int> >::iterator it    = sequence_data.begin();
    list<vector<int> >::iterator limit = sequence_data.end();
    for(; it != limit; it++)
    {
      it->push_back(L'+');
      insertOutOfSequence(lemma, tags, *it);
    }
  }
}

void
PatternList::insert(int const id, wstring const &lemma, wstring const &tags)
{
  if(!sequence)
  {
    vector<int> local;
    insertOutOfSequence(lemma, tags, local);
    local.push_back(alphabet(QUEUE));
    patterns.insert(pair<int, vector<int> >(id, local));
  }
  else
  {
    insertIntoSequence(id, lemma, tags);
  }
}

void
PatternList::insert(int const id, int const otherid)
{
  if(!sequence)
  {
    wcerr << L"Error: using labels outside of a sequence" << endl;
    exit(EXIT_FAILURE);
  }

  sequence_id = id;

  if(sequence_data.size() == 0)
  {
    PatternRange p = patterns.equal_range(otherid);
    for(; p.first != p.second; p.first++)
    {
      sequence_data.push_back(p.first->second);
    }
  }
  else
  {
    list<vector<int> > new_sequence_data;

    for(list<vector<int> >::iterator it = sequence_data.begin(),
	  limit = sequence_data.end(); it != limit; it++)
    {
      for(PatternRange p = patterns.equal_range(otherid);
	  p.first != p.second; p.first++)
      {
	vector<int> temp = *it;
	temp.push_back(L'+');
        temp.insert(temp.end(), (p.first->second).begin(),
		    (p.first->second).end());
	new_sequence_data.push_back(temp);
      }
    }

    sequence_data = new_sequence_data;
  }
}

int
PatternList::tagCount(wstring const &tags)
{
  int count = 0;

  for(unsigned int i = 0, limit = tags.size(); i < limit; i++)
  {
    if(i == 0)
    {
      count++;
    }
    else if(tags[i] == L'.')
    {
      count++;
    }
  }

  return count;
}

wstring
PatternList::tagAt(wstring const &tags, int const index)
{
  int start = 0;
  int end = 0;
  int count = 0;

  for(unsigned int i = 0, limit = tags.size(); i < limit; i++)
  {
    if(tags[i] == L'.')
    {
      count++;
      if(end == 0)
      {
	start = 0;
      }
      else
      {
	start = end + 1;
      }
      end = i;
    }
    if(count == index + 1)
    {
      return tags.substr(start, end - start);
    }
  }

  if(index > count)
  {
    return L"";
  }
  if(end != 0)
  {
    return tags.substr(end + 1);
  }
  else
  {
    return tags.substr(end);
  }
}

PatternStore const &
PatternList::getPatterns()
{
  return patterns;
}

void
PatternList::buildTransducer()
{
  for(PatternStore::const_iterator it = patterns.begin(), limit = patterns.end();
      it != limit; it++)
  {
    int state = transducer.getInitial();
    int prevstate = -1;
    for(unsigned int i = 0, limit2 = it->second.size(); i != limit2; i++)
    {
      int const val = it->second[i];
      if(alphabet(ANY_CHAR) == val || alphabet(ANY_TAG) == val)
      {
        state = transducer.insertSingleTransduction(val, state);
        if(prevstate != -1)
        {
          transducer.linkStates(prevstate, state, val);
          prevstate = -1;
        }
        transducer.linkStates(state, state, val);
      }
      else if(alphabet(QUEUE) == val)
      {
        if(prevstate != -1)
        {
          // ignore second (and next) possible consecutive queues
          continue;
        }
        
	// optional queue
        prevstate = state;
        state = transducer.insertSingleTransduction(static_cast<int>(L'_'), state);
        transducer.linkStates(prevstate, state, static_cast<int>(L' '));
        transducer.linkStates(prevstate, state, static_cast<int>(L'#'));
        transducer.linkStates(state, state, alphabet(ANY_CHAR));
      }
      else
      {
        state = transducer.insertSingleTransduction(val, state);
        if(prevstate != -1)
        {
          transducer.linkStates(prevstate, state, val);
          prevstate = -1;
        }
      }
    }
    if(prevstate != -1)
    {
      if(!transducer.isFinal(prevstate))
      {
        transducer.setFinal(prevstate);
        final_type[prevstate] = it->first;
      }
      prevstate = -1;
    }
    if(!transducer.isFinal(state))
    {
      transducer.setFinal(state);
      final_type[state] = it->first;
    }
  }
}  

void
PatternList::write(FILE *output)
{
  alphabet.write(output);
  wstring const tagger_name = L"tagger";

  Compression::multibyte_write(1, output);
  Compression::wstring_write(tagger_name, output);
  transducer.write(output, alphabet.size());

  Compression::multibyte_write(final_type.size(), output);

  for(map<int, int>::const_iterator it = final_type.begin(), limit = final_type.end();
      it != limit; it++)
  {
    Compression::multibyte_write(it->first, output);
    Compression::multibyte_write(it->second, output);
  }
}

void
PatternList::read(FILE *input)
{
  sequence = false;
  final_type.clear();
  
  alphabet.read(input);
  if(Compression::multibyte_read(input) == 1)
  {
    wstring mystr = Compression::wstring_read(input);
    transducer.read(input, alphabet.size());
    
    int finalsize = Compression::multibyte_read(input);
    for(; finalsize != 0; finalsize--)
    {
      int key = Compression::multibyte_read(input);
      final_type[key] = Compression::multibyte_read(input);
    }
  }
}

MatchExe *
PatternList::newMatchExe()
{
  return new MatchExe(transducer, final_type);
}

Alphabet &
PatternList::getAlphabet()
{
  return alphabet;
}
