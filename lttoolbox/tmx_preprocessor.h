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
#include <lttoolbox/ltstr.h>

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
   * List of characters to be considered alphabetic
   */
  wstring letters;
  
  /**
   * Identifier of all the symbols during the compilation
   */
  Alphabet alphabet;  
  
  /**
   * Method to parse an XML Node
   */
  void procNode();

  /**
   * Gets an attribute value with their name and the current context
   * @param name the name of the attribute
   * @return the value of the attribute
   */
  wstring attrib(wstring const &name);

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
  static wstring const TMX_XML_LANG_ATTR;

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
  void parse(string const &filename, wstring const &dir);
  
  /**
   * Write the result of compilation 
   * @param fd the stream where write the result
   */
  void write(FILE *fd);
};


#endif
