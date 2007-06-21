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
#ifndef _TMXPREPROCESSOR_
#define _TMXPREPROCESSOR_

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
class TMXPreprocessor
{
private:
  /**
   * The libxml2's XML reader
   */
  xmlTextReaderPtr reader;
  
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

  /*    
  static string range(char const a, char const b);
  string readAlphabet();
  */

  /**
   * Method to parse an XML Node
   */
  void procNode();

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
  static wstring const TMX_BODY_ELEM;
  static wstring const TMX_HEADER_ELEM;
  static wstring const TMX_MAP_ELEM;
  static wstring const TMX_NOTE_ELEM;
  static wstring const TMX_PROP_ELEM;
  static wstring const TMX_SEG_ELEM;
  static wstring const TMX_TMX_ELEM;
  static wstring const TMX_TU_ELEM;
  static wstring const TMX_TUV_ELEM;
  static wstring const TMX_UDE_ELEM;
  static wstring const TMX_BPT_ELEM;
  static wstring const TMX_EPT_ELEM;
  static wstring const TMX_HI_ELEM;
  static wstring const TMX_IT_ELEM;
  static wstring const TMX_PH_ELEM;
  static wstring const TMX_SUB_ELEM;
  static wstring const TMX_UT_ELEM;
  static wstring const TMX_ADMINLANG_ATTR;
  static wstring const TMX_ASSOC_ATTR;
  static wstring const TMX_BASE_ATTR;
  static wstring const TMX_CHANGEDATE_ATTR;
  static wstring const TMX_CHANGEID_ATTR;
  static wstring const TMX_CODE_ATTR;
  static wstring const TMX_CREATIONDATE_ATTR;
  static wstring const TMX_CREATIONID_ATTR;
  static wstring const TMX_CREATIONTOOL_ATTR;
  static wstring const TMX_CREATIONTOOLVERSION_ATTR;
  static wstring const TMX_DATATYPE_ATTR;
  static wstring const TMX_ENT_ATTR;
  static wstring const TMX_I_ATTR;
  static wstring const TMX_LASTUSAGEDATE_ATTR;
  static wstring const TMX_NAME_ATTR;
  static wstring const TMX_O_ENCODING_ATTR;
  static wstring const TMX_O_TMF_ATTR;
  static wstring const TMX_POS_ATTR;
  static wstring const TMX_SEGTYPE_ATTR;
  static wstring const TMX_SRCLANG_ATTR;
  static wstring const TMX_SUBST_ATTR;
  static wstring const TMX_TUID_ATTR;
  static wstring const TMX_TYPE_ATTR;
  static wstring const TMX_UNICODE_ATTR;
  static wstring const TMX_USAGECOUNT_ATTR;
  static wstring const TMX_VERSION_ATTR;
  static wstring const TMX_X_ATTR;
  static wstring const TMX_XML_LANG;

  /**
   * Copnstructor
   */
  TMXPreprocessor();

  /**
   * Destructor
   */
  ~TMXPreprocessor();

  /**
   * Compile dictionary to letter transducers
   */
  void parse(string const &fichero, wstring const &dir);
  
  /**
   * Write the result of compilation 
   * @param fd the stream where write the result
   */
  void write(FILE *fd);
};


#endif
