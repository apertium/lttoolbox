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

#include <deque>
#include <map>
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
 * How the translation memory matches input
 */
enum TranslationMemoryMode
{
  tm_punct,      // Require punctuation after a match
  tm_space,      // Require space or punctuation after a match
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

  std::deque<std::vector<int32_t>> transliteration_queue;

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
   * true if we're assuming input has surface forms.
   */
  bool biltransSurfaceForms = false;

  /**
   * true if we're assuming both input and output should have surface forms.
   */
  bool biltransSurfaceFormsKeep = false;


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

  /**
   * Output no more than 'N' best weight classes
   */
  int maxWeightClasses = INT_MAX;

  /**
   * Prints an error of input stream and exits
   */
  void streamError();

  /**
   * Write \0 to output and flush if at_null is true
   */
  void maybeFlush(UFILE* output, bool at_null);

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

  bool readTransliterationBlank(InputFile& input);
  bool readTransliterationWord(InputFile& input);

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
  UString filterFinals(const State& state, UStringView casefrom);

  /**
   * Write a string to an output stream,
   * @param str the string to write, escaping characters
   * @param output the stream to write in
   */
  void writeEscaped(UStringView str, UFILE *output);

  /**
   * Write a string to an output stream.
   * If we print a space, we may pop a space from blankqueue
   * immediately, otherwise it should be printed afterwards.
   *
   * @param str the string to write, escaping characters
   * @param output the stream to write in
   * @return how many blanks to pop and print after printing lu
   */
  size_t writeEscapedPopBlanks(UStringView str, UFILE *output);

  /**
   * Write a string to an output stream, escaping all escapable characters
   * but keeping symbols without escaping
   * @param str the string to write, escaping characters
   * @param output the stream to write in
   */
  void writeEscapedWithTags(UStringView str, UFILE *output);

  /**
   * Prints a word
   * @param sf surface form of the word
   * @param lf lexical form of the word
   * @param output stream where the word is written
   */
  void printWord(UStringView sf, UStringView lf, UFILE *output);

  /**
   * Prints a word.
   * If we print a space, we may pop a space from blankqueue.
   *
   * @param sf surface form of the word
   * @param lf lexical form of the word
   * @param output stream where the word is written
   */
  void printWordPopBlank(UStringView sf, UStringView lf, UFILE *output);

  /**
   * Prints a word, SAO version
   * @param lf lexical form
   * @param output stream where the word is written
   */
  void printSAOWord(UStringView lf, UFILE *output);

  /**
   * Prints an unknown word
   * @param sf surface form of the word
   * @param output stream where the word is written
   */
  void printUnknownWord(UStringView sf, UFILE *output);

  void initDecompositionSymbols();

  std::vector<UString> numbers;
  int readTMAnalysis(InputFile& input);

  unsigned int lastBlank(UStringView str);

  /**
   * Print one blankqueue item if there is one, or a given "space" value.
   *
   * @param val the space character to use if no blank queue
   * @param output stream where the word is written
   */
  void printSpace(UChar32 val, UFILE *output);
  /**
   * Print one possibly escaped character
   * if it's a space and the blank queue is non-empty,
   * pop the first blank and print that instead
   */
  void printChar(UChar32 val, UFILE* output);

  static UStringView removeTags(UStringView str);
  UString compoundAnalysis(UString str);

  struct Indices {
        size_t i_codepoint;
        size_t i_utf16; // always >= i_codepoint since some codepoints take up 2 UTF-16's
  };

  /*
   * Iterates through unicode characters, returns a Unicode character
   * index and UTF-16 string index of first non-alphabetic character,
   * or size of the string (in characters, string size)
   *
   * @return index of first non-alpha char, or string size, as a tuple of number of characters and index in string
   */
  Indices firstNotAlpha(UStringView sf);

  void analysis_wrapper_null_flush(InputFile& input, UFILE *output);
  void generation_wrapper_null_flush(InputFile& input, UFILE *output,
                                     GenerationMode mode);
  void tm_wrapper_null_flush(InputFile& input, UFILE *output,
                             TranslationMemoryMode tm_mode);
  UString compose(const std::vector<UString>& lexforms, UStringView queue,
                  bool delim = false, bool mark = false) const;
  bool step_biltrans(UStringView word, std::vector<UString>& result, UString& queue);

  void procNodeICX();
  void procNodeRCX();
  void initDefaultIgnoredCharacters();

  bool isLastBlankTM = false;

  xmlTextReaderPtr reader;

  static constexpr size_t max_case_insensitive_state_size = 65536;
  bool max_case_insensitive_state_size_warned = false;
  /*
   * Including lowercased versions for every character can potentially create very large states
   * (See https://github.com/apertium/lttoolbox/issues/167 ). As a sanity-check we don't do
   * case-insensitive matching if the state size exceeds max_case_insensitive_state_size.
   *
   * @return running with --case-sensitive or state size exceeds max
   */
  bool beCaseSensitive(const State& state) {
    if(caseSensitive) {
      return true;
    }
    else if(state.size() < max_case_insensitive_state_size)  {
      return false;             // ie. do case-folding
    }
    else {
      if(!max_case_insensitive_state_size_warned) {
        max_case_insensitive_state_size_warned = true; // only warn once
        UFILE* err_out = u_finit(stderr, NULL, NULL);
        u_fprintf(err_out, "Warning: matching case-sensitively since processor state size >= %d\n", max_case_insensitive_state_size);
      }
      return true;
    }
  }

  void appendEscaped(UString& to, const UString& from) {
    for(auto &c : from) {
      if (escaped_chars.find(c) != escaped_chars.end()) {
        to += u'\\';
      }
      to += c;
    }
  }

public:

  /*
   * String constants
   */
  static constexpr UStringView XML_TEXT_NODE           = u"#text";
  static constexpr UStringView XML_COMMENT_NODE        = u"#comment";
  static constexpr UStringView XML_IGNORED_CHARS_ELEM  = u"ignored-chars";
  static constexpr UStringView XML_RESTORE_CHAR_ELEM   = u"restore-char";
  static constexpr UStringView XML_RESTORE_CHARS_ELEM  = u"restore-chars";
  static constexpr UStringView XML_VALUE_ATTR          = u"value";
  static constexpr UStringView XML_CHAR_ELEM           = u"char";
  static constexpr UStringView WBLANK_FINAL            = u"[[/]]";

  FSTProcessor();

  void initAnalysis();
  void initTMAnalysis();
  void initSAO(){initAnalysis();};
  void initGeneration();
  void initPostgeneration(){initTransliteration();};
  void initTransliteration();
  void initBiltrans();
  void initDecomposition();

  void analysis(InputFile& input, UFILE *output);
  void tm_analysis(InputFile& input, UFILE *output, TranslationMemoryMode tm_mode);
  void generation(InputFile& input, UFILE *output, GenerationMode mode = gm_unknown);
  void postgeneration(InputFile& input, UFILE *output);
  void intergeneration(InputFile& input, UFILE *output);
  void transliteration(InputFile& input, UFILE *output);
  UString biltrans(UStringView input_word, bool with_delim = true);
  UString biltransfull(UStringView input_word, bool with_delim = true);
  void bilingual(InputFile& input, UFILE *output, GenerationMode mode = gm_unknown);
  void quoteMerge(InputFile& input, UFILE *output);
  std::pair<UString, int> biltransWithQueue(UStringView input_word, bool with_delim = true);
  UString biltransWithoutQueue(UStringView input_word, bool with_delim = true);
  void SAO(InputFile& input, UFILE *output);
  void parseICX(std::string const &file);
  void parseRCX(std::string const &file);

  void load(FILE *input);

  bool valid() const;

  void setCaseSensitiveMode(bool value);
  void setDictionaryCaseMode(bool value);
  void setBiltransSurfaceForms(bool value);
  void setBiltransSurfaceFormsKeep(bool value);
  void setIgnoredChars(bool value);
  void setRestoreChars(bool value);
  void setNullFlush(bool value);
  void setUseDefaultIgnoredChars(bool value);
  void setDisplayWeightsMode(bool value);
  void setMaxAnalysesValue(int value);
  void setMaxWeightClassesValue(int value);
  void setCompoundMaxElements(int value);
  bool getNullFlush();
  bool getDecompoundingMode();
};

#endif
