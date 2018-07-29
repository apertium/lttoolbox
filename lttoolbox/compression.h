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
#ifndef _COMPRESSION_
#define _COMPRESSION_

#include <cstdio>
#include <string>
#include <iostream>

using namespace std;

/**
 * Clase "Compression".
 * Class methods to access compressed data by the byte-aligned method
 */
class Compression
{
private:
  /**
   * Writing a byte
   * @param byte char to write.
   * @param output output stream.
   */
  static void writeByte(unsigned char byte, FILE *output);

  /**
   * Readinging a byte
   * @param input input stream
   * @return the value of the next byte in the input
   */
  static unsigned char readByte(FILE *input);

public:
  /**
   * Encodes an integer value and writes it into the output stream
   * @see multibyte_read()
   * @param value integer to write.
   * @param output output stream.
   */
  static void multibyte_write(unsigned int value, FILE *output);

  /**
   * Encodes an integer value and writes it into the output stream
   * @see multibyte_read()
   * @param value integer to write.
   * @param output output stream.
   */
  static void multibyte_write(unsigned int value, ostream &os);
  
  /**
   * Read and decode an integer from the input stream.
   * @see multibyte_read()
   * @param input input stream.
   * @return the integer value readed.
   */
  static unsigned int multibyte_read(FILE *input);

  /**
   * Read and decode an integer from the input stream.
   * @see multibyte_read()
   * @param input input stream.
   * @return the integer value readed.
   */
  static unsigned int multibyte_read(istream &is);

  /**
   * This method allows to write a wide string to an output stream
   * using its UCSencoding as integer.
   * @see wstring_read()
   * @param str the string to write.
   * @param output the output stream.
   */
  static void wstring_write(wstring const &str, FILE *output);
  
  /**
   * This method reads a wide string from the input stream.
   * @see wstring_write()
   * @param input the input stream.
   * @return the wide string read.
   */
  static wstring wstring_read(FILE *input);

  /**
   * This method allows to write a plain string to an output stream
   * using its UCSencoding as integer.
   * @see string_read()
   * @param str the string to write.
   * @param output the output stream.
   */
  static void string_write(string const &str, FILE *output);

  /**
   * This method reads a plain string from the input stream.
   * @see string_write()
   * @param input the input stream.
   * @return the string read.
   */
  static string string_read(FILE *input);

  /**
   * Encodes a double value and writes it into the output stream
   * @see long_multibyte_read()
   * @param value double to write.
   * @param output output stream.
   */
  static void long_multibyte_write(const double& value, FILE *output);

  /**
   * Encodes a double value and writes it into the output stream
   * @see long_multibyte_read()
   * @param value double to write.
   * @param output output stream.
   */
  static void long_multibyte_write(const double& value, ostream &os);

  /**
   * Read and decode a double from the input stream.
   * @see long_multibyte_read()
   * @param input input stream.
   * @return the double value read.
   */
  static double long_multibyte_read(FILE *input);

  /**
   * Read and decode a double from the input stream.
   * @see long_multibyte_read()
   * @param input input stream.
   * @return the double value read.
   */
  static double long_multibyte_read(istream &is);
};

#endif
