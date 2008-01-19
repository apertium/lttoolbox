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
#ifndef _REGEXP_COMPILER_
#define _REGEXP_COMPILER_

#include <lttoolbox/alphabet.h>
#include <lttoolbox/transducer.h>

#include <set>

using namespace std;

#define FIN_FICHERO - 1

/**
 * Compiler that builds a transducer to identify regular expressions.  This 
 * compiler is a recursive descendent parser (RDP).
 */
class RegexpCompiler
{
private:
  /**
   * Last token
   */
  int token;
 
  /**
   * Input string
   */
  wstring input;
 
  /**
   * Alphabet to encode symbols
   */
  Alphabet *alphabet;

  /**
   * Transducer to store analysis
   */
  Transducer transducer;
  
  /**
   * Current state
   */
  int state;
 
  /**
   * Current letter
   */
  int letter;

  /**
   * Post-operator: '+', '?', '*'
   */
  wstring postop;

  /**
   *
   */
  set<int> brackets;

  /**
   * Copy method
   * @param rec the regular expresion compiler to be copied
   */
  void copy(RegexpCompiler const &rec);

  /**
   * Destroy method
   */
  void destroy();

  /**
   * RDP top function
   */  
  void S();
  
  /**
   * RDP function
   */
  void RExpr();
  
  /**
   * RDP function
   */
  void Cola();
  
  /**
   * RDP function
   */
  void Term();
  
  /**
   * RDP function
   */
  void RExprp();
  
  /**
   * RDP function
   */
  void Letra();
  
  /**
   * RDP function
   */
  void Postop();
  
  /**
   * RDP function
   */
  void Esp();
  
  /**
   * RDP function
   */
  void Lista();
  
  /**
   * RDP function
   */
  void Reservado();
  
  /**
   * RDP function
   */
  void Elem();
  
  /**
   * RDP function
   */
  void ColaLetra();

  /**
   * Consume the input
   * @param t the input to be consumed
   */
  void consume(int t);

  /**
   * Error message function
   */
  void error();

  /**
   * Error message function
   * @param t the token being consumed
   */
  void errorConsuming(int const t);
  
  /**
   * Detect if it is a reserved token
   * @param t the token
   * @return true if the token is reserved
   */
  bool isReserved(int const t);
public:
  /**
   * Constructor
   */
  RegexpCompiler();
  
  /**
   * Destructor
   */
  ~RegexpCompiler();

  /**
   * Copy constructor
   * @param rec the regexp compiler to be copied
   */
  RegexpCompiler(RegexpCompiler const &rec);

  /**
   * Assignment operator
   * @param rec the regexp compiler to assign
   * @return the object result of the assignment
   */
  RegexpCompiler & operator = (RegexpCompiler const &rec);

  /**
   * Function that parses a regular expression and produces a transducer
   * @param er the regular expression
   */
  void compile(wstring const &er);

  /**
   * Set the decoder of symbols
   * @param a the alphabet
   */
  void setAlphabet(Alphabet *a);

  /**
   * Gets the transducer built
   * @return the transducer
   */
  Transducer & getTransducer();

  /**
   * Initializes the compiler
   * @param a the alphabet
   */
  void initialize(Alphabet *a); 
};

#endif
