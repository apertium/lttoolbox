/*
 * Copyright (C) 2013 Dávid Márk Nemeskey
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

#include <lttoolbox/att_compiler.h>
#include <lttoolbox/alphabet.h>
#include <lttoolbox/transducer.h>
#include <lttoolbox/compression.h>

using namespace std;

AttCompiler::AttCompiler()
{
}

AttCompiler::~AttCompiler()
{
}

void 
AttCompiler::clear() 
{
  for (map<int, AttNode*>::const_iterator it = graph.begin(); it != graph.end();
      ++it) 
  {
    delete it->second;
  }
  graph.clear();
  alphabet = Alphabet();
}

/** 
 * Converts symbols like @0@ to epsilon, @_SPACE_@ to space, etc.
 * @todo Are there other special symbols? If so, add them, and maybe use a map
 *       for conversion?
 */
void 
AttCompiler::convert_hfst(wstring& symbol) 
{
  if (symbol == L"@0@" || symbol == L"ε") 
  {
    symbol = L"";
  } 
  else if (symbol == L"@_SPACE_@") 
  {
    symbol = L" ";
  }
}

/**
 * Returns the code of the symbol in the alphabet. Run after convert_hfst has
 * run.
 *
 * Also adds all non-multicharacter symbols (letters) to the @p letters set.
 *
 * @return the code of the symbol, if @p symbol is multichar; its first (and
 *         only) character otherwise.
 */
int 
AttCompiler::symbol_code(const wstring& symbol) 
{
  if (symbol.length() > 1) {
    alphabet.includeSymbol(symbol);
    return alphabet(symbol);
  } else if (symbol == L"") {
    return 0;
  } else if (iswpunct(symbol[0]) || iswspace(symbol[0])) {
    return symbol[0];
  } else {
    letters.insert(symbol[0]);
    return symbol[0];
  }
}

void 
AttCompiler::parse(string const &file_name, wstring const &dir) 
{
  clear();

  wifstream infile(file_name.c_str());  // TODO: error checking
  vector<wstring> tokens;
  wstring line;
  bool first_line = true;       // First line -- see below
  bool seen_input_symbol = false;

  while (getline(infile, line)) 
  {
    tokens.clear();
    int from, to;
    wstring upper, lower;

    if (line.length() == 0 && first_line) 
    {
      cerr << "Error: empty file '" << file_name << "'." << endl;
      exit(EXIT_FAILURE);
    }
    if (first_line && line.find(L"\t") == wstring::npos)
    {
      cerr << "Error: invalid format '" << file_name << "'." << endl;
      exit(EXIT_FAILURE);
    }

    /* Empty line. */
    if (line.length() == 0) 
    {
      continue;
    }
    split(line, L'\t', tokens);

    from = convert(tokens[0]);

    AttNode* source = get_node(from);
    /* First line: the initial state is of both types. */
    if (first_line) 
    {
      starting_state = from;
      first_line = false;
    }

    /* Final state. */
    if (tokens.size() <= 2) 
    {
      finals.insert(from);
    } 
    else 
    {
      to = convert(tokens[1]);
      if(dir == L"RL")
      {
        upper = tokens[3];
        lower = tokens[2];
      }
      else
      {
        upper = tokens[2];
        lower = tokens[3];
      }
      convert_hfst(upper);
      convert_hfst(lower);
      if(upper != L"")
      {
        seen_input_symbol = true; 
      }
      /* skip lines that have an empty left side and output 
         if we haven't seen an input symbol */
      if(upper == L"" && lower != L"" && !seen_input_symbol) 
      {
        continue;
      }
      int tag = alphabet(symbol_code(upper), symbol_code(lower));
      /* We don't read the weights, even if they are defined. */
      source->transductions.push_back(Transduction(to, upper, lower, tag));

      get_node(to);
    }
  }

  /* Classify the nodes of the graph. */
  map<int, TransducerType> classified;
  classify(starting_state, classified, false, BOTH);

  infile.close();
}

/** Extracts the sub-transducer made of states of type @p type. */
Transducer 
AttCompiler::extract_transducer(TransducerType type) 
{
  Transducer transducer;
  /* Correlation between the graph's state ids and those in the transducer. */
  map<int, int> corr;
  set<int> visited;

  corr[starting_state] = transducer.getInitial();
  _extract_transducer(type, starting_state, transducer, corr, visited);

  /* The final states. */
  bool noFinals = true;
  for (set<int>::const_iterator f = finals.begin(); f != finals.end(); ++f) 
  {
    if (corr.find(*f) != corr.end()) 
    {
      transducer.setFinal(corr[*f]);
      noFinals = false;
    }
  }

/*
  if(noFinals)
  {
    wcerr << L"No final states (" << type << ")" << endl;
    wcerr << L"  were:" << endl;
    wcerr << L"\t" ;
    for (set<int>::const_iterator f = finals.begin(); f != finals.end(); ++f) 
    {
      wcerr << *f << L" ";
    }
    wcerr << endl;
  }
*/
  return transducer;
}

/**
 * Recursively fills @p transducer (and @p corr) -- helper method called by
 * extract_transducer().
 */
void 
AttCompiler::_extract_transducer(TransducerType type, int from,
                         Transducer& transducer, map<int, int>& corr,
                         set<int>& visited) 
{
  if (visited.find(from) != visited.end()) 
  {
    return;
  } 
  else 
  {
    visited.insert(from);
  }

  AttNode* source = get_node(from);

  /* Is the source state new? */
  bool new_from = corr.find(from) == corr.end();
  int from_t, to_t;

  for (vector<Transduction>::const_iterator it = source->transductions.begin();
       it != source->transductions.end(); ++it) 
  {
    if ((it->type & type) != type) 
    {
      continue;  // Not the right type
    }
    /* Is the target state new? */
    bool new_to = corr.find(it->to) == corr.end();

    if (new_from) 
    {
      corr[from] = transducer.size() + (new_to ? 1 : 0);
    }
    from_t = corr[from];

    /* Now with the target state: */
    if (!new_to) 
    {
      /* We already know it, possibly by a different name: link them! */
      to_t = corr[it->to];
      transducer.linkStates(from_t, to_t, it->tag);
    } 
    else 
    {
      /* We haven't seen it yet: add a new state! */
      to_t = transducer.insertNewSingleTransduction(it->tag, from_t);
      corr[it->to] = to_t;
    }
    _extract_transducer(type, it->to, transducer, corr, visited);
  }  // for
}


/**
 * Classifies the edges of the transducer graphs recursively. It works like
 * this:
 * - the type of the starting state is BOTH (already set)
 * - in case of an epsilon move, the type of the target state is the same as
 *   that of the source
 * - the first non-epsilon transition determines the type of the whole path
 * - it is also the time from which we begin filling the @p visited set.
 *
 * @param from the id of the source state.
 * @param visited the ids of states visited by this path.
 * @param path are we in a path?
 */
void 
AttCompiler::classify(int from, map<int, TransducerType>& visited, bool path,
              TransducerType type) 
{
  AttNode* source = get_node(from);
  if (visited.find(from) != visited.end()) 
  {
    if (path && ( (visited[from] & type) == type) ) 
    {
      return;
    }
  }

  if (path) 
  {
    visited[from] |= type;
  }

  for (vector<Transduction>::iterator it = source->transductions.begin();
       it != source->transductions.end(); ++it) 
  {
    bool next_path = path;
    int  next_type = type;
    bool first_transition = !path && it->upper != L"";
    if (first_transition) 
    {
      /* First transition: we now know the type of the path! */
      bool upper_word  = (it->upper.length() == 1 &&
                          letters.find(it->upper[0]) != letters.end());
      bool upper_punct = (it->upper.length() == 1 && iswpunct(it->upper[0]));
      next_type = UNDECIDED;
      if (upper_word)  next_type |= WORD;
      if (upper_punct) next_type |= PUNCT;
      next_path = true;
    } 
    else 
    {
      /* Otherwise (not yet, already): target's type is the same as ours. */
      next_type = type;
    }
    it->type |= next_type;
    classify(it->to, visited, next_path, next_type);
  }  // for
}

/** Writes the transducer to @p file_name in lt binary format. */
void 
AttCompiler::write(FILE *output) 
{
//  FILE* output = fopen(file_name, "w");
  Transducer punct_fst = extract_transducer(PUNCT);

  /* Non-multichar symbols. */
  Compression::wstring_write(wstring(letters.begin(), letters.end()), output);
  /* Multichar symbols. */
  alphabet.write(output);
  /* And now the FST. */
  if(punct_fst.numberOfTransitions() == 0)
  {
    Compression::multibyte_write(1, output);
  }
  else
  {
    Compression::multibyte_write(2, output);
  }
  Compression::wstring_write(L"main@standard", output);
  Transducer word_fst = extract_transducer(WORD);
  word_fst.write(output);
  wcout << L"main@standard" << " " << word_fst.size();
  wcout << " " << word_fst.numberOfTransitions() << endl;
  Compression::wstring_write(L"final@inconditional", output);
  if(punct_fst.numberOfTransitions() != 0) 
  {
    punct_fst.write(output);
    wcout << L"final@inconditional" << " " << punct_fst.size();
    wcout << " " << punct_fst.numberOfTransitions() << endl;
  }
//  fclose(output);
}
