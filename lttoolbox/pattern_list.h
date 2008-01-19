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
#ifndef _PATTERNLIST_
#define _PATTERNLIST_

#include <lttoolbox/alphabet.h>
#include <lttoolbox/match_exe.h>
#include <lttoolbox/transducer.h>

#include <list>
#include <map>
#include <string>
#include <vector>

using namespace std;

typedef multimap<int, vector<int> > PatternStore;
typedef pair<PatternStore::iterator, PatternStore::iterator> PatternRange;

class PatternList
{
private:
  Alphabet alphabet;  
  PatternStore patterns;
  bool sequence;
  list<vector<int> > sequence_data;
  Transducer transducer;
  map<int, int> final_type;
  int sequence_id;

  void copy(PatternList const &o);
  void destroy();
  void insertOutOfSequence(wstring const &lemma, wstring const &tags,
			   vector<int> &result);
  void insertIntoSequence(int const id, wstring const &lemma, 
			  wstring const &tags);

  static int tagCount(wstring const &tags);
  static wstring tagAt(wstring const &tags, int const index);
  
public:
  /**
   * This symbol stands for any char
   */
  static wstring const ANY_CHAR;

  /**
   * This symbol stands for any tag
   */
  static wstring const ANY_TAG;

  /**
   * This symbol marks a word queue
   */
  static wstring const QUEUE;

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
  void insert(int const id, wstring const &lemma, wstring const &tags);

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
  
  /**
   * Create a new MatchExe from PatternList, must be freed with 'delete'
   * @return the new MatchExe object
   */
  MatchExe * newMatchExe();
  
  /**
   * Get the alphabet of this PatternList object
   * @return the alphabet
   */
  Alphabet & getAlphabet();
};

#endif
