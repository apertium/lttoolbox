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
#include <unicode/uchriter.h>
#include <lttoolbox/alphabet.h>
#include <lttoolbox/buffer.h>
#include <lttoolbox/my_stdio.h>
#include <lttoolbox/state.h>
#include <lttoolbox/trans_exe.h>
#include <lttoolbox/input_file.h>
#include <libxml/xmlreader.h>

#include <map>
#include <deque>
#include <queue>
#include <set>
#include <string>
#include <cstdint>

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
  std::map<UString, TransExe> transducers;

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
  double default_weight = 0.0000;

  /**
   * The final states of inconditional sections in the dictionaries
   */
  std::map<Node *, double> inconditional;

  /**
   * The final states of standard sections in the dictionaries
   */
  std::map<Node *, double> standard;

  /**
   * The final states of postblank sections in the dictionaries
   */
  std::map<Node *, double> postblank;

  /**
   * The final states of preblank sections in the dictionaries
   */
  std::map<Node *, double> preblank;

  /**
   * Merge of 'inconditional', 'standard', 'postblank' and 'preblank' sets
   */
  std::map<Node *, double> all_finals;

  /**
   * Queue of blanks, used in reading methods
   */
  std::queue<UString> blankqueue;

  /**
   * Queue of wordbound blanks, used in reading methods
   */
  std::deque<UString> wblankqueue;

  /**
   * Set of characters being considered alphabetics
   */
  std::set<UChar32> alphabetic_chars;

  /**
   * Set of characters to escape with a backslash
   */
  std::set<UChar32> escaped_chars;

  /**
   * Set of characters to ignore
   */
  std::set<UChar32> ignored_chars;

  /**
   * Mapping of characters for simplistic diacritic restoration specified in RCX files
   */
  std::map<int, std::set<int> > rcx_map;

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
  bool outOfWord = false;

  /**
   * true if we're automatically removing surface forms.
   */
  bool biltransSurfaceForms = false;


  /**
   * if true, makes always difference between uppercase and lowercase
   * characters
   */
  bool caseSensitive = false;

  /**
   * if true, uses the dictionary case, discarding surface case
   * information
   */
  bool dictionaryCase = false;

  /**
   * if true, flush the output when the null character is found
   */
  bool nullFlush = false;

  /**
   * nullFlush property for the skipUntil function
   */
  bool nullFlushGeneration = false;

  /**
   * if true, ignore the provided set of characters
   */
  bool useIgnoredChars = false;

  /**
   * if true, attempt simplistic diacritic restoration
   */
  bool useRestoreChars = false;

  /**
   * if true, skips loading the default set of ignored characters
   */
  bool useDefaultIgnoredChars = true;

  /**
   * if true, displays the final weights (if any)
   */
  bool displayWeightsMode = false;

  /**
   * try analysing unknown words as compounds
   */
  bool do_decomposition = false;

  /**
   * Symbol of CompoundOnlyL
   */
  int compoundOnlyLSymbol = 0;

  /**
   * Symbol of CompoundR
   */
  int compoundRSymbol = 0;

  /**
   * Show or not the controls symbols (as compoundRSymbol)
   */
  bool showControlSymbols = false;

  /**
   * Max compound elements
   * Hard coded for now, but there might come a switch one day
   */
  int compound_max_elements = 4;

  /**
   * Output no more than 'N' number of weighted analyses
   */
  int maxAnalyses = INT_MAX;

  bool transliteration_drop_tilde = false;
  std::set<unsigned int> wblank_locs;

  /**
   * True if a wblank block ([[..]]xyz[[/]]) was just read
   */
  bool is_wblank = false;

  /**
   * Output no more than 'N' best weight classes
   */
  int maxWeightClasses = INT_MAX;

  /**
   * Prints an error of input stream and exits
   */
  void streamError();

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
   * Read text from stream (transliteration version)
   * @param input the stream to read
   * @return the next symbol in the stream
   */
  int32_t readTransliteration(InputFile& input);

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
  std::pair<UString, int> readBilingual(InputFile& input, UFILE *output);

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
   * Calculate the initial state of parsing
   */
  void calcInitial();

  /**
   * Calculate all the results of the word being parsed
   */
  void classifyFinals();

  /**
   * Shortcut for filtering on all final states with current settings
   * Assumes that casefrom is non-empty
   */
  UString filterFinals(const State& state, const UString& casefrom);

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

  std::vector<UString> numbers;
  int readTMAnalysis(InputFile& input);

  unsigned int lastBlank(UString const &str);

  /**
   * Print one blankqueue item if there is one, or a given "space" value.
   *
   * @param val the space character to use if no blank queue
   * @param output stream where the word is written
   */
  void printSpace(UChar32 const val, UFILE *output, bool flush=true);
  /**
   * Print one possibly escaped character
   */
  void putc_esc(const UChar32 val, UFILE* output);
  /**
   * Print one possibly escaped character
   * if it's a space and the blank queue is non-empty,
   * pop the first blank and print that instead
   */
  void printChar(const UChar32 val, UFILE* output, bool flush=false);

  void skipUntil(InputFile& input, UFILE *output, UChar32 const character);
  static UString removeTags(UString const &str);
  UString compoundAnalysis(UString str);
  size_t firstNotAlpha(UString const &sf);

  void analysis_wrapper_null_flush(InputFile& input, UFILE *output);
  void bilingual_wrapper_null_flush(InputFile& input, UFILE *output, GenerationMode mode = gm_unknown);
  void generation_wrapper_null_flush(InputFile& input, UFILE *output,
                                     GenerationMode mode);

  UString compose(UString const &lexforms, UString const &queue) const;

  void procNodeICX();
  void procNodeRCX();
  void initDefaultIgnoredCharacters();

  bool isLastBlankTM = false;

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
  static UString const WBLANK_FINAL;

  FSTProcessor();

  void initAnalysis();
  void initTMAnalysis();
  void initSAO(){initAnalysis();};
  void initGeneration();
  void initPostgeneration();
  void initTransliteration();
  void initBiltrans();
  void initDecomposition();

  void analysis(InputFile& input, UFILE *output);
  void tm_analysis(InputFile& input, UFILE *output);
  void generation(InputFile& input, UFILE *output, GenerationMode mode = gm_unknown);
  void postgeneration(InputFile& input, UFILE *output);
  // intergeneration is a synonym for transliteration
  // retained for backwards compatibility
  void intergeneration(InputFile& input, UFILE *output);
  void transliteration(InputFile& input, UFILE *output);
  UString biltrans(UString const &input_word, bool with_delim = true);
  UString biltransfull(UString const &input_word, bool with_delim = true);
  void bilingual(InputFile& input, UFILE *output, GenerationMode mode = gm_unknown);
  std::pair<UString, int> biltransWithQueue(UString const &input_word, bool with_delim = true);
  UString biltransWithoutQueue(UString const &input_word, bool with_delim = true);
  void SAO(InputFile& input, UFILE *output);
  void parseICX(std::string const &file);
  void parseRCX(std::string const &file);

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
