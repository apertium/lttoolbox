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
#ifndef _EXPANDER_
#define _EXPANDER_

#include <lttoolbox/ustring.h>

#include <list>
#include <map>
#include <libxml/xmlreader.h>
#include <string>

typedef std::list<std::pair<UString, UString> > EntList;

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
   * The direction of the compilation, 'lr' (left-to-right) or 'rl'
   * (right-to-left)
   */
  UString direction;

  /**
   * Do we print boundaries or not?
   */
  bool keep_boundaries;

  /**
   * Paradigms
   */
  std::map<UString, EntList> paradigm;

  std::map<UString, EntList> paradigm_lr;

  std::map<UString, EntList> paradigm_rl;

  /**
   * Method to parse an XML Node
   */
  void procNode(UFILE* output);

  /**
   * Parse the &lt;pardef&gt; element
   */
  void procParDef();

  /**
   * Parse the &lt;e&gt; element
   */
  void procEntry(UFILE* output);

  /**
   * Parse the &lt;re&gt; element
   * @return the string representing the regular expression
   */
  UString procRegexp();

  /**
   * Gets an attribute value with their name and the current context
   * @param name the name of the attribute
   * @return the value of the attribute
   */
  UString attrib(UString const &name);

  /**
   * Parse the &lt;p&gt; element
   * @return a pair of strings, left part and right part of a transduction
   */
  std::pair<UString, UString> procTransduction();

  /**
   * Parse the &lt;i&gt; element
   * @return a string from the dictionary's entry
   */
  UString procIdentity();

  /**
   * Parse the &lt;ig&gt; element
   * @return a pair of strings, whose right part begins with '#'
   * but are otherwise identical
   */
  std::pair<UString, UString> procIdentityGroup();

  /**
   * Parse the &lt;par&gt; element
   * @return the name of the paradigm
   */
  UString procPar();

  /**
   * Skip all document #text nodes before "elem"
   * @param name the name of the node
   * @param elem the name of the expected node
   */
  void skip(UString &name, UString const &elem);

  /**
   * Skip all blank #text nodes before "name"
   * @param name the name of the node
   */
  void skipBlanks(UString &name);


  void readString(UString &result, UString const &name);

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

  /**
   * Append a list of endings to a list of current transductions.
   * @param result the current partial transductions, and after calling
   *               this method, the result of concatenations.
   * @param endings the endings to be appended.
   */
  static void append(std::list<std::pair<UString, UString> > &result,
                     std::list<std::pair<UString, UString> > const &endings);

  /**
   * Append a list of endings to a list of current transductions.
   * @param result the current partial transductions, and after calling
   *               this method, the result of concatenations.
   * @param endings the endings to be appended.
   */
  static void append(std::list<std::pair<UString, UString> > &result,
                     UString const &endings);

  /**
   * Append a list of endings to a list of current transductions.
   * @param result the current partial transductions, and after calling
   *               this method, the result of concatenations.
   * @param endings the endings to be appended.
   */
  static void append(std::list<std::pair<UString, UString> > &result,
                     std::pair<UString, UString> const &endings);

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
  void expand(std::string const &file, UFILE* output);

  /**
   * Set the alt value to use in compilation
   * @param a the value
   */
   void setAltValue(UString const &a);

  /**
   * Set the variant value to use in expansion
   * @param v the value
   */
   void setVariantValue(UString const &v);

  /**
   * Set the variant_left value to use in expansion
   * @param v the value
   */
   void setVariantLeftValue(UString const &v);

  /**
   * Set the variant_right value to use in expansion
   * @param v the value
   */
   void setVariantRightValue(UString const &v);

  /**
   * Set if we are going to keep morpheme boundaries
   * @param keep true, false
   */
   void setKeepBoundaries(bool keep_boundaries = false);

};

#endif
