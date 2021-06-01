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
#include <lttoolbox/xml_parse_util.h>

#include <cstdlib>
#include <iostream>
#include <utf8.h>

using namespace std;

UString
XMLParseUtil::attrib(xmlTextReaderPtr reader, UString const &name)
{
  return attrib(reader, name, ""_u);
}

UString
XMLParseUtil::attrib(xmlTextReaderPtr reader, UString const &name, const UString fallback)
{
  std::string temp;
  temp.reserve(name.size());
  utf8::utf16to8(name.begin(), name.end(), std::back_inserter(temp));
  xmlChar *attrname = xmlCharStrdup(temp.c_str());
  xmlChar *myattr = xmlTextReaderGetAttribute(reader, attrname);
  xmlFree(attrname);
  if(myattr == NULL) {
    xmlFree(myattr);
    return fallback;
  } else {
    UString result = to_ustring(reinterpret_cast<char*>(myattr));
    xmlFree(myattr);
    return result;
  }
}

UString
XMLParseUtil::readName(xmlTextReaderPtr reader)
{
  const xmlChar* name = xmlTextReaderConstName(reader);
  return to_ustring(reinterpret_cast<const char*>(name));
}

UString
XMLParseUtil::readValue(xmlTextReaderPtr reader)
{
  const xmlChar* val = xmlTextReaderConstValue(reader);
  return to_ustring(reinterpret_cast<const char*>(val));
}
