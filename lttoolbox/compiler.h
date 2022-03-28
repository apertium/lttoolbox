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
#include <lttoolbox/regexp_compiler.h>
#include <lttoolbox/entry_token.h>
#include <lttoolbox/transducer.h>
#include <lttoolbox/ustring.h>

#include <thread>
#include <map>
#include <string>
#include <set>
#include <libxml/xmlreader.h>

#ifdef _MSC_VER
  #if !defined(LTTOOLBOX_EXPORTS)
    #define LTTOOLBOX_IMPORTS __declspec(dllimport)
  #else
    #define LTTOOLBOX_IMPORTS
  #endif
#else
  #define LTTOOLBOX_IMPORTS
#endif

using namespace std;

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
   * The dictionary section being compiled
   */
  UString current_section;

  /**
   * The direction of the compilation, 'lr' (left-to-right) or 'rl'
   * (right-to-left)
   */
  UString direction;

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
   * Identifier of all the symbols during the compilation
   */
  Alphabet alphabet;

  /**
   * List of named transducers-paradigms
   */
  map<UString, Transducer> paradigms;

  /**
   * List of named dictionary sections
   */
  map<UString, Transducer> sections;

  /**
   * List of named prefix copy of a paradigm
   */
  map<UString, map<UString, int> > prefix_paradigms;

  /**
   * List of named suffix copy of a paradigm
   */
  map<UString, map<UString, int> > suffix_paradigms;

  /**
   * List of named endings of a suffix copy of a paradgim
   */
  map<UString, map<UString, int> > postsuffix_paradigms;

  /**
   * Mapping of aliases of characters specified in ACX files
   */
  map<int, set<int> > acx_map;

  /**
   * Original char being mapped
   */
  int acx_current_char = 0;

  /*
  static string range(char const a, char const b);
  string readAlphabet();
  */

  /**
   * Method to parse an XML Node
   */
  void procNode();

  /**
   * Method to parse an XML Node in ACX files
   */
  void procNodeACX();


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
  UString attrib(UString const &name);

  /**
   * Construct symbol pairs by align left side of both parts and insert
   * them into a transducer
   * @param lp left part of the transduction
   * @param rp right part of the transduction
   * @param state the state from wich insert the new transduction
   * @param t the transducer
   * @return the last state of the inserted transduction
   */
  int matchTransduction(vector<int> const &lp, vector<int> const &rp,
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
  void insertEntryTokens(vector<EntryToken> const &elements);

  /**
   * Skip all document #text nodes before "elem"
   * @param name the name of the node
   * @param elem the name of the expected node
   */
  void skip(UString &name, UString const &elem);

  /**
   * Skip all document #text nodes before "elem"
   * @param name the name of the node
   * @param elem the name of the expected node
   * @param open true for open element, false for closed
   */
  void skip(UString &name, UString const &elem, bool open);

  /**
   * Skip all blank #text nodes before "name"
   * @param name the name of the node
   */
  void skipBlanks(UString &name);


  void readString(vector<int> &result, UString const &name);

  /**
   * Force an element to be empty, and check for it
   * @param name the element
   */
  void requireEmptyError(UString const &name);

  /**
   * Force an attribute to be specified, amd check for it
   * @param value the value of the attribute
   * @param attrname the name of the attribute
   * @param elemname the parent of the attribute
   */
  void requireAttribute(UString const &value, UString const &attrname,
                        UString const &elemname);

  /**
   * True if all the elements in the current node are blanks
   * @return true if all are blanks
   */
  bool allBlanks();

  bool valid(UString const& dir) const;

public:

  /*
   * Constants to represent the element and the attributes of
   * dictionaries
   */
  LTTOOLBOX_IMPORTS static UString const COMPILER_DICTIONARY_ELEM;
  LTTOOLBOX_IMPORTS static UString const COMPILER_ALPHABET_ELEM;
  LTTOOLBOX_IMPORTS static UString const COMPILER_SDEFS_ELEM;
  LTTOOLBOX_IMPORTS static UString const COMPILER_SDEF_ELEM;
  LTTOOLBOX_IMPORTS static UString const COMPILER_N_ATTR;
  LTTOOLBOX_IMPORTS static UString const COMPILER_PARDEFS_ELEM;
  LTTOOLBOX_IMPORTS static UString const COMPILER_PARDEF_ELEM;
  LTTOOLBOX_IMPORTS static UString const COMPILER_PAR_ELEM;
  LTTOOLBOX_IMPORTS static UString const COMPILER_ENTRY_ELEM;
  LTTOOLBOX_IMPORTS static UString const COMPILER_RESTRICTION_ATTR;
  LTTOOLBOX_IMPORTS static UString const COMPILER_RESTRICTION_LR_VAL;
  LTTOOLBOX_IMPORTS static UString const COMPILER_RESTRICTION_RL_VAL;
  LTTOOLBOX_IMPORTS static UString const COMPILER_PAIR_ELEM;
  LTTOOLBOX_IMPORTS static UString const COMPILER_LEFT_ELEM;
  LTTOOLBOX_IMPORTS static UString const COMPILER_RIGHT_ELEM;
  LTTOOLBOX_IMPORTS static UString const COMPILER_S_ELEM;
  LTTOOLBOX_IMPORTS static UString const COMPILER_M_ELEM;
  LTTOOLBOX_IMPORTS static UString const COMPILER_REGEXP_ELEM;
  LTTOOLBOX_IMPORTS static UString const COMPILER_SECTION_ELEM;
  LTTOOLBOX_IMPORTS static UString const COMPILER_ID_ATTR;
  LTTOOLBOX_IMPORTS static UString const COMPILER_TYPE_ATTR;
  LTTOOLBOX_IMPORTS static UString const COMPILER_IDENTITY_ELEM;
  LTTOOLBOX_IMPORTS static UString const COMPILER_IDENTITYGROUP_ELEM;
  LTTOOLBOX_IMPORTS static UString const COMPILER_JOIN_ELEM;
  LTTOOLBOX_IMPORTS static UString const COMPILER_BLANK_ELEM;
  LTTOOLBOX_IMPORTS static UString const COMPILER_POSTGENERATOR_ELEM;
  LTTOOLBOX_IMPORTS static UString const COMPILER_GROUP_ELEM;
  LTTOOLBOX_IMPORTS static UString const COMPILER_LEMMA_ATTR;
  LTTOOLBOX_IMPORTS static UString const COMPILER_IGNORE_ATTR;
  LTTOOLBOX_IMPORTS static UString const COMPILER_IGNORE_YES_VAL;
  LTTOOLBOX_IMPORTS static UString const COMPILER_ALT_ATTR;
  LTTOOLBOX_IMPORTS static UString const COMPILER_V_ATTR;
  LTTOOLBOX_IMPORTS static UString const COMPILER_VL_ATTR;
  LTTOOLBOX_IMPORTS static UString const COMPILER_VR_ATTR;
  LTTOOLBOX_IMPORTS static UString const COMPILER_WEIGHT_ATTR;
  LTTOOLBOX_IMPORTS static UString const COMPILER_TEXT_NODE;
  LTTOOLBOX_IMPORTS static UString const COMPILER_COMMENT_NODE;
  LTTOOLBOX_IMPORTS static UString const COMPILER_ACX_ANALYSIS_ELEM;
  LTTOOLBOX_IMPORTS static UString const COMPILER_ACX_CHAR_ELEM;
  LTTOOLBOX_IMPORTS static UString const COMPILER_ACX_EQUIV_CHAR_ELEM;
  LTTOOLBOX_IMPORTS static UString const COMPILER_ACX_VALUE_ATTR;
  LTTOOLBOX_IMPORTS static UString const COMPILER_LSX_WB_ELEM;
  LTTOOLBOX_IMPORTS static UString const COMPILER_LSX_CHAR_ELEM;
  LTTOOLBOX_IMPORTS static UString const COMPILER_LSX_TAG_ELEM;

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
  void parse(string const &file, UString const &dir);

  /**
   * Read ACX file
   */
  void parseACX(string const &file, UString const &dir);


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
  void setJobs(bool jobs = false);

  /**
   * Set verbose output
   */
  void setVerbose(bool verbosity = false);

  /**
   * Set the alt value to use in compilation
   * @param a the value
   */
  void setAltValue(UString const &a);

  /**
   * Set the variant value to use in compilation
   * @param v the value
   */
  void setVariantValue(UString const &v);

  /**
   * Set the variant_left value to use in compilation
   * @param v the value
   */
  void setVariantLeftValue(UString const &v);

  /**
   * Set the variant_right value to use in compilation
   * @param v the value
   */
  void setVariantRightValue(UString const &v);
};


#endif
