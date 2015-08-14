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
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */
#ifndef _TMXCOMPILER_
#define _TMXCOMPILER_

#include <lttoolbox/alphabet.h>
#include <lttoolbox/regexp_compiler.h>
#include <lttoolbox/entry_token.h>
#include <lttoolbox/ltstr.h>
#include <lttoolbox/transducer.h>

#include <map>
#include <string>
#include <set>
#include <libxml/xmlreader.h>
#include <iostream>

using namespace std;

/**
 * A compiler of dictionaries to letter transducers
 */
class TMXCompiler
{
private:
  /**
   * The libxml2's XML reader
   */
  xmlTextReaderPtr reader;
  
  /**
   * Identifier of all the symbols during the compilation
   */
  Alphabet alphabet;

  /**
   * Main transducer representing the TM
   */  
  Transducer transducer;  

  /**
   * Origin language
   */
  wstring origin_language;

  /**
   * Meta language
   */
  wstring meta_language;
  
  /**
   * Origin language code in the TMX
   */
  wstring origin_language_inner_code;

  /**
   * Origin language code in the TMX
   */
  wstring meta_language_inner_code;


  /**
   * Method to parse an XML Node
   */
  void procNode();

  /**
   * Parse the &lt;tu&gt; element
   */
  void procTU();

  /**
   * Insert a tu into the transducer
   * @param origin left part
   * @param meta right part
   */
  void insertTU(vector<int> const &origin, vector<int> const &meta);

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

  wstring getTag(size_t const &val) const;
  void trim(vector<int> &v) const;
  void align(vector<int> &origin, vector<int> &meta);
  unsigned int numberLength(vector<int> &v, unsigned int const position) const;
  bool vectorcmp(vector<int> const &orig, unsigned int const begin_orig,
                       vector<int> const &meta, unsigned int const begin_meta,
                       unsigned const int length) const;
  void split(vector<int> const &v, vector<vector<int> > &sv, int const symbol) const;
  void align_blanks(vector<int> &o, vector<int> &m);
  vector<int> join(vector<vector<int> > const &v, int const s) const;

  static void printvector(vector<int> const &v, wostream &wos = std::wcout);  //eliminar este método
  
public:

  /*
   * Constants to represent the element and the attributes of
   * translation memories in TMX format
   */
  static wstring const TMX_COMPILER_TMX_ELEM;
  static wstring const TMX_COMPILER_HEADER_ELEM;
  static wstring const TMX_COMPILER_BODY_ELEM;
  static wstring const TMX_COMPILER_TU_ELEM;
  static wstring const TMX_COMPILER_TUV_ELEM;
  static wstring const TMX_COMPILER_HI_ELEM;
  static wstring const TMX_COMPILER_PH_ELEM;
  static wstring const TMX_COMPILER_XMLLANG_ATTR;
  static wstring const TMX_COMPILER_LANG_ATTR;  
  static wstring const TMX_COMPILER_SEG_ELEM;
  static wstring const TMX_COMPILER_PROP_ELEM;


  /**
   * Constructor
   */
  TMXCompiler();

  /**
   * Destructor
   */
  ~TMXCompiler();

  /**
   * Compile dictionary to letter transducers
   */
  void parse(string const &fichero, wstring const &lo, wstring const &lm);
  
  /**
   * Write the result of compilation 
   * @param fd the stream where write the result
   */
  void write(FILE *fd);

  /**
   * Set origin language inner code
   * @param code the code of the origin language into the TMX file being compiled
   */
  void setOriginLanguageCode(wstring const &code);

  /**
   * Set meta language inner code
   * @param code the code of the meta language into the TMX file being compiled
   */
  void setMetaLanguageCode(wstring const &code);
 
};

#endif
