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
#include <lttoolbox/compression.h>
#include <lttoolbox/my_stdio.h>

#include <cstdlib>
#include <cmath>
#include <limits>
#include <iostream>

void
Compression::writeByte(unsigned char byte, FILE *output)
{
  if(fwrite_unlocked(&byte, 1, 1, output) != 1)
  {
    wcerr << L"I/O Error writing" << endl;
    exit(EXIT_FAILURE);
  }
}

unsigned char
Compression::readByte(FILE *input)
{
  unsigned char value = 0;
  if(fread_unlocked(&value, 1, 1, input) != 1)
  {
//    Not uncomment this code since
//    wcerr << L"I/O Error reading" << endl;
//    exit(EXIT_FAILURE);
  }

  return value;
}

void
Compression::multibyte_write(unsigned int value, FILE *output)
{
  if(value < 0x00000040)
  {
    unsigned char byte = (unsigned char) value;
    writeByte(byte, output);
  }
  else if(value < 0x00004000)
  {
    unsigned char low = (unsigned char) value;
    unsigned char up =  (unsigned char) (value >> 8);
    up = up | 0x40;
    writeByte(up, output);
    writeByte(low, output);
  }
  else if(value < 0x00400000)
  {
    unsigned char low = (unsigned char) value;
    unsigned char middle = (unsigned char) (value >> 8);
    unsigned char up = (unsigned char) (value >> 16);
    up = up | 0x80;
    writeByte(up, output);
    writeByte(middle, output);
    writeByte(low, output);
  }
  else if(value < 0x40000000)
  {
    unsigned char low = (unsigned char) value;
    unsigned char middlelow = (unsigned char) (value >> 8);
    unsigned char middleup = (unsigned char) (value >> 16);
    unsigned char up = (unsigned char) (value >> 24);
    up = up | 0xc0;
    writeByte(up, output);
    writeByte(middleup, output);
    writeByte(middlelow, output);
    writeByte(low, output);
  }
  else
  {
    wcerr << L"Out of range: " << value << endl;
    exit(EXIT_FAILURE);
  }
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
    wcerr << "Out of range: " << value << endl;
    exit(EXIT_FAILURE);
  }
}


unsigned int
Compression::multibyte_read(FILE *input)
{
  unsigned int result = 0;

  unsigned char up = readByte(input);
  if(up < 0x40)
  {
    result = (unsigned int) up;
  }
  else if(up < 0x80)
  {
    up = up & 0x3f;
    unsigned int aux = (unsigned int) up;
    aux = aux << 8;
    unsigned char low = readByte(input);
    result = (unsigned int) low;
    result = result | aux;
  }
  else if(up < 0xc0)
  {
    up = up & 0x3f;
    unsigned int aux = (unsigned int) up;
    aux = aux << 8;
    unsigned char middle = readByte(input);
    result = (unsigned int) middle;
    aux = result | aux;
    aux = aux << 8;
    unsigned char low = readByte(input);
    result = (unsigned int) low;
    result = result | aux;
  }
  else
  {
    up = up & 0x3f;
    unsigned int aux = (unsigned int) up;
    aux = aux << 8;
    unsigned char middleup = readByte(input);
    result = (unsigned int) middleup;
    aux = result | aux;
    aux = aux << 8;
    unsigned char middlelow = readByte(input);
    result = (unsigned int) middlelow;
    aux = result | aux;
    aux = aux << 8;
    unsigned char low = readByte(input);
    result = (unsigned int) low;
    result = result | aux;
  }

  return result;
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


void
Compression::wstring_write(wstring const &str, FILE *output)
{
  Compression::multibyte_write(str.size(), output);
  for(auto c : str)
  {
    Compression::multibyte_write(static_cast<int>(c), output);
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
Compression::string_write(string const &str, FILE *output)
{
  Compression::multibyte_write(str.size(), output);
  for(auto c : str)
  {
    Compression::multibyte_write(static_cast<int>(c), output);
  }
}

string
Compression::string_read(FILE *input)
{
  string retval = "";

  for(unsigned int i = 0, limit = Compression::multibyte_read(input);
      i != limit; i++)
  {
    retval += static_cast<char>(Compression::multibyte_read(input));
  }

  return retval;
}


void
Compression::long_multibyte_write(const double& value, FILE *output)
{
  int exp = 0;

  unsigned int mantissa = static_cast<unsigned int>(static_cast<int>(0x40000000 * frexp(value, &exp)));
  unsigned int exponent = static_cast<unsigned int>(static_cast<int>(exp));

  if(mantissa < 0x04000000)
  {
    multibyte_write(mantissa, output);
  }
  else
  {
    unsigned int low_mantissa = (unsigned int) (mantissa << 6);
    low_mantissa = (unsigned int) (low_mantissa >> 6);
    unsigned int up_mantissa = (unsigned int) (mantissa >> 26);
    up_mantissa = up_mantissa | 0x04000000;
    multibyte_write(up_mantissa, output);
    multibyte_write(low_mantissa, output);
  }

  if(exponent < 0x04000000)
  {
    multibyte_write(exponent, output);
  }
  else
  {
    unsigned int low_exponent = (unsigned int) (exponent << 6);
    low_exponent = (unsigned int) (low_exponent >> 6);
    unsigned int up_exponent = (unsigned int) (exponent >> 26);
    up_exponent = up_exponent | 0x04000000;
    multibyte_write(up_exponent, output);
    multibyte_write(low_exponent, output);
  }
}

void
Compression::long_multibyte_write(const double& value, ostream &output)
{
  int exp = 0;

  unsigned int mantissa = static_cast<unsigned int>(static_cast<int>(0x40000000 * frexp(value, &exp)));
  unsigned int exponent = static_cast<unsigned int>(static_cast<int>(exp));

  if(mantissa < 0x04000000)
  {
    multibyte_write(mantissa, output);
  }
  else
  {
    unsigned int low_mantissa = (unsigned int) (mantissa << 6);
    low_mantissa = (unsigned int) (low_mantissa >> 6);
    unsigned int up_mantissa = (unsigned int) (mantissa >> 26);
    up_mantissa = up_mantissa | 0x04000000;
    multibyte_write(up_mantissa, output);
    multibyte_write(low_mantissa, output);
  }

  if(exponent < 0x04000000)
  {
    multibyte_write(exponent, output);
  }
  else
  {
    unsigned int low_exponent = (unsigned int) (exponent << 6);
    low_exponent = (unsigned int) (low_exponent >> 6);
    unsigned int up_exponent = (unsigned int) (exponent >> 26);
    up_exponent = up_exponent | 0x04000000;
    multibyte_write(up_exponent, output);
    multibyte_write(low_exponent, output);
  }
}

double
Compression::long_multibyte_read(FILE *input)
{
  double result = 0.0;

  unsigned int mantissa = 0;
  unsigned int exponent = 0;

  unsigned int up_mantissa = multibyte_read(input);
  if(up_mantissa < 0x04000000)
  {
    mantissa = up_mantissa;
  }
  else
  {
    up_mantissa = up_mantissa & 0x03ffffff;
    unsigned int aux = (unsigned int) up_mantissa;
    aux = aux << 26;
    unsigned int low_mantissa = multibyte_read(input);
    mantissa = (unsigned int) (low_mantissa);
    mantissa = mantissa | aux;
  }
  unsigned int up_exponent = multibyte_read(input);
  if(up_exponent < 0x04000000)
  {
    exponent = up_exponent;
  }
  else
  {
    up_exponent = up_exponent & 0x03ffffff;
    unsigned int aux = (unsigned int) up_exponent;
    aux = aux << 26;
    unsigned int low_exponent = multibyte_read(input);
    exponent = (unsigned int) (low_exponent);
    exponent = exponent | aux;
  }

  double value = static_cast<double>(static_cast<int>(mantissa)) / 0x40000000;
  result = ldexp(value, static_cast<int>(exponent));

  return result;
}

double
Compression::long_multibyte_read(istream &input)
{
  double result = 0.0;

  unsigned int mantissa = 0;
  unsigned int exponent = 0;

  unsigned int up_mantissa = multibyte_read(input);
  if(up_mantissa < 0x04000000)
  {
    mantissa = up_mantissa;
  }
  else
  {
    up_mantissa = up_mantissa & 0x03ffffff;
    unsigned int aux = (unsigned int) up_mantissa;
    aux = aux << 26;
    unsigned int low_mantissa = multibyte_read(input);
    mantissa = (unsigned int) (low_mantissa);
    mantissa = mantissa | aux;
  }
  unsigned int up_exponent = multibyte_read(input);
  if(up_exponent < 0x04000000)
  {
    exponent = up_exponent;
  }
  else
  {
    up_exponent = up_exponent & 0x03ffffff;
    unsigned int aux = (unsigned int) up_exponent;
    aux = aux << 26;
    unsigned int low_exponent = multibyte_read(input);
    exponent = (unsigned int) (low_exponent);
    exponent = exponent | aux;
  }

  double value = static_cast<double>(static_cast<int>(mantissa)) / 0x40000000;
  result = ldexp(value, static_cast<int>(exponent));

  return result;
}
