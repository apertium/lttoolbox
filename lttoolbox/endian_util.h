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
#ifndef _ENDIANUTIL_
#define _ENDIANUTIL_

#include <cctype>
#include <cstdio>
#include <iostream>

using namespace std;


/**
 * Some definitions copied from somewhere in glibc
 */
#ifndef __BYTE_ORDER
#define	__LITTLE_ENDIAN	1234
#define	__BIG_ENDIAN	4321
#define	__PDP_ENDIAN	3412
#define __BYTE_ORDER __BIG_ENDIAN
#endif


/**
 * Generic class to process correctly endian-enabled I/O operations
 */
template<class T>
class EndianUtil
{
public:
  /**
   * Read procedure.
   * @param input the stream to read from.
   * @returns the first element readed from the current position of the stream
   */
  static T read(FILE *input)
  {
    T retval;
#if __BYTE_ORDER == __BIG_ENDIAN
    fread(&retval, sizeof(T), 1, input);
#else
    char *s = reinterpret_cast<char *>(&retval);

    for(int i = sizeof(T)-1; i != -1; i--)
    {
      fread(&(s[i]), 1, 1, input);
    } 
#endif
    return retval;
  }

  /**
   * Read procedure, C++ I/O version.
   * @param is the stream to read from.
   * @returns the first element readed from the current position of the stream
   */
  static T read(istream &is)
  {
    T retval;
#if __BYTE_ORDER == __BIG_ENDIAN
    is.read((char *) &retval, sizeof(T));
#else
    char *s = reinterpret_cast<char *>(&retval);

    for(int i = sizeof(T)-1; i != -1; i--)
    {
      is.read(&(s[i]), sizeof(char));
    } 
#endif
    return retval;    
  }
  
  /**
   * Write procedure.
   * @param output the stream to write to
   * @param val the value of the generic object to write to the stream
   */
  static void write(FILE *output, T const &val)
  {
    T val2 = val;
#if __BYTE_ORDER == __BIG_ENDIAN
    fwrite(&val2, sizeof(T), 1, output);
#else
    char *s = reinterpret_cast<char *>(&val2);
    
    for(int i = sizeof(T)-1; i != -1; i--)
    {
      fwrite(&(s[i]), 1, 1, output);
    }
#endif
  }
  
  /**
   * Write procedure, C++ I/O version.
   * @param output the stream to write to
   * @param val the value of the generic object to write to the stream
   */
  static void write(ostream &os, T const &val)
  {
    T val2 = val;
#if __BYTE_ORDER == __BIG_ENDIAN
    os.write(reinterpret_cast<char *>(&val2), sizeof(T));
#else
    char *s = reinterpret_cast<char *>(&val2);
    
    for(int i = sizeof(T)-1; i != -1; i--)
    {
      os.write(&(s[i]), sizeof(char));
    }
#endif
  }
};

#endif
