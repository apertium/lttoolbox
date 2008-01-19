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

#ifndef _FSTPROCESSOR_
#define _FSTPROCESSOR_

#include <lttoolbox/alphabet.h>
#include <lttoolbox/buffer.h>
#include <lttoolbox/ltstr.h>
#include <lttoolbox/my_stdio.h>
#include <lttoolbox/state.h>
#include <lttoolbox/trans_exe.h>

#include <cwchar>
#include <map>
#include <queue>
#include <set>
#include <string>

using namespace std;

/**
 * Kind of output of the generator module
 */
enum GenerationMode
{
  gm_clean,      // clear all
  gm_unknown,    // display unknown words, clear transfer and generation tags
  gm_all         // display all
};

/**
 * Class that implement the FST-based modules of the system
 */
class FSTProcessor
{
private:
  /**
   * Transducers in FSTP
   */
  map<wstring, TransExe, Ltstr> transducers;
  
  /**
   * Current state of lexical analysis
   */
  State current_state;
  
  /**
   * Initial state of every token
   */
  State initial_state;
  
  /**
   * Set of final states of incoditional sections in the dictionaries
   */
  set<Node *> inconditional;
  
  /**
   * Set of final states of standard sections in the dictionaries
   */
  set<Node *> standard;

  /**
   * Set of final states of postblank sections in the dictionaries
   */    
  set<Node *> postblank;
  
  /**
   * Merge of 'inconditional', 'standard' and 'postblank sets
   */
  set<Node *> all_finals;
   
  /**
   * Queue of blanks, used in reading methods 
   */
  queue<wstring> blankqueue;
  
  /**
   * Set of characters being considered alphabetics
   */
  set<wchar_t> alphabetic_chars;
  
  /**
   * Set of characters to escape with a backslash
   */
  set<wchar_t> escaped_chars;
  
  /**
   * Alphabet
   */
  Alphabet alphabet;
  
  /**
   * Input buffer
   */
  Buffer<int> input_buffer; 
  
  /**
   * Begin of the transducer
   */
  Node root;
  
  /**
   * true if the position of input stream is out of a word
   */
  bool outOfWord;
  
  
  /**
   * if true, makes always difference between uppercase and lowercase 
   * characters
   */
  bool caseSensitive;
  
  /**
   * Prints an error of input stream and exits
   */
  void streamError();
  
  /**
   * Reads a character that is defined in the set of escaped_chars
   * @param input the stream to read from
   * @return code of the character
   */
  wchar_t readEscaped(FILE *input);
  
  /**
   * Reads a block from the stream input, enclosed by delim1 and delim2
   * @param input the stream being read
   * @param delim1 the delimiter of the beginning of the sequence
   * @param delim1 the delimiter of the end of the sequence   
   */
  wstring readFullBlock(FILE *input, wchar_t const delim1, wchar_t const delim2);

  /**
   * Returns true if the character code is identified as alphabetic
   * @param c the code provided by the user
   * @return true if it's alphabetic
   */
  bool isAlphabetic(wchar_t const c) const;
  
  /**
   * Tests if a character is in the set of escaped_chars
   * @param c the character code provided by de user
   * @return true if it is in the set
   */
  bool isEscaped(wchar_t const c) const;

  /**
   * Read text from stream (analysis version, also used in postgeneration)
   * @param input the stream to read
   * @return the next symbol in the stream
   */
  int readAnalysis(FILE *input);

  /**
   * Read text from stream (postgeneration version)
   * @param input the stream to read
   * @return the next symbol in the stream
   */
  int readPostgeneration(FILE *input);

  /**
   * Read text from stream (generation version)
   * @param input the stream to read
   * @return the next symbol in the stream
   */
  int readGeneration(FILE *input, FILE *output);
  
  /**
   * Read text from stream (SAO version)
   * @param input the stream to read
   * @return the next symbol in the stream
   */  
  int readSAO(FILE *input);
  
  /**
   * Flush all the blanks remaining in the current process
   * @param output stream to write blanks
   */
  void flushBlanks(FILE *output);
  
  /**
   * Calculate the initial state of parsing
   */
  void calcInitial();
  
  /**
   * Calculate all the results of the word being parsed
   */
  void classifyFinals();
  
  /**
   * Write a string to an output stream, 
   * @param str the string to write scaping characters
   * @param output the stream to write in
   */
  void writeEscaped(wstring const &str, FILE *output);

  /**
   * Checks if an string ends with a partiuclar suffix
   * @param str the string to test
   * @param the searched suffix
   * @returns true if 'str' has the suffix 'suffix'
   */
  static bool endsWith(wstring const &str, wstring const &suffix);

  /**
   * Prints a word
   * @param sf surface form of the word
   * @param lf lexical form of the word
   * @param output stream where the word is written 
   */
  void printWord(wstring const &sf, wstring const &lf, FILE *output);

  /**
   * Prints a word, SAO version
   * @param lf lexical form
   * @param output stream where the word is written
   */
  void printSAOWord(wstring const &lf, FILE *output);

  /**
   * Prints an unknown word
   * @param sf surface form of the word
   * @param output stream where the word is written
   */
  void printUnknownWord(wstring const &sf, FILE *output);

  unsigned int lastBlank(wstring const &str);
  void printSpace(wchar_t const val, FILE *output);
  void skipUntil(FILE *input, FILE *output, wint_t const character);
  static wstring removeTags(wstring const &str);
  
public:
  FSTProcessor();
  ~FSTProcessor();

  void initAnalysis();
  void initSAO(){initAnalysis();};
  void initGeneration();
  void initPostgeneration();
  void initBiltrans();
  
  void analysis(FILE *input = stdin, FILE *output = stdout);
  void generation(FILE *input = stdin, FILE *output = stdout, GenerationMode mode = gm_unknown);
  void postgeneration(FILE *input = stdin, FILE *output = stdout); 
  void transliteration(FILE *input = stdin, FILE *output = stdout); 
  wstring biltrans(wstring const &input_word, bool with_delim = true);
  pair<wstring, int> biltransWithQueue(wstring const &input_word, bool with_delim = true);
  wstring biltransWithoutQueue(wstring const &input_word, bool with_delim = true);
  void SAO(FILE *input = stdin, FILE *output = stdout);  
  
  void load(FILE *input);

  bool valid() const;
  
  void setCaseSensitiveMode(bool const value);
};

#endif
