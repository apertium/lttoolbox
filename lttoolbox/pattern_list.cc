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
#include <lttoolbox/pattern_list.h>
#include <lttoolbox/compression.h>
#include <lttoolbox/serialiser.h>
#include <lttoolbox/deserialiser.h>

#include <cstdlib>
#include <iostream>

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
  default_weight = o.default_weight;
}

void
PatternList::destroy()
{
}

PatternList::PatternList() :
sequence_id(0),
default_weight(0.0000)
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
    std::cerr << "Error: opening an unended sequence" << std::endl;
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
    std::cerr << "Error: ending an unopened sequence" << std::endl;
    exit(EXIT_FAILURE);
  }
  sequence = false;

  for(auto it = sequence_data.begin(),
      limit = sequence_data.end();
      it != limit; it++)
  {
    it->push_back(alphabet(QUEUE));
    patterns.insert({sequence_id, *it});
  }
}

void
PatternList::insertOutOfSequence(UStringView lemma, UStringView tags,
                                 std::vector<int> &result)
{
  if(lemma.empty())
  {
    result.push_back(alphabet(ANY_CHAR));
  }
  else
  {
    for(unsigned int i = 0, limit = lemma.size(); i < limit; i++)
    {
      if(lemma[i] == '*')
      {
        result.push_back(alphabet(ANY_CHAR));
      }
      else
      {
        result.push_back(static_cast<int32_t>(lemma[i]));
      }
    }
  }
  if(tags.empty())
  {
    result.push_back(alphabet(ANY_TAG));
  }
  else
  {
    for(unsigned int i = 0, limit = tagCount(tags); i < limit; i++)
    {
      UString tag = "<"_u + US(tagAt(tags, i)) + ">"_u;

      if(tag == u"<*>"_uv)
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
PatternList::insertIntoSequence(int id, UStringView lemma, UStringView tags)
{
  sequence_id = id;

  if(sequence_data.size() == 0)
  {
    std::vector<int> new_vector;
    insertOutOfSequence(lemma, tags, new_vector);
    sequence_data.push_back(new_vector);
  }
  else
  {
    auto it    = sequence_data.begin();
    auto limit = sequence_data.end();
    for(; it != limit; it++)
    {
      it->push_back('+');
      insertOutOfSequence(lemma, tags, *it);
    }
  }
}

void
PatternList::insert(int id, UStringView lemma, UStringView tags)
{
  if(!sequence)
  {
    std::vector<int> local;
    insertOutOfSequence(lemma, tags, local);
    local.push_back(alphabet(QUEUE));
    patterns.insert({id, local});
  }
  else
  {
    insertIntoSequence(id, lemma, tags);
  }
}

void
PatternList::insert(int id, int otherid)
{
  if(!sequence)
  {
    std::cerr << "Error: using labels outside of a sequence" << std::endl;
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
    std::list<std::vector<int>> new_sequence_data;

    for(auto it = sequence_data.begin(),
          limit = sequence_data.end(); it != limit; it++)
    {
      for(PatternRange p = patterns.equal_range(otherid);
          p.first != p.second; p.first++)
      {
        std::vector<int> temp = *it;
        temp.push_back('+');
        temp.insert(temp.end(), (p.first->second).begin(),
                    (p.first->second).end());
        new_sequence_data.push_back(temp);
      }
    }

    sequence_data = new_sequence_data;
  }
}

int
PatternList::tagCount(UStringView tags)
{
  int count = 0;

  for(unsigned int i = 0, limit = tags.size(); i < limit; i++)
  {
    if(i == 0)
    {
      count++;
    }
    else if(tags[i] == '.')
    {
      count++;
    }
  }

  return count;
}

UStringView
PatternList::tagAt(UStringView tags, int index)
{
  int start = 0;
  int end = 0;
  int count = 0;

  for(unsigned int i = 0, limit = tags.size(); i < limit; i++)
  {
    if(tags[i] == '.')
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
    return u"";
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
  for(auto it = patterns.begin(), limit = patterns.end();
      it != limit; it++)
  {
    int state = transducer.getInitial();
    int prevstate = -1;
    for(unsigned int i = 0, limit2 = it->second.size(); i != limit2; i++)
    {
      int const val = it->second[i];
      if(alphabet(ANY_CHAR) == val || alphabet(ANY_TAG) == val)
      {
        state = transducer.insertSingleTransduction(val, state, default_weight);
        if(prevstate != -1)
        {
          transducer.linkStates(prevstate, state, val, default_weight);
          prevstate = -1;
        }
        transducer.linkStates(state, state, val, default_weight);
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
        state = transducer.insertSingleTransduction(static_cast<int>('_'), state, default_weight);
        transducer.linkStates(prevstate, state, static_cast<int>(' '), default_weight);
        transducer.linkStates(prevstate, state, static_cast<int>('#'), default_weight);
        transducer.linkStates(state, state, alphabet(ANY_CHAR), default_weight);
      }
      else
      {
        state = transducer.insertSingleTransduction(val, state, default_weight);
        if(prevstate != -1)
        {
          transducer.linkStates(prevstate, state, val, default_weight);
          prevstate = -1;
        }
      }
    }
    if(prevstate != -1)
    {
      if(!transducer.isFinal(prevstate))
      {
        transducer.setFinal(prevstate, default_weight);
        final_type[prevstate] = it->first;
      }
    }
    if(!transducer.isFinal(state))
    {
      transducer.setFinal(state, default_weight);
      final_type[state] = it->first;
    }
  }
}

void
PatternList::write(FILE *output)
{
  alphabet.write(output);
  UStringView tagger_name = u"tagger";

  Compression::multibyte_write(1, output);
  Compression::string_write(tagger_name, output);
  transducer.write(output, alphabet.size());

  Compression::multibyte_write(final_type.size(), output);

  for(auto it = final_type.begin(), limit = final_type.end();
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
    UString mystr = Compression::string_read(input);
    transducer.read(input, alphabet.size());

    int finalsize = Compression::multibyte_read(input);
    for(; finalsize != 0; finalsize--)
    {
      int key = Compression::multibyte_read(input);
      final_type[key] = Compression::multibyte_read(input);
    }
  }
}

void
PatternList::serialise(std::ostream &serialised) const
{
  alphabet.serialise(serialised);
  transducer.serialise(serialised);
  Serialiser<std::map<int, int> >::serialise(final_type, serialised);
}

void
PatternList::deserialise(std::istream &serialised)
{
  alphabet.deserialise(serialised);
  transducer.deserialise(serialised);
  final_type = Deserialiser<std::map<int, int> >::deserialise(serialised);
}

MatchExe *
PatternList::newMatchExe() const
{
  return new MatchExe(transducer, final_type);
}

Alphabet &
PatternList::getAlphabet()
{
  return alphabet;
}

const Alphabet &
PatternList::getAlphabet() const
{
  return alphabet;
}
