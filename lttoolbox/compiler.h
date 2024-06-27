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
#ifndef _MYCOMPILER_
#define _MYCOMPILER_

#include <lttoolbox/alphabet.h>
#include <lttoolbox/entry_token.h>
#include <lttoolbox/transducer.h>
#include <lttoolbox/ustring.h>
#include <lttoolbox/sorted_vector.hpp>

#include <map>
#include <set>
#include <libxml/xmlreader.h>

/**
 * A compiler of dictionaries to letter transducers
 */
class Compiler
{
private:
  /**
   * The libxml2's XML reader
   */
  xmlTextReaderPtr reader = nullptr;

  /**
   * The alt value
   */
  UString alt;

  /**
   * The variant value (monodix)
   */
  UString variant;

  /**
   * The variant value (left side of bidix)
   */
  UString variant_left;

  /**
   * The variant value (right side of bidix)
   */
  UString variant_right;

  /**
   * The paradigm being compiled
   */
  UString current_paradigm;

  /**
   * The number of top-level entries read in this section.
   */
  size_t n_section_entries = 0;

  /**
   * The maximum number of top-level entries per section.
   * If 0, no limit.
   */
  size_t max_section_entries = 0;

  /**
   * The dictionary section being compiled
   */
  UString current_section;

  /**
   * The direction of the compilation, 'lr' (left-to-right) or 'rl'
   * (right-to-left)
   */
  UString direction;

  /**
   * If this is set to true, attributes v, vl, vr, r, and alt
   * insert special symbols to be filtered by lt-restrict rather than
   * ignoring entries.
   */
  bool unified_compilation = false;

  /**
   * List of characters to be considered alphabetic
   */
  UString letters;

  /**
   * Set verbose mode: warnings which may or may not be correct
   */
  bool verbose = false;

  /**
   * First element (of an entry)
   */
  bool first_element = false;

  /**
   * Default value of weight (of a transition)
   */
  double default_weight = 0.0000;

  /**
   * Maintain morpheme boundaries
   */
  bool keep_boundaries = false;

  /**
   * Allow parallel minimisation jobs
   */
  bool jobs = false;

  /**
   * Are we compiling an LSX dictionary
   */
  bool is_separable = false;

  /**
   * Should we put output line numbers on each <e> in <section>?
   */
  bool entry_debugging = false;


  /**
   * Identifier of all the symbols during the compilation
   */
  Alphabet alphabet;

  /**
   * List of named transducers-paradigms
   */
  std::map<UString, Transducer, std::less<>> paradigms;

  /**
   * List of named dictionary sections
   */
  std::map<UString, Transducer> sections;

  /**
   * List of named prefix copy of a paradigm
   */
  std::map<UString, std::map<UString, int> > prefix_paradigms;

  /**
   * List of named suffix copy of a paradigm
   */
  std::map<UString, std::map<UString, int>, std::less<>> suffix_paradigms;

  /**
   * List of named endings of a suffix copy of a paradgim
   */
  std::map<UString, std::map<UString, int> > postsuffix_paradigms;

  /**
   * Mapping of aliases of characters specified in ACX files
   */
  std::map<int32_t, sorted_vector<int32_t> > acx_map;

  /**
   * LSX symbols
   */
  int32_t any_tag = 0;
  int32_t any_char = 0;
  int32_t word_boundary = 0;
  int32_t word_boundary_s = 0;
  int32_t word_boundary_ns = 0;
  int32_t reading_boundary = 0;

  /**
   * Method to parse an XML Node
   */
  void procNode();

  /**
   * Parse the &lt;alphabet&gt; element
   */
  void procAlphabet();

  /**
   * Parse the &lt;sdef&gt; element
   */
  void procSDef();

  /**
   * Parse the &lt;pardef&gt; element
   */
  void procParDef();

  /**
   * Parse the &lt;e&gt; element
   */
  void procEntry();

  /**
   * Return true if the filter (command line) is consistent with
   * the value (attribute) and false otherwise
   */
  bool filterEntry(UStringView value, UStringView filter, bool keep_on_empty_filter);
  void symbolFilters(UStringView value, UStringView prefix, std::vector<std::vector<int32_t>>& symbols);

  /**
   * Parse the &lt;re&gt; element
   * @return a list of tokens from the dictionary's entry
   */
  EntryToken procRegexp();

  /**
   * Parse the &lt;section&gt; element
   */
  void procSection();

  /**
   * Gets an attribute value with their name and the current context
   * @param name the name of the attribute
   * @return the value of the attribute
   */
  UString attrib(UStringView name);

  /**
   * Construct symbol pairs by align left side of both parts and insert
   * them into a transducer
   * @param lp left part of the transduction
   * @param rp right part of the transduction
   * @param state the state from wich insert the new transduction
   * @param t the transducer
   * @return the last state of the inserted transduction
   */
  int matchTransduction(std::vector<int> const &lp, std::vector<int> const &rp,
                        int state, Transducer &t, double const &entry_weight);
  /**
   * Parse the &lt;p&gt; element
   * @return a list of tokens from the dictionary's entry
   */
  EntryToken procTransduction(double const entry_weight);

  /**
   * Parse the &lt;i&gt; element
   * @return a list of tokens from the dictionary's entry
   */
  EntryToken procIdentity(double const entry_weight, bool ig = false);

  /**
   * Parse the &lt;par&gt; element
   * @return a list of tokens from the dictionary's entry
   */
  EntryToken procPar();

  /**
   * Insert a list of tokens into the paradigm / section being processed
   * @param elements the list
   */
  void insertEntryTokens(std::vector<EntryToken> const &elements);

  /**
   * Skip all document #text nodes before "elem"
   * @param name the name of the node
   * @param elem the name of the expected node
   * @param open true for open element, false for closed
   */
  void skip(UString &name, UStringView elem, bool open = true);

  /**
   * Skip all blank #text nodes before "name"
   * @param name the name of the node
   */
  void skipBlanks(UString &name);

  void step(UString& name);

  void readString(std::vector<int> &result, UStringView name);

  /**
   * Force an element to be empty, and check for it
   * @param name the element
   */
  void requireEmptyError(UStringView name);

  /**
   * Force an attribute to be specified, amd check for it
   * @param value the value of the attribute
   * @param attrname the name of the attribute
   * @param elemname the parent of the attribute
   */
  void requireAttribute(UStringView value, UStringView attrname, UStringView elemname);

  /**
   * True if all the elements in the current node are blanks
   * @return true if all are blanks
   */
  bool allBlanks();

  bool valid(UStringView dir) const;

public:

  /*
   * Constants to represent the element and the attributes of
   * dictionaries
   */
  static constexpr UStringView COMPILER_DICTIONARY_ELEM    = u"dictionary";
  static constexpr UStringView COMPILER_ALPHABET_ELEM      = u"alphabet";
  static constexpr UStringView COMPILER_SDEFS_ELEM         = u"sdefs";
  static constexpr UStringView COMPILER_SDEF_ELEM          = u"sdef";
  static constexpr UStringView COMPILER_N_ATTR             = u"n";
  static constexpr UStringView COMPILER_PARDEFS_ELEM       = u"pardefs";
  static constexpr UStringView COMPILER_PARDEF_ELEM        = u"pardef";
  static constexpr UStringView COMPILER_PAR_ELEM           = u"par";
  static constexpr UStringView COMPILER_ENTRY_ELEM         = u"e";
  static constexpr UStringView COMPILER_RESTRICTION_ATTR   = u"r";
  static constexpr UStringView COMPILER_RESTRICTION_LR_VAL = u"LR";
  static constexpr UStringView COMPILER_RESTRICTION_RL_VAL = u"RL";
  static constexpr UStringView COMPILER_RESTRICTION_U_VAL  = u"U";
  static constexpr UStringView COMPILER_PAIR_ELEM          = u"p";
  static constexpr UStringView COMPILER_LEFT_ELEM          = u"l";
  static constexpr UStringView COMPILER_RIGHT_ELEM         = u"r";
  static constexpr UStringView COMPILER_S_ELEM             = u"s";
  static constexpr UStringView COMPILER_M_ELEM             = u"m";
  static constexpr UStringView COMPILER_REGEXP_ELEM        = u"re";
  static constexpr UStringView COMPILER_SECTION_ELEM       = u"section";
  static constexpr UStringView COMPILER_ID_ATTR            = u"id";
  static constexpr UStringView COMPILER_TYPE_ATTR          = u"type";
  static constexpr UStringView COMPILER_SEQUENTIAL_VAL     = u"sequential";
  static constexpr UStringView COMPILER_SEPARABLE_VAL      = u"separable";
  static constexpr UStringView COMPILER_IDENTITY_ELEM      = u"i";
  static constexpr UStringView COMPILER_IDENTITYGROUP_ELEM = u"ig";
  static constexpr UStringView COMPILER_JOIN_ELEM          = u"j";
  static constexpr UStringView COMPILER_BLANK_ELEM         = u"b";
  static constexpr UStringView COMPILER_POSTGENERATOR_ELEM = u"a";
  static constexpr UStringView COMPILER_GROUP_ELEM         = u"g";
  static constexpr UStringView COMPILER_LEMMA_ATTR         = u"lm";
  static constexpr UStringView COMPILER_IGNORE_ATTR        = u"i";
  static constexpr UStringView COMPILER_IGNORE_YES_VAL     = u"yes";
  static constexpr UStringView COMPILER_ALT_ATTR           = u"alt";
  static constexpr UStringView COMPILER_V_ATTR             = u"v";
  static constexpr UStringView COMPILER_VL_ATTR            = u"vl";
  static constexpr UStringView COMPILER_VR_ATTR            = u"vr";
  static constexpr UStringView COMPILER_WEIGHT_ATTR        = u"w";
  static constexpr UStringView COMPILER_TEXT_NODE          = u"#text";
  static constexpr UStringView COMPILER_COMMENT_NODE       = u"#comment";
  static constexpr UStringView COMPILER_ACX_ANALYSIS_ELEM  = u"analysis-chars";
  static constexpr UStringView COMPILER_ACX_CHAR_ELEM      = u"char";
  static constexpr UStringView COMPILER_ACX_EQUIV_CHAR_ELEM= u"equiv-char";
  static constexpr UStringView COMPILER_ACX_VALUE_ATTR     = u"value";
  static constexpr UStringView COMPILER_LSX_WB_ELEM        = u"d";
  static constexpr UStringView COMPILER_LSX_CHAR_ELEM      = u"w";
  static constexpr UStringView COMPILER_LSX_TAG_ELEM       = u"t";
  static constexpr UStringView COMPILER_LSX_FORM_SEP_ELEM  = u"f";
  static constexpr UStringView COMPILER_LSX_SPACE_ATTR     = u"space";
  static constexpr UStringView COMPILER_LSX_SPACE_YES_VAL  = u"yes";
  static constexpr UStringView COMPILER_LSX_SPACE_NO_VAL   = u"no";

  /**
   * Constructor
   */
  Compiler();

  /**
   * Destructor
   */
  ~Compiler();

  /**
   * Compile dictionary to letter transducers
   */
  void parse(std::string const &file, UStringView dir);

  /**
   * Read ACX file
   */
  void parseACX(std::string const &file, UStringView dir);


  /**
   * Write the result of compilation
   * @param fd the stream where write the result
   */
  void write(FILE *fd);

  /**
   * Set keep morpheme boundaries
   */
  void setKeepBoundaries(bool keep_boundaries = false);

  /**
   * Set whether to allow parallel minimisation jobs
   */
  void setJobs(bool jobs);

  /**
   * Set how many top-level entries to allow in a section before starting a new one automatically
   */
  void setMaxSectionEntries(size_t m);

  /**
   * Set verbose output
   */
  void setVerbose(bool verbosity = false);

  /**
   * Set the alt value to use in compilation
   * @param a the value
   */
  void setAltValue(UStringView a);

  /**
   * Set the variant value to use in compilation
   * @param v the value
   */
  void setVariantValue(UStringView v);

  /**
   * Set the variant_left value to use in compilation
   * @param v the value
   */
  void setVariantLeftValue(UStringView v);

  /**
   * Set the variant_right value to use in compilation
   * @param v the value
   */
  void setVariantRightValue(UStringView v);

  void setEntryDebugging(bool b);
};


#endif
