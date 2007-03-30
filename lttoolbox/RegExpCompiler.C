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
#include <lttoolbox/RegExpCompiler.H>

#include <cstdlib>
#include <iostream>

RegExpCompiler::RegExpCompiler()
{
}

RegExpCompiler::~RegExpCompiler()
{
  destroy();
}

RegExpCompiler::RegExpCompiler(RegExpCompiler const &rec)
{
  copy(rec);
}

RegExpCompiler &
RegExpCompiler::operator =(RegExpCompiler const &rec)
{
  if(this != &rec)
  {
    destroy();
    copy(rec);
  }

  return *this;
}

void
RegExpCompiler::copy(RegExpCompiler const &rec)
{
  token = rec.token;
  input = rec.input;
  transducer = rec.transducer;
  letter = rec.letter;
  alphabet = rec.alphabet;
  state = rec.state;
  letter = rec.letter;
  postop = rec.postop;
}

void
RegExpCompiler::destroy()
{
}

bool 
RegExpCompiler::isReserved(int const t)
{
  switch(t) 
  {
    case L'(':
    case L')':
    case L'[':
    case L']':
    case L'*':
    case L'?':
    case L'+':
    case L'-':
    case L'^':
    case L'\\':
    case L'|':
    case FIN_FICHERO:
      return true;
 
    default:
      return false;
  }
}

void
RegExpCompiler::error()
{
  cerr << "Error de análisis de expresión regular\n";
  exit(EXIT_FAILURE);
}

void 
RegExpCompiler::errorConsuming(int const t)
{
  cerr << "Expresión regular: Error de análisis en '" << char(t) << "'\n";
  exit(EXIT_FAILURE);
}

void 
RegExpCompiler::consume(int const t)
{
  if(token == t)
  {
    input = input.substr(1);
    if(input ==  L"")
    {
      token = FIN_FICHERO;
    }
    else
    {
      token = input[0];
    }
  }
  else
  {
    errorConsuming(t);
  }
}

void 
RegExpCompiler::compile(wstring const &er)
{
  input = er;
  token = static_cast<int>(input[0]);
  state = transducer.getInitial();
  S();
  transducer.setFinal(state);
}

void 
RegExpCompiler::S()
{
  if(token == L'(' || token == L'[' || !isReserved(token) || token == L'\\')
  {
    RExpr();
    Cola();
  }
  else
  {
    error();
  }
}

void 
RegExpCompiler::RExpr()
{
  if(token == L'(' || token == L'[' || !isReserved(token) || token == L'\\')
  {
    Term();
    RExprp();
  }
  else
  {
    error();
  }
}

void 
RegExpCompiler::Cola()
{
  if(token == FIN_FICHERO || token == L')')
  {
  }
  else if(token == L'|')
  {
    int e = state;
    state = transducer.getInitial();
    consume('|');
    RExpr();
    Cola();
   
    state = transducer.insertNewSingleTransduction((*alphabet)(0, 0), state);
    transducer.linkStates(e, state, (*alphabet)(0, 0));
  }
  else
  {
    error();
  }
}

void 
RegExpCompiler::Term()
{
  if(!isReserved(token) || token == L'\\')
  {
    Transducer t;
    int e = t.getInitial();
    Letra();
    e = t.insertNewSingleTransduction((*alphabet)(letter, letter), e);
    t.setFinal(e);
    Postop();
    if(postop == L"*")
    {
      t.zeroOrMore((*alphabet)(0, 0));
    }
    else if(postop == L"+")
    {
      t.oneOrMore((*alphabet)(0, 0));
    }
    else if(postop == L"?")
    {
      t.optional((*alphabet)(0, 0));
    }

    postop = L"";
    state = transducer.insertTransducer(state, t, (*alphabet)(0, 0));
  }
  else if(token == L'(')
  {
    Transducer t = transducer;
    int e = state;
    transducer.clear();
    state = transducer.getInitial();
    consume(L'(');
    S();
    consume(L')');
    transducer.setFinal(state);
    Postop();
    if(postop == L"*")
    {
      transducer.zeroOrMore((*alphabet)(0, 0));
    }
    else if(postop == L"+")
    {
      transducer.oneOrMore((*alphabet)(0, 0));
    }
    else if(postop == L"?")
    {
      transducer.optional((*alphabet)(0, 0));
    }

    postop = L"";
    state = t.insertTransducer(e, transducer, (*alphabet)(0, 0));
    transducer = t;
  }
  else if(token == L'[')
  {
    consume(L'[');
    Esp();
  }
  else
  {
    error();
  }
}

void 
RegExpCompiler::RExprp()
{
  if(token == L'(' || token == L'[' || !isReserved(token) || token == L'\\')
  {
    Term();
    RExprp();
  }
  else if(token == L'|' || token == FIN_FICHERO || token == L')')
  {
  }
  else
  {
    error();
  }
}

void
RegExpCompiler::Letra()
{
  if(!isReserved(token))
  {
    letter = token;
    consume(token);
  }
  else if(token == L'\\')
  {
    consume(L'\\');
    letter = token;
    Reservado();
  }
  else
  {
    error();
  }
}

void
RegExpCompiler::Postop()
{
  if(token == L'*')
  {
    consume(L'*');
    postop = L"*";
  }
  else if(token == L'?')
  {
    consume(L'?');
    postop = L"?";
  }
  else if(token == L'+')
  {
    consume(L'+');
    postop = L"+";
  }
  else if(token == L'(' || token == L'[' || !isReserved(token) || 
          token == L'\\' || token == L'|' ||  token == FIN_FICHERO || 
	  token == L')')
  {
  }
  else
  {
    error();
  }
}

void
RegExpCompiler::Esp()
{
  Transducer t;
  if(!isReserved(token) || token == L'\\' || token == L']')
  {
    Lista();
    consume(L']');
    Postop();

    for(set<int>::iterator it = brackets.begin(); 
        it != brackets.end(); it++)
    {
      int mystate = t.getInitial();
      mystate = t.insertNewSingleTransduction((*alphabet)(0, 0), mystate);
      mystate = t.insertNewSingleTransduction((*alphabet)(*it, *it), mystate);
      t.setFinal(mystate);
    }

    t.joinFinals((*alphabet)(0, 0));
  }
  else if(token == L'^')
  {
    consume(L'^');
    Lista();
    consume(L']');
    Postop();
   
    for(int i = 0; i < 256 ;i++)
    {
      if(brackets.find(i) == brackets.end())
      {
        int mystate = t.getInitial();
        mystate = t.insertNewSingleTransduction((*alphabet)(0, 0), mystate);
        mystate = t.insertNewSingleTransduction((*alphabet)(i, i), mystate);
	t.setFinal(mystate);
      }
    }
    
    t.joinFinals((*alphabet)(0, 0));
  }
  else
  {
    error();
  }

  if(postop == L"+")
  {
    t.oneOrMore((*alphabet)(0, 0));     
  }
  else if(postop == L"*")
  {
    t.zeroOrMore((*alphabet)(0, 0));
  }
  else if(postop == L"?")
  {
    t.optional((*alphabet)(0, 0));
  }
  brackets.clear();
  postop = L"";

  state = transducer.insertTransducer(state, t, (*alphabet)(0, 0));
}

void 
RegExpCompiler::Lista()
{
  if(!isReserved(token) || token == L'\\')
  {
    Elem();
    Lista();
  }
  else if(token == L']')
  {
  }
  else
  {
    error();
  }
}

void 
RegExpCompiler::Reservado()
{
  if(isReserved(token))
  {
    consume(token);
  }
  else
  {
    error();
  }
}

void 
RegExpCompiler::Elem()
{
  if(!isReserved(token) || token == L'\\')
  {
    Letra();
    int rango1 = letter;
    ColaLetra();
    int rango2 = letter;

    if(rango1 > rango2)
    {
      error();
    }
    else
    {
      for(int i = rango1; i <= rango2; i++)
      {
        brackets.insert(i);
      }
    }
  }
  else
  {
    error();
  }
}

void
RegExpCompiler::ColaLetra()
{
  if(token == L'-')
  {
    consume(L'-');
    Letra();
  }
  else if(!isReserved(token) || token == L'\\' || token == L']')
  {
  }
  else
  {
    error();
  }
}

void
RegExpCompiler::setAlphabet(Alphabet *a)
{
  alphabet = a;
}

Transducer &
RegExpCompiler::getTransducer()
{
  return transducer;
}

void
RegExpCompiler::initialize(Alphabet *a)
{
  setAlphabet(a);
  transducer.clear();
  brackets.clear();
  postop = L"";
}
