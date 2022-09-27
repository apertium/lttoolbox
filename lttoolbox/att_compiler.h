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
 * along with this program; if not, see <https://www.gnu.org/licenses/>.
 */
#ifndef _MYATT_COMPILER_
#define _MYATT_COMPILER_

#include <string>
#include <fstream>
#include <map>
#include <set>
#include <vector>

#include <lttoolbox/ustring.h>
#include <lttoolbox/alphabet.h>
#include <lttoolbox/transducer.h>
#include <lttoolbox/compression.h>

#include <cstdlib>

#define UNDECIDED 0
#define WORD      1
#define PUNCT     2
#define BOTH      3

using namespace icu;

/** Bitmask; 1 = WORD, 2 = PUNCT, 3 = BOTH. */
typedef unsigned int TransducerType;

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
                           Transducer& transducer, std::map<int, int>& corr,
                           std::set<int>& visited) ;

  /**
   * Reads the AT&T format file @p file_name. The transducer and the alphabet
   * are both cleared before reading the new file.
   * If read_rl = true then the second tape is used as the input
   */
  void parse(std::string const &file_name, bool read_rl);

  /** Writes the transducer to @p file_name in lt binary format. */

  void write(FILE *fd) ;

  void setHfstSymbols(bool b);
  void setSplitting(bool b);

private:

  bool hfstSymbols = false;
  bool splitting = true;

  /** The final state(s). */
  std::map<int, double> finals;
  /**
   * Id of the starting state. We assume it is the source state of the first
   * transduction in the file.
   */
  int starting_state = 0;
  /**
   * Default value of weight of a transduction unless specified.
   */
  double default_weight = 0.0000;

  // how many phantom states have we created to split multichar symbols
  int phantom_count = 0;

  Alphabet alphabet;
  /** All non-multicharacter symbols. */
  std::set<UChar> letters;

  /** Used in AttNode. */
  struct Transduction
  {
    int            to;
    UString        upper;
    UString        lower;
    int            tag;
    double         weight;
    TransducerType type;

    Transduction(int to, UString upper, UString lower, int tag,
                 double weight, TransducerType type=UNDECIDED) :
      to(to), upper(upper), lower(lower), tag(tag), weight(weight), type(type) {}
  };

  /** A node in the transducer graph. */
  struct AttNode
  {
    int                  id;
    std::vector<Transduction> transductions;

    AttNode(int id) : id(id) {}
  };

  /** Stores the transducer graph. */
  std::map<int, AttNode*> graph;

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
  bool is_word_punct(UChar32 symbol);

  /**
   * Determines initial type of single transition
   *
   */
  void classify_single_transition(Transduction& t);

  void classify_forwards();
  TransducerType classify_backwards(int state, std::set<int>& path);

  /**
   * Converts symbols like @0@ to epsilon, @_SPACE_@ to space, etc.
   * @todo Are there other special symbols? If so, add them, and maybe use a map
   *       for conversion?
   */
  void convert_hfst(UString& symbol);

  // if a character should be in the alphabet, add it
  void update_alphabet(UChar32 c);
  // convert a string to a symbol code, splitting non-tag multichars
  void symbol_code(UStringView symbol, std::vector<int32_t>& split);
  void add_transition(int from, int to,
                      UStringView upper, UStringView lower,
                      double weight);
};

#endif /* _MYATT_COMPILER_ */
