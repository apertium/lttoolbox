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
#include <lttoolbox/xml_parse_util.h>

#include <cstdlib>
#include <iostream>

using namespace std;

wstring
XMLParseUtil::attrib(xmlTextReaderPtr reader, wstring const &name)
{
  string mystr = "";
  for(int i = 0, limit = name.size(); i != limit; i++)
  {
    mystr += static_cast<char>(name[i]);
  }
 
  xmlChar *attrname = xmlCharStrdup(mystr.c_str());
  xmlChar *myattr = xmlTextReaderGetAttribute(reader, attrname);
  wstring result = towstring(myattr);
  xmlFree(myattr);
  xmlFree(attrname);
  return result;
}

string
XMLParseUtil::latin1(xmlChar const *input)
{
 if(input == NULL)
  {
    return "";
  }

  int outputlen = xmlStrlen(input) + 1;
  int inputlen = xmlStrlen(input);

  unsigned char* output = new unsigned char[outputlen];
  
  if(UTF8Toisolat1(output, &outputlen, input, &inputlen) != 0)
  {
  }

  output[outputlen] = 0;
  string result = reinterpret_cast<char *>(output);
  delete[] output;
  return result;  
}

wstring
XMLParseUtil::towstring(xmlChar const * input)
{ 
  wstring result = L"";
  
  for(int i = 0, limit = xmlStrlen(input); i != limit; i++)
  {
    int val = 0;
    if(((unsigned char) input[i] & 0x80) == 0x0)
    {
      val = static_cast<wchar_t>(input[i]);
    }
    else if(((unsigned char) input[i] & 0xE0) == 0xC0)
    {
      val = (input[i] & 0x1F) << 6;
      i++;
      val += input[i] & 0x7F;  
    }
    else if(((unsigned char) input[i] & 0xF0) == 0xE0)
    {
      val = (input[i] & 0x0F) << 6;
      i++;
      val += input[i] & 0x7F;
      val = val << 6;
      i++;
      val += input[i] & 0x7F;
    }
    else if(((unsigned char) input[i] & 0xF8) == 0xF0)
    {
      val = (input[i] & 0x07) << 6;
      i++;
      val += input[i] & 0x7F;
      val = val << 6;
      i++;
      val += input[i] & 0x7F;
      val = val << 6;
      i++;
      val += input[i] & 0x7F;
    }
    else
    {
      wcerr << L"UTF-8 invalid string" << endl;
      exit(EXIT_FAILURE);  
    }
    
    result += static_cast<wchar_t>(val);
  }
  return result;
}

wstring 
XMLParseUtil::stows(string const &str)
{
  wchar_t* result = new wchar_t[str.size()+1];
  size_t retval = mbstowcs(result, str.c_str(), str.size());
  result[retval] = L'\0';
  wstring result2 = result;
  delete[] result;
  return result2;
}
