/*
 * Copyright (C) 2005-2019 Universitat d'Alacant / Universidad de Alicante
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

#ifndef _FSTPROCESSOR_
#define _FSTPROCESSOR_

#include <lttoolbox/ustring.h>
#include <lttoolbox/alphabet.h>
#include <lttoolbox/buffer.h>
#include <lttoolbox/my_stdio.h>
#include <lttoolbox/state.h>
#include <lttoolbox/trans_exe.h>
#include <lttoolbox/input_file.h>
#include <libxml/xmlreader.h>

#include <map>
#include <queue>
#include <set>
#include <string>
#include <cstdint>

using namespace std;

/**
 * Kind of output of the generator module
 */
enum GenerationMode
{
  gm_clean,      // clear all
  gm_unknown,    // display unknown words, clear transfer and generation tags
  gm_all,        // display all
  gm_tagged,     // tagged generation
  gm_tagged_nm,  // clean tagged generation
  gm_carefulcase // try lowercase iff no uppercase
};

/**
 * Class that implements the FST-based modules of the system
 */
class FSTProcessor
{
private:
  /**
   * Transducers in FSTP
   */
  map<UString, TransExe> transducers;

  /**
   * Current state of lexical analysis
   */
  State current_state;

  /**
   * Initial state of every token
   */
  State initial_state;

  /**
   * Default value of weight unless specified
   */
  double default_weight;

  /**
   * The final states of inconditional sections in the dictionaries
   */
  map<Node *, double> inconditional;

  /**
   * The final states of standard sections in the dictionaries
   */
  map<Node *, double> standard;

  /**
   * The final states of postblank sections in the dictionaries
   */
  map<Node *, double> postblank;

  /**
   * The final states of preblank sections in the dictionaries
   */
  map<Node *, double> preblank;

  /**
   * Merge of 'inconditional', 'standard', 'postblank' and 'preblank' sets
   */
  map<Node *, double> all_finals;

  /**
   * Queue of blanks, used in reading methods
   */
  queue<UString> blankqueue;

  /**
   * Queue of wordbound blanks, used in reading methods
   */
  queue<UString> wblankqueue;

  /**
   * Set of characters being considered alphabetics
   */
  set<UChar32> alphabetic_chars;

  /**
   * Set of characters to escape with a backslash
   */
  set<UChar32> escaped_chars;

  /**
   * Set of characters to ignore
   */
  set<UChar32> ignored_chars;

  /**
   * Mapping of characters for simplistic diacritic restoration specified in RCX files
   */
  map<int, set<int> > rcx_map;

  /**
   * Original char being restored
   */
  int rcx_current_char;

  /**
   * Alphabet
   */
  Alphabet alphabet;

  /**
   * Input buffer
   */
  Buffer<int32_t> input_buffer;

  /**
   * Begin of the transducer
   */
  Node root;

  /**
   * true if the position of input stream is out of a word
   */
  bool outOfWord;

  /**
   * true if we're automatically removing surface forms.
   */
  bool biltransSurfaceForms;


  /**
   * if true, makes always difference between uppercase and lowercase
   * characters
   */
  bool caseSensitive;

  /**
   * if true, uses the dictionary case, discarding surface case
   * information
   */
  bool dictionaryCase;

  /**
   * if true, flush the output when the null character is found
   */
  bool nullFlush;

  /**
   * nullFlush property for the skipUntil function
   */
  bool nullFlushGeneration;

  /**
   * if true, ignore the provided set of characters
   */
  bool useIgnoredChars;

  /**
   * if true, attempt simplistic diacritic restoration
   */
  bool useRestoreChars;

  /**
   * if true, skips loading the default set of ignored characters
   */
  bool useDefaultIgnoredChars;

  /**
   * if true, displays the final weights (if any)
   */
  bool displayWeightsMode;

  /**
   * try analysing unknown words as compounds
   */
  bool do_decomposition;

  /**
   * Symbol of CompoundOnlyL
   */
  int compoundOnlyLSymbol;

  /**
   * Symbol of CompoundR
   */
  int compoundRSymbol;

  /**
   * Show or not the controls symbols (as compoundRSymbol)
   */
  bool showControlSymbols;

  /**
   * Max compound elements
   * Hard coded for now, but there might come a switch one day
   */
  int compound_max_elements;

  /**
   * Output no more than 'N' number of weighted analyses
   */
  int maxAnalyses;

  /**
   * True if a wblank block ([[..]]xyz[[/]]) was just read
   */
  bool is_wblank;

  /**
   * True if skip_mode is false and need to collect wblanks
   */
  bool collect_wblanks;

  /**
   * True if a wblank has been processed for postgen and we need an ending wblank
   */
  bool need_end_wblank;

  /**
   * Output no more than 'N' best weight classes
   */
  int maxWeightClasses;

  /**
   * Prints an error of input stream and exits
   */
  void streamError();

  /**
   * Reads a character that is defined in the set of escaped_chars
   * @param input the stream to read from
   * @return code of the character
   */
  UChar32 readEscaped(InputFile& input);

  /**
   * Reads a block from the stream input, enclosed by delim1 and delim2
   * @param input the stream being read
   * @param delim1 the delimiter of the beginning of the sequence
   * @param delim1 the delimiter of the end of the sequence
   */
  UString readFullBlock(InputFile& input, UChar32 const delim1, UChar32 const delim2);

  /**
   * Reads a wordbound blank from the stream input
   * @param input the stream being read
   */
  UString readWblank(InputFile& input);

  /**
   * Reads a wordbound blank (opening blank to closing blank) from the stream input -> [[...]]xyz[[/]]
   * @param input the stream being read
   * @param output the stream to write on
   * @return true if the word enclosed by the wordbound blank has a ~ for postgeneration activation
   */
  bool wblankPostGen(InputFile& input, UFILE *output);

  /**
   * Returns true if the character code is identified as alphabetic
   * @param c the code provided by the user
   * @return true if it's alphabetic
   */
  bool isAlphabetic(UChar32 const c) const;

  /**
   * Tests if a character is in the set of escaped_chars
   * @param c the character code provided by the user
   * @return true if it is in the set
   */
  bool isEscaped(UChar32 const c) const;

  /**
   * Read text from stream (analysis version)
   * @param input the stream to read
   * @return the next symbol in the stream
   */
  int readAnalysis(InputFile& input);

  /**
   * Read text from stream (decomposition version)
   * @param input the stream to read
   * @param output the stream to write on
   * @return the next symbol in the stream
   */
  int readDecomposition(InputFile& input, UFILE *output);

  /**
   * Read text from stream (postgeneration version)
   * @param input the stream to read
   * @param output the stream to write on
   * @return the next symbol in the stream
   */
  int readPostgeneration(InputFile& input, UFILE *output);

  /**
   * Read text from stream (generation version)
   * @param input the stream to read
   * @param output the stream being written to
   * @return the next symbol in the stream
   */
  int readGeneration(InputFile& input, UFILE *output);

  /**
   * Read text from stream (biltrans version)
   * @param input the stream to read
   * @param output the stream to write on
   * @return the queue of 0-symbols, and the next symbol in the stream
   */
  pair<UString, int> readBilingual(InputFile& input, UFILE *output);

  /**
   * Read text from stream (SAO version)
   * @param input the stream to read
   * @return the next symbol in the stream
   */
  int readSAO(InputFile& input);

  /**
   * Flush all the blanks remaining in the current process
   * @param output stream to write blanks
   */
  void flushBlanks(UFILE *output);

  /**
   * Flush all the wordbound blanks remaining in the current process
   * @param output stream to write blanks
   */
  void flushWblanks(UFILE *output);

  /**
   * Combine wordbound blanks in the queue and return them.
   *
   * May pop from 'wblankqueue' and set 'need_end_wblank' to true.
   *
   * If 'wblankqueue' (see which) is empty, we get an empty string,
   * otherwise we return a semicolon-separated combination of opening
   * wblanks in the queue. If there is only a closing wblank, we just
   * set need_end_wblank.
   *
   * @return final wblank string
  */
  UString combineWblanks();

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
   * @param str the string to write, escaping characters
   * @param output the stream to write in
   */
  void writeEscaped(UString const &str, UFILE *output);

  /**
   * Write a string to an output stream.
   * If we print a space, we may pop a space from blankqueue
   * immediately, otherwise it should be printed afterwards.
   *
   * @param str the string to write, escaping characters
   * @param output the stream to write in
   * @return how many blanks to pop and print after printing lu
   */
  size_t writeEscapedPopBlanks(UString const &str, UFILE *output);

  /**
   * Write a string to an output stream, escaping all escapable characters
   * but keeping symbols without escaping
   * @param str the string to write, escaping characters
   * @param output the stream to write in
   */
  void writeEscapedWithTags(UString const &str, UFILE *output);


  /**
   * Checks if an string ends with a particular suffix
   * @param str the string to test
   * @param the searched suffix
   * @returns true if 'str' has the suffix 'suffix'
   */
  static bool endsWith(UString const &str, UString const &suffix);

  /**
   * Prints a word
   * @param sf surface form of the word
   * @param lf lexical form of the word
   * @param output stream where the word is written
   */
  void printWord(UString const &sf, UString const &lf, UFILE *output);

  /**
   * Prints a word.
   * If we print a space, we may pop a space from blankqueue.
   *
   * @param sf surface form of the word
   * @param lf lexical form of the word
   * @param output stream where the word is written
   */
  void printWordPopBlank(UString const &sf, UString const &lf, UFILE *output);

  /**
   * Prints a word (Bilingual version)
   * @param sf surface form of the word
   * @param lf lexical form of the word
   * @param output stream where the word is written
   */
  void printWordBilingual(UString const &sf, UString const &lf, UFILE *output);


  /**
   * Prints a word, SAO version
   * @param lf lexical form
   * @param output stream where the word is written
   */
  void printSAOWord(UString const &lf, UFILE *output);

  /**
   * Prints an unknown word
   * @param sf surface form of the word
   * @param output stream where the word is written
   */
  void printUnknownWord(UString const &sf, UFILE *output);

  void initDecompositionSymbols();

  vector<UString> numbers;
  int readTMAnalysis(InputFile& input);

  unsigned int lastBlank(UString const &str);

  /**
   * Print one blankqueue item if there is one, or a given "space" value.
   *
   * @param val the space character to use if no blank queue
   * @param output stream where the word is written
   */
  void printSpace(UChar const val, UFILE *output);

  void skipUntil(InputFile& input, UFILE *output, UChar32 const character);
  static UString removeTags(UString const &str);
  UString compoundAnalysis(UString str, bool uppercase, bool firstupper);
  size_t firstNotAlpha(UString const &sf);

  void analysis_wrapper_null_flush(InputFile& input, UFILE *output);
  void bilingual_wrapper_null_flush(InputFile& input, UFILE *output, GenerationMode mode = gm_unknown);
  void generation_wrapper_null_flush(InputFile& input, UFILE *output,
                                     GenerationMode mode);
  void postgeneration_wrapper_null_flush(InputFile& input, UFILE *output);
  void intergeneration_wrapper_null_flush(InputFile& input, UFILE *output);
  void transliteration_wrapper_null_flush(InputFile& input, UFILE *output);

  UString compose(UString const &lexforms, UString const &queue) const;

  void procNodeICX();
  void procNodeRCX();
  void initDefaultIgnoredCharacters();

  bool isLastBlankTM;

  xmlTextReaderPtr reader;
public:

  /*
   * String constants
   */
  static UString const XML_TEXT_NODE;
  static UString const XML_COMMENT_NODE;
  static UString const XML_IGNORED_CHARS_ELEM;
  static UString const XML_RESTORE_CHAR_ELEM;
  static UString const XML_RESTORE_CHARS_ELEM;
  static UString const XML_VALUE_ATTR;
  static UString const XML_CHAR_ELEM;
  static UString const WBLANK_START;
  static UString const WBLANK_END;
  static UString const WBLANK_FINAL;

  FSTProcessor();

  void initAnalysis();
  void initTMAnalysis();
  void initSAO(){initAnalysis();};
  void initGeneration();
  void initPostgeneration();
  void initBiltrans();
  void initDecomposition();

  void analysis(InputFile& input, UFILE *output);
  void tm_analysis(InputFile& input, UFILE *output);
  void generation(InputFile& input, UFILE *output, GenerationMode mode = gm_unknown);
  void postgeneration(InputFile& input, UFILE *output);
  void intergeneration(InputFile& input, UFILE *output);
  void transliteration(InputFile& input, UFILE *output);
  UString biltrans(UString const &input_word, bool with_delim = true);
  UString biltransfull(UString const &input_word, bool with_delim = true);
  void bilingual(InputFile& input, UFILE *output, GenerationMode mode = gm_unknown);
  pair<UString, int> biltransWithQueue(UString const &input_word, bool with_delim = true);
  UString biltransWithoutQueue(UString const &input_word, bool with_delim = true);
  void SAO(InputFile& input, UFILE *output);
  void parseICX(string const &file);
  void parseRCX(string const &file);

  void load(FILE *input);

  bool valid() const;

  void setCaseSensitiveMode(bool const value);
  void setDictionaryCaseMode(bool const value);
  void setBiltransSurfaceForms(bool const value);
  void setIgnoredChars(bool const value);
  void setRestoreChars(bool const value);
  void setNullFlush(bool const value);
  void setUseDefaultIgnoredChars(bool const value);
  void setDisplayWeightsMode(bool const value);
  void setMaxAnalysesValue(int const value);
  void setMaxWeightClassesValue(int const value);
  bool getNullFlush();
  bool getDecompoundingMode();
};

#endif
