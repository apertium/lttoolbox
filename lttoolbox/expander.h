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
#ifndef _EXPANDER_
#define _EXPANDER_

#include <lttoolbox/ltstr.h>
#include <lttoolbox/string_to_wostream.h>

#include <list>
#include <map>
#include <libxml/xmlreader.h>
#include <string>

using namespace std;

typedef list<pair<wstring, wstring> > EntList;

/**
 * An expander of dictionaries
 */
class Expander
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
   * The direction of the compilation, 'lr' (left-to-right) or 'rl'
   * (right-to-left)
   */
  wstring direction;
  
  /**
   * Paradigms
   */
  map<wstring, EntList, Ltstr> paradigm;

  map<wstring, EntList, Ltstr> paradigm_lr;

  map<wstring, EntList, Ltstr> paradigm_rl;

  /**
   * Method to parse an XML Node
   */
  void procNode(FILE *output);

  /**
   * Parse the &lt;pardef&gt; element
   */
  void procParDef();
  
  /**
   * Parse the &lt;e&gt; element
   */
  void procEntry(FILE *output);

  /**
   * Parse the &lt;re&gt; element
   * @return the string representing the regular expression
   */
  wstring procRegexp();

  /**
   * Gets an attribute value with their name and the current context
   * @param name the name of the attribute
   * @return the value of the attribute
   */
  wstring attrib(wstring const &name);

  /**
   * Parse the &lt;p&lt; element
   * @return a pair of strings, left part and right part of a transduction
   */
  pair<wstring, wstring> procTransduction();

  /**
   * Parse the &lt;i&lt; element
   * @return a string from the dictionary's entry
   */
  wstring procIdentity();

  /**
   * Parse the &lt;ig&lt; element
   * @return a pair of strings, whose right part begins with '#'
   * but are otherwise identical
   */
  pair<wstring, wstring> procIdentityGroup();

  /**
   * Parse the &lt;par&gt; element
   * @return the name of the paradigm
   */
  wstring procPar();

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
  
  
  void readString(wstring &result, wstring const &name);
  
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

  /**
   * Append a list of endings to a list of current transductions.
   * @param result the current partial transductions, and after calling
   *               this method, the result of concatenations.
   * @param endings the endings to be appended.
   */
  static void append(list<pair<wstring, wstring> > &result, 
                     list<pair<wstring, wstring> > const &endings);

  /**
   * Append a list of endings to a list of current transductions.
   * @param result the current partial transductions, and after calling
   *               this method, the result of concatenations.
   * @param endings the endings to be appended.
   */
  static void append(list<pair<wstring, wstring> > &result, 
                     wstring const &endings);

  /**
   * Append a list of endings to a list of current transductions.
   * @param result the current partial transductions, and after calling
   *               this method, the result of concatenations.
   * @param endings the endings to be appended.
   */
  static void append(list<pair<wstring, wstring> > &result, 
	             pair<wstring, wstring> const &endings);

public:
  /**
   * Constructor
   */
  Expander();

  /**
   * Destructor
   */
  ~Expander();

  /**
   * Compile dictionary to letter transducers
   */
  void expand(string const &fichero, FILE *output);
  /**
   * Set the alt value to use in compilation
   * @param a the value
   */
   void setAltValue(string const &a);

  /**
   * Set the variant value to use in expansion
   * @param v the value
   */
   void setVariantValue(string const &v);

  /**
   * Set the variant_left value to use in expansion
   * @param v the value
   */
   void setVariantLeftValue(string const &v);

  /**
   * Set the variant_right value to use in expansion
   * @param v the value
   */
   void setVariantRightValue(string const &v);
};


#endif
