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
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */
#ifndef _MYATT_COMPILER_
#define _MYATT_COMPILER_

#include <string>
#include <fstream>
#include <sstream>
#include <map>
#include <set>
#include <vector>

#include <lttoolbox/alphabet.h>
#include <lttoolbox/transducer.h>
#include <lttoolbox/compression.h>

#include <cstdlib>

#define UNDECIDED 0
#define WORD      1
#define PUNCT     2
#define BOTH      3

using namespace std;

/** Bitmask; 1 = WORD, 2 = PUNCT, 3 = BOTH. */
typedef unsigned int TransducerType;

namespace 
{
  /** Splits a string into fields. */
  vector<wstring>& split(const wstring& s, wchar_t delim, vector<wstring> &out) 
  {
      wistringstream ss(s);
      wstring item;
      while (getline(ss, item, delim)) 
      {
        out.push_back(item);
      }
      return out;
  }
  
  /** Converts a string to a number. Slow, but at this point I don't care. */
  int convert(const wstring& s) 
  {
    int ret;
    wistringstream ss(s);
    ss >> ret;
    return ret;
  }
};

/**
 * Converts transducers from AT&T text format to lt binary format.
 *
 * @note To ensure that the AT&T file is read correctly, set std::locale to an
 *       appropriate value. If your current locale is compatible with the
 *       encoding of the file, just set it by adding
 *       <tt>std::locale::global(locale(""));</tt> to your code.
 */
class AttCompiler 
{
public:
  /**
   * Constructor
   */
  AttCompiler();

  /**
   * Destructor
   */
  ~AttCompiler();

  /** Extracts the sub-transducer made of states of type @p type. */
  Transducer extract_transducer(TransducerType type);

  /**
   * Recursively fills @p transducer (and @p corr) -- helper method called by
   * extract_transducer().
   */
  void _extract_transducer(TransducerType type, int from,
                           Transducer& transducer, map<int, int>& corr,
                           set<int>& visited) ;

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
  void classify(int from, map<int, TransducerType>& visited, bool path,
                TransducerType type) ;



  /**
   * Reads the AT&T format file @p file_name. The transducer and the alphabet
   * are both cleared before reading the new file.
   */
  void parse(string const &file_name, wstring const &dir);

  /** Writes the transducer to @p file_name in lt binary format. */

  void write(FILE *fd) ;

private:

  /** The final state(s). */
  set<int> finals;
  /**
   * Id of the starting state. We assume it is the source state of the first
   * transduction in the file.
   */
  int starting_state;

  Alphabet alphabet;
  /** All non-multicharacter symbols. */
  set<wchar_t> letters;

  /** Used in AttNode. */
  struct Transduction 
  {
    int            to;
    wstring        upper;
    wstring        lower;
    int            tag;
    TransducerType type;

    Transduction(int to, wstring upper, wstring lower, int tag,
                 TransducerType type=UNDECIDED) :
      to(to), upper(upper), lower(lower), tag(tag), type(type) {}
  };

  /** A node in the transducer graph. */
  struct AttNode 
  {
    int                  id;
    vector<Transduction> transductions;

    AttNode(int id) : id(id) {}
  };

  /** Stores the transducer graph. */
  map<int, AttNode*> graph;

  /** Clears the data associated with the current transducer. */
  void clear();

  /**
   * Returns the Node that represents the state @id. If it does not exist,
   * creates it and inserts it into @c graph.
   */

  AttNode* get_node(int id)
  {
    AttNode* state;
  
    if (graph.find(id) != graph.end())
    {
      state = graph[id];
    }
    else
    {
      state = new AttNode(id);
      graph[id] = state;
    }
    return state;
  }

  /** 
   * Returns true for combining diacritics and modifier letters
   *
   */
  bool is_word_punct(wchar_t symbol);

  /** 
   * Converts symbols like @0@ to epsilon, @_SPACE_@ to space, etc.
   * @todo Are there other special symbols? If so, add them, and maybe use a map
   *       for conversion?
   */
  void convert_hfst(wstring& symbol);

  /**
   * Returns the code of the symbol in the alphabet. Run after convert_hfst has
   * run.
   *
   * Also adds all non-multicharacter symbols (letters) to the @p letters set.
   *
   * @return the code of the symbol, if @p symbol is multichar; its first (and
   *         only) character otherwise.
   */
  int symbol_code(const wstring& symbol);
};

#endif /* _MYATT_COMPILER_ */
