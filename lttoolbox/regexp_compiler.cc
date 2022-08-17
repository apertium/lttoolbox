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
#include <lttoolbox/regexp_compiler.h>

#include <cstdlib>
#include <iostream>
#include <climits>

#define FIN_FICHERO INT_MAX

RegexpCompiler::RegexpCompiler()
{
}

RegexpCompiler::~RegexpCompiler()
{
}

RegexpCompiler::RegexpCompiler(RegexpCompiler const &rec)
{
  copy(rec);
}

RegexpCompiler &
RegexpCompiler::operator =(RegexpCompiler const &rec)
{
  if(this != &rec)
  {
    copy(rec);
  }

  return *this;
}

void
RegexpCompiler::copy(RegexpCompiler const &rec)
{
  token = rec.token;
  input = rec.input;
  transducer = rec.transducer;
  letter = rec.letter;
  alphabet = rec.alphabet;
  state = rec.state;
  letter = rec.letter;
  postop = rec.postop;
  default_weight = rec.default_weight;
}

bool
RegexpCompiler::isReserved(int const t)
{
  switch(t)
  {
    case '(':
    case ')':
    case '[':
    case ']':
    case '*':
    case '?':
    case '+':
    case '-':
    case '^':
    case '\\':
    case '|':
    case FIN_FICHERO:
      return true;

    default:
      return false;
  }
}

void
RegexpCompiler::error()
{
  std::cerr << "Error parsing regexp" << std::endl;
  exit(EXIT_FAILURE);
}

void
RegexpCompiler::errorConsuming(int const t)
{
  std::cerr << "Error parsing regexp" << std::endl;
  exit(EXIT_FAILURE);
}

void
RegexpCompiler::consume(int const t)
{
  if(token == t)
  {
    index++;
    if(index == input.size())
    {
      token = FIN_FICHERO;
    }
    else
    {
      token = input[index];
    }
  }
  else
  {
    errorConsuming(t);
  }
}

void
RegexpCompiler::compile(std::vector<int32_t> const &er)
{
  input = er;
  token = input[0];
  index = 0;
  state = transducer.getInitial();
  S();
  transducer.setFinal(state, default_weight);
}

void
RegexpCompiler::S()
{
  if(token == '(' || token == '[' || !isReserved(token) || token == '\\')
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
RegexpCompiler::RExpr()
{
  if(token == '(' || token == '[' || !isReserved(token) || token == '\\')
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
RegexpCompiler::Cola()
{
  if(token == FIN_FICHERO || token == ')')
  {
  }
  else if(token == '|')
  {
    int e = state;
    state = transducer.getInitial();
    consume('|');
    RExpr();
    Cola();

    state = transducer.insertNewSingleTransduction((*alphabet)(0, 0), state, default_weight);
    transducer.linkStates(e, state, (*alphabet)(0, 0), default_weight);
  }
  else
  {
    error();
  }
}

void
RegexpCompiler::Term()
{
  if(!isReserved(token) || token == '\\')
  {
    Transducer t;
    int e = t.getInitial();
    Letra();
    e = t.insertNewSingleTransduction((*alphabet)(letter, letter), e, default_weight);
    t.setFinal(e, default_weight);
    Postop();
    if(postop == '*')
    {
      t.zeroOrMore((*alphabet)(0, 0));
    }
    else if(postop == '+')
    {
      t.oneOrMore((*alphabet)(0, 0));
    }
    else if(postop == '?')
    {
      t.optional((*alphabet)(0, 0));
    }

    postop = 0;
    state = transducer.insertTransducer(state, t, (*alphabet)(0, 0));
  }
  else if(token == '(')
  {
    Transducer t = transducer;
    int e = state;
    transducer.clear();
    state = transducer.getInitial();
    consume('(');
    S();
    consume(')');
    transducer.setFinal(state, default_weight);
    Postop();
    if(postop == '*')
    {
      transducer.zeroOrMore((*alphabet)(0, 0));
    }
    else if(postop == '+')
    {
      transducer.oneOrMore((*alphabet)(0, 0));
    }
    else if(postop == '?')
    {
      transducer.optional((*alphabet)(0, 0));
    }

    postop = 0;
    state = t.insertTransducer(e, transducer, (*alphabet)(0, 0));
    transducer = t;
  }
  else if(token == '[')
  {
    consume('[');
    Esp();
  }
  else
  {
    error();
  }
}

void
RegexpCompiler::RExprp()
{
  if(token == '(' || token == '[' || !isReserved(token) || token == '\\')
  {
    Term();
    RExprp();
  }
  else if(token == '|' || token == FIN_FICHERO || token == ')')
  {
  }
  else
  {
    error();
  }
}

void
RegexpCompiler::Letra()
{
  if(!isReserved(token))
  {
    letter = token;
    consume(token);
  }
  else if(token == '\\')
  {
    consume('\\');
    letter = token;
    Reservado();
  }
  else
  {
    error();
  }
}

void
RegexpCompiler::Postop()
{
  if(token == '*')
  {
    consume('*');
    postop = '*';
  }
  else if(token == '?')
  {
    consume('?');
    postop = '?';
  }
  else if(token == '+')
  {
    consume('+');
    postop = '+';
  }
  else if(token == '(' || token == '[' || !isReserved(token) ||
          token == '\\' || token == '|' ||  token == FIN_FICHERO ||
          token == ')')
  {
  }
  else
  {
    error();
  }
}

void
RegexpCompiler::Esp()
{
  Transducer t;
  if(!isReserved(token) || token == '\\' || token == ']')
  {
    Lista();
    consume(']');
    Postop();

    for(auto it = brackets.begin();
        it != brackets.end(); it++)
    {
      int mystate = t.getInitial();
      mystate = t.insertNewSingleTransduction((*alphabet)(0, 0), mystate, default_weight);
      mystate = t.insertNewSingleTransduction((*alphabet)(*it, *it), mystate, default_weight);
      t.setFinal(mystate, default_weight);
    }

    t.joinFinals((*alphabet)(0, 0));
  }
  else if(token == '^')
  {
    consume('^');
    Lista();
    consume(']');
    Postop();

    for(int i = 0; i < 256 ;i++)
    {
      if(brackets.find(i) == brackets.end())
      {
        int mystate = t.getInitial();
        mystate = t.insertNewSingleTransduction((*alphabet)(0, 0), mystate, default_weight);
        mystate = t.insertNewSingleTransduction((*alphabet)(i, i), mystate, default_weight);
        t.setFinal(mystate, default_weight);
      }
    }

    t.joinFinals((*alphabet)(0, 0));
  }
  else
  {
    error();
  }

  if(postop == '+')
  {
    t.oneOrMore((*alphabet)(0, 0));
  }
  else if(postop == '*')
  {
    t.zeroOrMore((*alphabet)(0, 0));
  }
  else if(postop == '?')
  {
    t.optional((*alphabet)(0, 0));
  }
  brackets.clear();
  postop = 0;

  state = transducer.insertTransducer(state, t, (*alphabet)(0, 0));
}

void
RegexpCompiler::Lista()
{
  if(!isReserved(token) || token == '\\')
  {
    Elem();
    Lista();
  }
  else if(token == ']')
  {
  }
  else
  {
    error();
  }
}

void
RegexpCompiler::Reservado()
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
RegexpCompiler::Elem()
{
  if(!isReserved(token) || token == '\\')
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
RegexpCompiler::ColaLetra()
{
  if(token == '-')
  {
    consume('-');
    Letra();
  }
  else if(!isReserved(token) || token == '\\' || token == ']')
  {
  }
  else
  {
    error();
  }
}

void
RegexpCompiler::setAlphabet(Alphabet *a)
{
  alphabet = a;
}

Transducer &
RegexpCompiler::getTransducer()
{
  return transducer;
}

void
RegexpCompiler::initialize(Alphabet *a)
{
  setAlphabet(a);
  transducer.clear();
  brackets.clear();
  postop = 0;
}
