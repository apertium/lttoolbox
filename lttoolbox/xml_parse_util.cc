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


UString
XMLParseUtil::attrib(xmlTextReaderPtr reader, UString const &name)
{
  return attrib(reader, name, ""_u);
}

UString
XMLParseUtil::attrib(xmlTextReaderPtr reader, UString const& name, const UString& fallback)
{
  std::string temp;
  temp.reserve(name.size());
  utf8::utf16to8(name.begin(), name.end(), std::back_inserter(temp));
  const xmlChar *attrname = reinterpret_cast<const xmlChar*>(temp.c_str());
  xmlChar *myattr = xmlTextReaderGetAttribute(reader, attrname);
  if(myattr == NULL) {
    xmlFree(myattr);
    return fallback;
  } else {
    UString result = to_ustring(reinterpret_cast<char*>(myattr));
    xmlFree(myattr);
    return result;
  }
}

std::string
XMLParseUtil::attrib_str(xmlTextReaderPtr reader, const UString& name)
{
  std::string temp;
  temp.reserve(name.size());
  utf8::utf16to8(name.begin(), name.end(), std::back_inserter(temp));
  const xmlChar *attrname = reinterpret_cast<const xmlChar*>(temp.c_str());
  xmlChar *myattr = xmlTextReaderGetAttribute(reader, attrname);
  if(myattr == NULL) {
    xmlFree(myattr);
    return "";
  } else {
    std::string result = reinterpret_cast<char*>(myattr);
    xmlFree(myattr);
    return result;
  }
}

UString
XMLParseUtil::readName(xmlTextReaderPtr reader)
{
  const xmlChar* name = xmlTextReaderConstName(reader);
  if (name == NULL) return ""_u;
  return to_ustring(reinterpret_cast<const char*>(name));
}

UString
XMLParseUtil::readValue(xmlTextReaderPtr reader)
{
  const xmlChar* val = xmlTextReaderConstValue(reader);
  if (val == NULL) return ""_u;
  return to_ustring(reinterpret_cast<const char*>(val));
}

void
XMLParseUtil::readValueInto32(xmlTextReaderPtr reader, std::vector<int32_t>& vec)
{
  const xmlChar* val = xmlTextReaderConstValue(reader);
  if (val == NULL) return;
  auto sz = xmlStrlen(val);
  vec.reserve(vec.size() + sz);
  utf8::utf8to32(val, val+sz, std::back_inserter(vec));
}
