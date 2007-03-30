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
#include <lttoolbox/Compression.H>
#include <lttoolbox/MyStdio.H>

#include <cstdlib>
#include <iostream>

void
Compression::multibyte_write(unsigned int value, FILE *output)
{
  if(value < 0x00000040)
  {
    unsigned char byte = (unsigned char) value;
    fwrite_unlocked(&byte, 1, 1, output);
  }
  else if(value < 0x00004000)
  {
    unsigned char low = (unsigned char) value;
    unsigned char up =  (unsigned char) (value >> 8);
    up = up | 0x40;
    fwrite_unlocked(&up, 1, 1, output);
    fwrite_unlocked(&low, 1, 1, output);
  }
  else if(value < 0x00400000)
  {
    unsigned char low = (unsigned char) value;
    unsigned char middle = (unsigned char) (value >> 8);
    unsigned char up = (unsigned char) (value >> 16);
    up = up | 0x80; 
    fwrite_unlocked(&up, 1, 1, output);
    fwrite_unlocked(&middle, 1, 1, output);
    fwrite_unlocked(&low, 1, 1, output);
  }
  else if(value < 0x40000000)
  {
    unsigned char low = (unsigned char) value;
    unsigned char middlelow = (unsigned char) (value >> 8);
    unsigned char middleup = (unsigned char) (value >> 16);
    unsigned char up = (unsigned char) (value >> 24);
    up = up | 0xc0; 
    fwrite_unlocked(&up, 1, 1, output);
    fwrite_unlocked(&middleup, 1, 1, output);
    fwrite_unlocked(&middlelow, 1, 1, output);
    fwrite_unlocked(&low, 1, 1, output);
  }
  else
  {
    cerr << "Out of range: " << value << endl;
    exit(EXIT_FAILURE);
  }
}

unsigned int
Compression::multibyte_read(FILE *input)
{
  unsigned char up;
  unsigned int result = 0;

  fread_unlocked(&up, 1, 1, input);
  if(up < 0x40)
  {
    result = (unsigned int) up;
  }
  else if(up < 0x80)
  {
    up = up & 0x3f;
    unsigned int aux = (unsigned int) up;
    aux = aux << 8;
    unsigned char low;
    fread_unlocked(&low, 1, 1, input);
    result = (unsigned int) low;
    result = result | aux;
  }
  else if(up < 0xc0)
  {
    up = up & 0x3f;
    unsigned int aux = (unsigned int) up;
    aux = aux << 8;
    unsigned char middle;
    fread_unlocked(&middle, 1, 1, input);
    result = (unsigned int) middle;
    aux = result | aux;
    aux = aux << 8;
    unsigned char low;
    fread_unlocked(&low, 1, 1, input);
    result = (unsigned int) low;
    result = result | aux;
  }
  else
  {
    up = up & 0x3f;
    unsigned int aux = (unsigned int) up;
    aux = aux << 8;
    unsigned char middleup;
    fread_unlocked(&middleup, 1, 1, input);
    result = (unsigned int) middleup;
    aux = result | aux;
    aux = aux << 8;
    unsigned char middlelow;
    fread_unlocked(&middlelow, 1, 1, input);
    result = (unsigned int) middlelow;
    aux = result | aux;
    aux = aux << 8;
    unsigned char low;
    fread_unlocked(&low, 1, 1, input);
    result = (unsigned int) low;
    result = result | aux;
  }
   
  return result;
}

void
Compression::wstring_write(wstring const &str, FILE *output)
{
  Compression::multibyte_write(str.size(), output);
  for(unsigned int i = 0, limit = str.size(); i != limit; i++)
  {
    Compression::multibyte_write(static_cast<int>(str[i]), output);
  }
}

wstring
Compression::wstring_read(FILE *input)
{
  wstring retval = L"";

  for(unsigned int i = 0, limit = Compression::multibyte_read(input);
      i != limit; i++)
  {
    retval += static_cast<wchar_t>(Compression::multibyte_read(input));
  }
  
  return retval;
}


void
Compression::multibyte_write(unsigned int value, ostream &output)
{
  if(value < 0x00000040)
  {
    unsigned char byte = (unsigned char) value;
    output.write(reinterpret_cast<char *>(&byte), sizeof(char));
  }
  else if(value < 0x00004000)
  {
    unsigned char low = (unsigned char) value;
    unsigned char up =  (unsigned char) (value >> 8);
    up = up | 0x40;
    output.write(reinterpret_cast<char *>(&up), sizeof(char));
    output.write(reinterpret_cast<char *>(&low), sizeof(char));
  }
  else if(value < 0x00400000)
  {
    unsigned char low = (unsigned char) value;
    unsigned char middle = (unsigned char) (value >> 8);
    unsigned char up = (unsigned char) (value >> 16);
    up = up | 0x80; 
    output.write(reinterpret_cast<char *>(&up), sizeof(char));
    output.write(reinterpret_cast<char *>(&middle), sizeof(char));
    output.write(reinterpret_cast<char *>(&low), sizeof(char));
    
  }
  else if(value < 0x40000000)
  {
    unsigned char low = (unsigned char) value;
    unsigned char middlelow = (unsigned char) (value >> 8);
    unsigned char middleup = (unsigned char) (value >> 16);
    unsigned char up = (unsigned char) (value >> 24);
    up = up | 0xc0; 

    output.write(reinterpret_cast<char *>(&up), sizeof(char));
    output.write(reinterpret_cast<char *>(&middleup), sizeof(char));
    output.write(reinterpret_cast<char *>(&middlelow), sizeof(char));
    output.write(reinterpret_cast<char *>(&low), sizeof(char));
  }
  else
  {
    cerr << "Out of range: " << value << endl;
    exit(EXIT_FAILURE);
  }
}

unsigned int
Compression::multibyte_read(istream &input)
{
  unsigned char up;
  unsigned int result = 0;

  input.read(reinterpret_cast<char *>(&up), sizeof(char));
  if(up < 0x40)
  {
    result = (unsigned int) up;
  }
  else if(up < 0x80)
  {
    up = up & 0x3f;
    unsigned int aux = (unsigned int) up;
    aux = aux << 8;
    unsigned char low;
    input.read(reinterpret_cast<char *>(&low), sizeof(char));
    result = (unsigned int) low;
    result = result | aux;
  }
  else if(up < 0xc0)
  {
    up = up & 0x3f;
    unsigned int aux = (unsigned int) up;
    aux = aux << 8;
    unsigned char middle;
    input.read(reinterpret_cast<char *>(&middle), sizeof(char));
    result = (unsigned int) middle;
    aux = result | aux;
    aux = aux << 8;
    unsigned char low;
    input.read(reinterpret_cast<char *>(&low), sizeof(char));
    result = (unsigned int) low;
    result = result | aux;
  }
  else
  {
    up = up & 0x3f;
    unsigned int aux = (unsigned int) up;
    aux = aux << 8;
    unsigned char middleup;
    input.read(reinterpret_cast<char *>(&middleup), sizeof(char));
    result = (unsigned int) middleup;
    aux = result | aux;
    aux = aux << 8;
    unsigned char middlelow;
    input.read(reinterpret_cast<char *>(&middlelow), sizeof(char));
    result = (unsigned int) middlelow;
    aux = result | aux;
    aux = aux << 8;
    unsigned char low;
    input.read(reinterpret_cast<char *>(&low), sizeof(char));
    result = (unsigned int) low;
    result = result | aux;
  }
   
  return result;
}

