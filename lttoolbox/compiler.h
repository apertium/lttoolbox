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
#ifndef _MYCOMPILER_
#define _MYCOMPILER_

#include <lttoolbox/alphabet.h>
#include <lttoolbox/regexp_compiler.h>
#include <lttoolbox/entry_token.h>
#include <lttoolbox/ltstr.h>
#include <lttoolbox/transducer.h>

#include <map>
#include <string>
#include <set>
#include <libxml/xmlreader.h>

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
  xmlTextReaderPtr reader;
  
  /**
   * The alt value
   */
  wstring alt;

  /**
   * The variant value (monodix)
   */
  wstring variant;
  
  /**
   * The variant value (left side of bidix)
   */
  wstring variant_left;
  
  /**
   * The variant value (right side of bidix)
   */
  wstring variant_right;
    
  /**
   * The paradigm being compiled
   */
  wstring current_paradigm;
  
  /**
   * The dictionary section being compiled
   */
  wstring current_section;
  
  /**
   * The direction of the compilation, 'lr' (left-to-right) or 'rl'
   * (right-to-left)
   */
  wstring direction;
  
  /**
   * List of characters to be considered alphabetic
   */
  wstring letters;
  
  /**
   * Set verbose mode: warnings which may or may not be correct
   */
  bool verbose;

  /**
   * First element (of an entry)
   */
  bool first_element;

  /**
   * Identifier of all the symbols during the compilation
   */
  Alphabet alphabet;  
  
  /**
   * List of named transducers-paradigms
   */
  map<wstring, Transducer, Ltstr> paradigms;
  
  /**
   * List of named dictionary sections
   */
  map<wstring, Transducer, Ltstr> sections;
  
  /**
   * List of named prefix copy of a paradigm
   */
  map<wstring, map<wstring, int, Ltstr>, Ltstr> prefix_paradigms;
  
  /**
   * List of named suffix copy of a paradigm
   */
  map<wstring, map<wstring, int, Ltstr>, Ltstr> suffix_paradigms;

  /**
   * List of named endings of a suffix copy of a paradgim
   */
  map<wstring, map<wstring, int, Ltstr>, Ltstr> postsuffix_paradigms;

  /**
   * Mapping of aliases of characters specified in ACX files
   */
  map<int, set<int> > acx_map;
  
  /**
   * Original char being mapped
   */
  int acx_current_char; 

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
   * Parse the &lt;sdef&lt; element
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
  wstring attrib(wstring const &name);

  /**
   * Construct symbol pairs by align left side of both parts and insert
   * them into a transducer
   * @param lp left part of the transduction
   * @param rp right part of the transduction
   * @param state the state from wich insert the new transduction
   * @param t the transducer
   * @return the last state of the inserted transduction
   */
  int matchTransduction(list<int> const &lp, list<int> const &rp,
			    int state, Transducer &t);
  /**
   * Parse the &lt;p&lt; element
   * @return a list of tokens from the dictionary's entry
   */
  EntryToken procTransduction();

  /**
   * Parse the &lt;i&lt; element
   * @return a list of tokens from the dictionary's entry
   */
  EntryToken procIdentity();

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
  void skip(wstring &name, wstring const &elem);

  /**
   * Skip all blank #text nodes before "name"
   * @param name the name of the node
   */
  void skipBlanks(wstring &name);
  
  
  void readString(list<int> &result, wstring const &name);
  
  /**
   * Force an element to be empty, and check for it
   * @param name the element 
   */
  void requireEmptyError(wstring const &name);

  /**
   * Force an attribute to be specified, amd check for it
   * @param value the value of the attribute
   * @param attrname the name of the attribute
   * @param elemname the parent of the attribute
   */
  void requireAttribute(wstring const &value, wstring const &attrname,
                        wstring const &elemname);

  /**
   * True if all the elements in the current node are blanks
   * @return true if all are blanks
   */
  bool allBlanks();

public:

  /*
   * Constants to represent the element and the attributes of
   * dictionaries
   */
  static wstring const COMPILER_DICTIONARY_ELEM;
  static wstring const COMPILER_ALPHABET_ELEM;
  static wstring const COMPILER_SDEFS_ELEM;
  static wstring const COMPILER_SDEF_ELEM;
  static wstring const COMPILER_N_ATTR;
  static wstring const COMPILER_PARDEFS_ELEM;
  static wstring const COMPILER_PARDEF_ELEM;
  static wstring const COMPILER_PAR_ELEM;
  static wstring const COMPILER_ENTRY_ELEM;
  static wstring const COMPILER_RESTRICTION_ATTR;
  static wstring const COMPILER_RESTRICTION_LR_VAL;
  static wstring const COMPILER_RESTRICTION_RL_VAL;
  static wstring const COMPILER_PAIR_ELEM;
  static wstring const COMPILER_LEFT_ELEM;
  static wstring const COMPILER_RIGHT_ELEM;
  static wstring const COMPILER_S_ELEM;
  static wstring const COMPILER_REGEXP_ELEM;
  static wstring const COMPILER_SECTION_ELEM;
  static wstring const COMPILER_ID_ATTR;
  static wstring const COMPILER_TYPE_ATTR;
  static wstring const COMPILER_IDENTITY_ELEM;
  static wstring const COMPILER_JOIN_ELEM;
  static wstring const COMPILER_BLANK_ELEM;
  static wstring const COMPILER_POSTGENERATOR_ELEM;
  static wstring const COMPILER_GROUP_ELEM;
  static wstring const COMPILER_LEMMA_ATTR;
  static wstring const COMPILER_IGNORE_ATTR;
  static wstring const COMPILER_IGNORE_YES_VAL;
  static wstring const COMPILER_ALT_ATTR;
  static wstring const COMPILER_V_ATTR;
  static wstring const COMPILER_VL_ATTR;
  static wstring const COMPILER_VR_ATTR;


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
  void parse(string const &fichero, wstring const &dir);

  /**
   * Read ACX file
   */
  void parseACX(string const &fichero, wstring const &dir);

  
  /**
   * Write the result of compilation 
   * @param fd the stream where write the result
   */
  void write(FILE *fd);

  /**
   * Set verbose output
   */
  void setVerbose(bool verbosity = false);

  /**
   * Set the alt value to use in compilation
   * @param a the value
   */
  void setAltValue(string const &a);

  /**
   * Set the variant value to use in compilation
   * @param v the value
   */
  void setVariantValue(string const &v);

  /**
   * Set the variant_left value to use in compilation
   * @param v the value
   */
  void setVariantLeftValue(string const &v);

  /**
   * Set the variant_right value to use in compilation
   * @param v the value
   */
  void setVariantRightValue(string const &v);
};


#endif
