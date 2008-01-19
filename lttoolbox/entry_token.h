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
#ifndef _ENTRYTOKEN_
#define _ENTRYTOKEN_


#include <list>
#include <string>

using namespace std;

/**
 * This is a "Compiler" helper class, to store the parts of each entry
 * before combining it to build the transducer being "compiled".
 */
class EntryToken
{
  /**
   * Type of tokens, inner enum.
   */
  enum Type {paradigm, single_transduction, regexp};
private:
  /**
   * Type of this token
   */
  Type type;
  
  /**
   * Name of the paradigm (if it is of 'paradigm' 'type')
   */
  wstring parName;
  
  /**
   * Left side of transduction (if 'single_transduction')
   */
  list<int> leftSide;
  
  /**
   * Right side of transduction (if 'single_transduction')
   */
  list<int> rightSide;
  
  /**
   * Regular expression (if 'regexp')
   */
  wstring myregexp;

  /**
   * copy method
   */
  void copy(EntryToken const &e);
  
  /**
   * destroy method
   */
  void destroy();
public:

  /**
   * Non-parametric constructor
   */
  EntryToken();
  
  /**
   * Destructor
   */
  ~EntryToken();
  
  /**
   * Copy constructor
   */
  EntryToken(EntryToken const &e);
  
  /**
   * Operator assignment
   */
  EntryToken & operator = (EntryToken const &e);
  
  /**
   * Sets the name of the paradigm.
   * @param np the paradigm name
   */
  void setParadigm(wstring const &np);
  
  /**
   * Set both parts of a single transduction.
   * @param pi left part
   * @param pd right part
   */
  void setSingleTransduction(list<int> const &pi, list<int> const &pd);
  
  /**
   * Set regular expression.
   * @param r the regular expression specification.
   */
  void setRegexp(wstring const &r);
  
  /**
   * eTest EntryToken to detect if is a paradigm.
   * @return true if it is a paradigm.
   */
  bool isParadigm() const;
  
  /**
   * Test EntryToken to check if it is a single transduction.
   * @return true if it is a single transduction.
   */
  bool isSingleTransduction() const;

  /**
   * Test EntryToken to check if it is a single regular expression.
   * @return true if it is a regular expression.
   */
  bool isRegexp() const;
 
  /**
   * Retrieve the name of the paradigm.
   * @return the name of the paradigm.
   */
  wstring const & paradigmName() const;
  
  /**
   * Retrieve the left part of the paradigm.
   * @return the left part of the paradigm.
   */
  list<int> const & left() const;
  
  /**
   * Retrieve the right part of the paradigm.
   * @return the right part of the paradigm.
   */
  list<int> const & right() const;
  
  /**
   * Retrieve the regular expression specification.
   * @return the regular expression specification.
   */
  wstring const & regExp() const;
};

#endif
