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
#ifndef _PATTERNLIST_
#define _PATTERNLIST_

#include <lttoolbox/alphabet.h>
#include <lttoolbox/match_exe.h>
#include <lttoolbox/transducer.h>

#include <list>
#include <map>
#include <string>
#include <vector>

typedef std::multimap<int, std::vector<int> > PatternStore;
typedef std::pair<PatternStore::iterator, PatternStore::iterator> PatternRange;

class PatternList
{
private:
  Alphabet alphabet;
  PatternStore patterns;
  bool sequence;
  std::list<std::vector<int> > sequence_data;
  Transducer transducer;
  std::map<int, int> final_type;
  int sequence_id;
  double default_weight;

  void copy(PatternList const &o);
  void destroy();
  void insertOutOfSequence(UString const &lemma, UString const &tags,
                           std::vector<int> &result);
  void insertIntoSequence(int const id, UString const &lemma,
                          UString const &tags);

  static int tagCount(UString const &tags);
  static UString tagAt(UString const &tags, int const index);

public:
  /**
   * This symbol stands for any char
   */
  static UString const ANY_CHAR;

  /**
   * This symbol stands for any tag
   */
  static UString const ANY_TAG;

  /**
   * This symbol marks a word queue
   */
  static UString const QUEUE;

  /**
   * Constructor
   */
  PatternList();

  /**
   * Destructor
   */
  ~PatternList();

  /**
   * Copy constructor
   */
  PatternList(PatternList const &o);

  /**
   * Assignment operator
   * @param o the object to be assigned
   */
  PatternList & operator =(PatternList const &o);

  /**
   * Marks the start of a multiple word sequence
   */
  void beginSequence();

  /**
   * Ends the multiple word sequence
   */
  void endSequence();

  /**
   * Insertion method
   * @param id
   * @param lemma
   * @param tags
   */
  void insert(int const id, UString const &lemma, UString const &tags);

  /**
   * Insertion method
   * @param id
   * @param otherid
   */
  void insert(int const id, int const otherid);

  /**
   * Get the PatternStore
   * @returns a PatternStore object
   */
  PatternStore const & getPatterns();

  /**
   * Build the FSM
   */
  void buildTransducer();

  /**
   * Read PatternList from a file
   * @param input the input stream
   */
  void read(FILE *input);

  /**
   * Write PatternList to a file
   * @param output the output stream
   */
  void write(FILE *output);

  void serialise(std::ostream &serialised) const;
  void deserialise(std::istream &serialised);

  /**
   * Create a new MatchExe from PatternList, must be freed with 'delete'
   * @return the new MatchExe object
   */
  MatchExe * newMatchExe() const;

  /**
   * Get the alphabet of this PatternList object
   * @return the alphabet
   */
  Alphabet & getAlphabet();
  const Alphabet & getAlphabet() const;
};

#endif
