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
#ifndef _XMLPARSEUTIL_
#define _XMLPARSEUTIL_

#include <libxml/encoding.h>
#include <libxml/xmlreader.h>
#include <lttoolbox/ustring.h>
#include <vector>
#include <cstdint>
#include <string>


class XMLParseUtil
{
public:

  static xmlTextReaderPtr open_or_exit(const char* fname);

  /* If attrib does not exist (or other error), returns fallback: */
  static UString attrib(xmlTextReaderPtr reader, UStringView name, UStringView fallback=u"");

  static std::string attrib_str(xmlTextReaderPtr reader, UStringView name);

  static UString readName(xmlTextReaderPtr reader);
  static UString readValue(xmlTextReaderPtr reader);
  static void readValueInto32(xmlTextReaderPtr reader, std::vector<int32_t>& vec);

  static bool allBlanks(xmlTextReaderPtr reader);

  static void error_and_die(xmlTextReaderPtr reader, const char* fmt, ...);
};

#endif
