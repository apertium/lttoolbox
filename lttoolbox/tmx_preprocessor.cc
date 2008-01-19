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
#include <lttoolbox/tmx_preprocessor.h>
#include <lttoolbox/lt_locale.h>
#include <lttoolbox/xml_parse_util.h>

#include <cstdlib>
#include <iostream>
#include <libxml/encoding.h>

using namespace std;

wstring const TMXPreprocessor::TMX_BODY_ELEM 	= L"body";
wstring const TMXPreprocessor::TMX_HEADER_ELEM  = L"header";
wstring const TMXPreprocessor::TMX_MAP_ELEM     = L"map";
wstring const TMXPreprocessor::TMX_NOTE_ELEM    = L"note";
wstring const TMXPreprocessor::TMX_PROP_ELEM    = L"prop";
wstring const TMXPreprocessor::TMX_SEG_ELEM     = L"seg";
wstring const TMXPreprocessor::TMX_TMX_ELEM     = L"tmx";
wstring const TMXPreprocessor::TMX_TU_ELEM      = L"tu";
wstring const TMXPreprocessor::TMX_TUV_ELEM     = L"tuv";
wstring const TMXPreprocessor::TMX_UDE_ELEM 	= L"ude";
wstring const TMXPreprocessor::TMX_BPT_ELEM 	= L"bpt";
wstring const TMXPreprocessor::TMX_EPT_ELEM 	= L"ept";
wstring const TMXPreprocessor::TMX_HI_ELEM 	= L"hi";
wstring const TMXPreprocessor::TMX_IT_ELEM      = L"it";
wstring const TMXPreprocessor::TMX_PH_ELEM      = L"ph";
wstring const TMXPreprocessor::TMX_SUB_ELEM     = L"sub";
wstring const TMXPreprocessor::TMX_UT_ELEM      = L"ut";
wstring const TMXPreprocessor::TMX_ADMINLANG_ATTR 	= L"adminlang";
wstring const TMXPreprocessor::TMX_ASSOC_ATTR   	= L"assoc";
wstring const TMXPreprocessor::TMX_BASE_ATTR	    	= L"base";
wstring const TMXPreprocessor::TMX_CHANGEDATE_ATTR	= L"changedate";
wstring const TMXPreprocessor::TMX_CHANGEID_ATTR 	= L"changeid";
wstring const TMXPreprocessor::TMX_CODE_ATTR         	= L"code";
wstring const TMXPreprocessor::TMX_CREATIONDATE_ATTR    = L"creationdate";
wstring const TMXPreprocessor::TMX_CREATIONID_ATTR      = L"creationid";
wstring const TMXPreprocessor::TMX_CREATIONTOOL_ATTR    = L"creationtool";
wstring const TMXPreprocessor::TMX_CREATIONTOOLVERSION_ATTR = L"creationtoolversion";
wstring const TMXPreprocessor::TMX_DATATYPE_ATTR      	= L"datatype";
wstring const TMXPreprocessor::TMX_ENT_ATTR	    	= L"ent";
wstring const TMXPreprocessor::TMX_I_ATTR	    	= L"i";
wstring const TMXPreprocessor::TMX_LASTUSAGEDATE_ATTR 	= L"lastusagedate";
wstring const TMXPreprocessor::TMX_NAME_ATTR         	= L"name";
wstring const TMXPreprocessor::TMX_O_ENCODING_ATTR      = L"o-encoding";
wstring const TMXPreprocessor::TMX_O_TMF_ATTR        	= L"o-tmf";
wstring const TMXPreprocessor::TMX_POS_ATTR     	= L"pos";
wstring const TMXPreprocessor::TMX_SEGTYPE_ATTR 	= L"segtype";
wstring const TMXPreprocessor::TMX_SRCLANG_ATTR      	= L"srclang";
wstring const TMXPreprocessor::TMX_SUBST_ATTR	    	= L"subst";
wstring const TMXPreprocessor::TMX_TUID_ATTR	    	= L"tuid";
wstring const TMXPreprocessor::TMX_TYPE_ATTR 		= L"type";
wstring const TMXPreprocessor::TMX_UNICODE_ATTR         = L"unicode";
wstring const TMXPreprocessor::TMX_USAGECOUNT_ATTR      = L"usagecount";
wstring const TMXPreprocessor::TMX_VERSION_ATTR        	= L"version";
wstring const TMXPreprocessor::TMX_X_ATTR    		= L"x";
wstring const TMXPreprocessor::TMX_XML_LANG_ATTR    	= L"xml:lang";

TMXPreprocessor::TMXPreprocessor()
{
  LtLocale::tryToSetLocale();
}

TMXPreprocessor::~TMXPreprocessor()
{
}

void
TMXPreprocessor::parse(string const &filename, wstring const &dir)
{
  reader = xmlReaderForFile(filename.c_str(), NULL, 0);
  if(reader == NULL)
  {
    wcerr << L"Error: Cannot open '";
    cerr << filename;
    wcerr << L"'." << endl;
    exit(EXIT_FAILURE);
  }

  int ret = xmlTextReaderRead(reader);
  while(ret == 1)
  {
    procNode();
    ret = xmlTextReaderRead(reader);
  }

  if(ret != 0)
  {
    cerr << "Error: Parse error at the end of input." << endl;
  }

  xmlFreeTextReader(reader);
  xmlCleanupParser();
}

void
TMXPreprocessor::requireEmptyError(wstring const &name)
{
  if(!xmlTextReaderIsEmptyElement(reader))
  {
    wcerr << L"Error (" << xmlTextReaderGetParserLineNumber(reader);
    wcerr << L"): Non-empty element '<" << name << L">' should be empty." << endl;
    exit(EXIT_FAILURE);
  }
}

bool 
TMXPreprocessor::allBlanks()
{
  bool flag = true;
  wstring text = XMLParseUtil::towstring(xmlTextReaderConstValue(reader));
  
  for(unsigned int i = 0, limit = text.size(); i < limit; i++)
  {
    flag = flag && iswspace(text[i]);
  }
  
  return flag;
}

void
TMXPreprocessor::skipBlanks(wstring &name)
{
  while(name == L"#text" || name == L"#comment")
  {
    if(name != L"#comment")
    {
      if(!allBlanks())
      {
        cerr << "Error (" << xmlTextReaderGetParserLineNumber(reader); 
        cerr << "): Invalid construction." << endl;
        exit(EXIT_FAILURE);
      }
    }
    
    xmlTextReaderRead(reader);
    name = XMLParseUtil::towstring(xmlTextReaderConstName(reader));
  }
}

void
TMXPreprocessor::skip(wstring &name, wstring const &elem)
{
  xmlTextReaderRead(reader);
  name = XMLParseUtil::towstring(xmlTextReaderConstName(reader));
  
  while(name == L"#text" || name == L"#comment")
  {
    if(name != L"#comment")
    {
      if(!allBlanks())
      {
        cerr << "Error (" << xmlTextReaderGetParserLineNumber(reader);
        cerr << "): Invalid construction." << endl;
        exit(EXIT_FAILURE);
      }
    }
    xmlTextReaderRead(reader);
    name = XMLParseUtil::towstring(xmlTextReaderConstName(reader));  
  }    
    
  if(name != elem)
  {
    wcerr << L"Error (" << xmlTextReaderGetParserLineNumber(reader);
    wcerr << L"): Expected '<" << elem << L">'." << endl;
    exit(EXIT_FAILURE);
  }  
}

wstring
TMXPreprocessor::attrib(wstring const &name)
{
  return XMLParseUtil::attrib(reader, name);
} 

void
TMXPreprocessor::requireAttribute(wstring const &value, wstring const &attrname,
                           wstring const &elemname)
{
  if(value == L"")
  {
    wcerr << L"Error (" << xmlTextReaderGetParserLineNumber(reader);  
    wcerr << L"): '<" << elemname;
    wcerr << L"' element must specify non-void '";
    wcerr << attrname << L"' attribute." << endl;
    exit(EXIT_FAILURE);
  }  
}

void
TMXPreprocessor::procNode()
{
  xmlChar const *xnombre = xmlTextReaderConstName(reader);
  wstring nombre = XMLParseUtil::towstring(xnombre);

  wcout << nombre << endl;


  if(nombre == TMX_TMX_ELEM)
  {
    wcout << nombre << endl;
  }
  else
  {
    wcerr << L"Error (" << xmlTextReaderGetParserLineNumber(reader);
    wcerr << L"): Invalid node '<" << nombre << L">'." << endl;
    exit(EXIT_FAILURE);
  }  
}

int main(int argc, char *argv[])
{
  TMXPreprocessor tmxp;
  if(argc == 3)
  {
    tmxp.parse(argv[1], L"LR");
  }
  return 0;
}

