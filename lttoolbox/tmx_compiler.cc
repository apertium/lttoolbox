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
#include <lttoolbox/tmx_compiler.h>
#include <lttoolbox/compression.h>
#include <lttoolbox/entry_token.h>
#include <lttoolbox/lt_locale.h>
#include <lttoolbox/xml_parse_util.h>
#include <lttoolbox/string_to_wostream.h>

#include <cstdlib>
#include <iostream>
#include <libxml/encoding.h>

#ifdef _WIN32
#define swprintf _snwprintf
#endif

using namespace std;

wstring const TMXCompiler::TMX_COMPILER_TMX_ELEM     = L"tmx";
wstring const TMXCompiler::TMX_COMPILER_HEADER_ELEM  = L"header"; 
wstring const TMXCompiler::TMX_COMPILER_BODY_ELEM    = L"body";
wstring const TMXCompiler::TMX_COMPILER_TU_ELEM      = L"tu";
wstring const TMXCompiler::TMX_COMPILER_TUV_ELEM     = L"tuv";
wstring const TMXCompiler::TMX_COMPILER_HI_ELEM      = L"hi";
wstring const TMXCompiler::TMX_COMPILER_PH_ELEM      = L"ph";
wstring const TMXCompiler::TMX_COMPILER_XMLLANG_ATTR = L"xml:lang";
wstring const TMXCompiler::TMX_COMPILER_LANG_ATTR    = L"lang";
wstring const TMXCompiler::TMX_COMPILER_SEG_ELEM     = L"seg";
wstring const TMXCompiler::TMX_COMPILER_PROP_ELEM    = L"prop";

TMXCompiler::TMXCompiler() :
reader(0)
{
  LtLocale::tryToSetLocale();
  alphabet.includeSymbol(L"<n>"); // -1 -> numbers
  alphabet.includeSymbol(L"<b>"); // -2 -> blanks
}

TMXCompiler::~TMXCompiler()
{
}

void
TMXCompiler::parse(string const &fichero, wstring const &lo, wstring const &lm)
{
  origin_language = lo;
  meta_language = lm;
  reader = xmlReaderForFile(fichero.c_str(), NULL, 0);
  if(reader == NULL)
  {
    wcerr << "Error: Cannot open '" << fichero << "'." << endl;
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
    wcerr << L"Error: Parse error at the end of input." << endl;
  }

  xmlFreeTextReader(reader);
  xmlCleanupParser();

  // Minimize transducer
  transducer.minimize();
}

void
TMXCompiler::requireEmptyError(wstring const &name)
{
  if(!xmlTextReaderIsEmptyElement(reader))
  {
    wcerr << L"Error (" << xmlTextReaderGetParserLineNumber(reader);
    wcerr << L"): Non-empty element '<" << name << L">' should be empty." << endl;
    exit(EXIT_FAILURE);
  }
}

bool 
TMXCompiler::allBlanks()
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
TMXCompiler::skipBlanks(wstring &name)
{
  while(name == L"#text" || name == L"#comment")
  {
    if(name != L"#comment")
    {
      if(!allBlanks())
      {
        wcerr << "Error (" << xmlTextReaderGetParserLineNumber(reader); 
        wcerr << "): Invalid construction." << endl;
        exit(EXIT_FAILURE);
      }
    }
    
    xmlTextReaderRead(reader);
    name = XMLParseUtil::towstring(xmlTextReaderConstName(reader));
  }
}

void
TMXCompiler::skip(wstring &name, wstring const &elem)
{
  xmlTextReaderRead(reader);
  name = XMLParseUtil::towstring(xmlTextReaderConstName(reader));
  
  while(name == L"#text" || name == L"#comment")
  {
    if(name != L"#comment")
    {
      if(!allBlanks())
      {
        wcerr << "Error (" << xmlTextReaderGetParserLineNumber(reader);
        wcerr << "): Invalid construction." << endl;
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
TMXCompiler::attrib(wstring const &name)
{
  return XMLParseUtil::attrib(reader, name);
} 

void
TMXCompiler::requireAttribute(wstring const &value, wstring const &attrname,
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

wstring
TMXCompiler::getTag(size_t const &val) const
{
  wchar_t cad[32];
  swprintf(cad, 32, L"<%d>", val);
  return cad;
}

void
TMXCompiler::insertTU(vector<int> const &origin, vector<int> const &meta)
{
  if(origin.size() < 5 || meta.size() < 5)
  {
    return;
  }
  
  if(origin[0] == alphabet(L"<b>") || meta[0] == alphabet(L"<b>"))
  {
    return;
  }
  
  if(origin.size() != 0 && meta.size() != 0)
  {
    int source = transducer.getInitial();
    for(size_t i = 0 ;; i++)
    {
      int s1 = 0, s2 = 0;
      if(origin.size() > i)
      {
        s1 = origin[i];
      }
      if(meta.size() > i)
      {
        s2 = meta[i];
      }
      if(s1 == 0 && s2 == 0)
      {
        break;
      }
      source = transducer.insertSingleTransduction(alphabet(s1, s2), source);
    }
    transducer.setFinal(source);
  }
}

void
TMXCompiler::split(vector<int> const &v, vector<vector<int> > &sv, int const symbol) const
{
  sv.clear();
  
  for(unsigned int i = 0, limit = v.size(), j = 0; i != limit; i++)
  {
    if(sv.size() == j)
    {
      sv.push_back(vector<int>());
    }
    if(v[i] == symbol)
    {
      j++;
    }
    else
    {
      sv[j].push_back(v[i]);
    }
  }
}

vector<int>
TMXCompiler::join(vector<vector<int> > const &v, int const s) const
{
  vector<int> result;
  for(unsigned int i = 0, limit = v.size(); i != limit; i++)
  {
    for(unsigned int j = 0, limit2 = v[i].size(); j != limit2; j++)
    {
      result.push_back(v[i][j]);
    }
    if(i != limit - 1)
    {
      result.push_back(s);  
    }
  }
  
  return result;
}

void
TMXCompiler::align_blanks(vector<int> &o, vector<int> &m)
{
  vector<unsigned int> puntos;
  vector<int> resultado_o, resultado_m;
  
  int const symbol = alphabet(L"<b>");
  
  vector<vector<int> > so, sm;
  
  split(o, so, symbol);
  split(m, sm, symbol);
  
  if(so.size() == sm.size())
  {
    for(unsigned int i = 0, limit = sm.size(); i != limit; i++)
    {
      trim(so[i]);
      trim(sm[i]);
      if(sm.size() - 1 != i)
      {
        sm[i].push_back(L'(');
        sm[i].push_back(L'#');
      }
    /*
      while(so[i].size() < sm[i].size())
      {
        so[i].push_back(0);
      }
      while(so[i].size() > sm[i].size())
      {
        sm[i].push_back(0);
      }*/
    } 
    o = join(so, L' ');
    m = join(sm, L')');
  }
  else
  {
    for(unsigned int i = 0, limit = so.size(); i != limit; i++)
    {
      trim(so[i]);
    }
    for(unsigned int i = 0, limit = sm.size(); i != limit; i++)
    {
      trim(sm[i]);
      if(sm.size() - 1 != i)
      {
        sm[i].push_back(L'(');
        sm[i].push_back(L'#');
      }
    }
    o = join(so, L' ');
    m = join(sm, L')');
  }
}

void
TMXCompiler::procTU()
{
  wstring name = XMLParseUtil::towstring(xmlTextReaderConstName(reader));
  int type = xmlTextReaderNodeType(reader);
  vector<int> origin;
  vector<int> meta;
  vector<int> foo;
  
  while(name != TMX_COMPILER_TU_ELEM || type != XML_READER_TYPE_END_ELEMENT)
  {
    if(name == TMX_COMPILER_TUV_ELEM && type != XML_READER_TYPE_END_ELEMENT)
    {
      wstring l = attrib(TMX_COMPILER_XMLLANG_ATTR);
      if(l == L"")
      {
        l = attrib(TMX_COMPILER_LANG_ATTR);
      }
      
      vector<int> *ref;
      if(l == meta_language)
      {
        ref = &meta;
      }
      else if(l == origin_language)
      {
        ref = &origin;
      }   
      else
      {
        ref = &foo;
      }
      
      while(name != TMX_COMPILER_TUV_ELEM || type != XML_READER_TYPE_END_ELEMENT)
      {
        xmlTextReaderRead(reader);
        name = XMLParseUtil::towstring(xmlTextReaderConstName(reader));
        type = xmlTextReaderNodeType(reader);
        
        if(name == L"#text")
        {
          wstring l = XMLParseUtil::towstring(xmlTextReaderConstValue(reader));
          for(size_t i = 0, limit = l.size(); i != limit; i++)
          {
            ref->push_back(l[i]);
          }
        }
        else if(name == TMX_COMPILER_HI_ELEM || name == TMX_COMPILER_PH_ELEM)
        {
          if(type != XML_READER_TYPE_END_ELEMENT)
          {
            ref->push_back(alphabet(L"<b>"));
          }
        }
      }
    }
    xmlTextReaderRead(reader);
    name = XMLParseUtil::towstring(xmlTextReaderConstName(reader));
    type = xmlTextReaderNodeType(reader);
  }
  
  trim(origin);
  trim(meta);
//  wcout << L"DESPUES DE TRIM\n";
//  printvector(origin);
//  printvector(meta);

  align(origin, meta);
//  wcout << L"DESPUES DE ALIGN\n";
//  printvector(origin);
//  printvector(meta);
  align_blanks(origin, meta);
//  wcout << L"DESPUES DE ALIGNBLANKS\n";
//  printvector(origin);
//  printvector(meta);
  insertTU(origin, meta);
}

void
TMXCompiler::procNode()
{
  xmlChar const *xnombre = xmlTextReaderConstName(reader);
  wstring nombre = XMLParseUtil::towstring(xnombre);

  // HACER: optimizar el orden de ejecución de esta ristra de "ifs"

  if(nombre == L"#text")
  {
    /* ignorar */
  }
  else if(nombre == TMX_COMPILER_TMX_ELEM)
  {
    /* ignorar */
  }
  else if(nombre == TMX_COMPILER_HEADER_ELEM)
  {
    /* ignorar */
  }
  else if(nombre == TMX_COMPILER_BODY_ELEM)
  {
    /* ignorar */
  }
  else if(nombre == TMX_COMPILER_PROP_ELEM)
  {
    /* ignorar */
  }
  else if(nombre == TMX_COMPILER_TU_ELEM)
  {
    procTU();
  }
  else if(nombre== L"#comment")
  {
    /* ignorar */
  }
  else
  {
    wcerr << L"Error (" << xmlTextReaderGetParserLineNumber(reader);
    wcerr << L"): Invalid node '<" << nombre << L">'." << endl;
    exit(EXIT_FAILURE);
  }
}

void 
TMXCompiler::write(FILE *output)
{
  // letters (empty to keep the file format)
  Compression::wstring_write(L"", output);
  
  // symbols
  alphabet.write(output);
  
  // transducers
  Compression::multibyte_write(1, output); // keeping file format
  Compression::wstring_write(L"", output); // keeping file format
  transducer.write(output);

  wcout << origin_language << L"->" << meta_language << L" ";
  wcout << transducer.size() << L" " << transducer.numberOfTransitions();
  wcout << endl;
}

void
TMXCompiler::trim(vector<int> &v) const
{
  while(v.size() > 0)
  {
    if(iswspace(v[v.size()-1]))
    {
      v.pop_back();
    }
    else
    {
      break;
    }
  }
  
  bool principio = true;
  vector<int> aux;
  for(unsigned int i = 0, limit = v.size(); i < limit; i++)
  {
    if(!iswspace(v[i]) || !principio)
    {
      principio = false;
      aux.push_back(v[i]);
    }      
  }
  
  v = aux;
}

void
TMXCompiler::align(vector<int> &origin, vector<int> &meta)
{
  vector<unsigned int> numbers_origin_start,
                       numbers_origin_length;
  vector<int> modified_origin, modified_meta;

  // compile information from origin
  for(unsigned int i = 0, limit = origin.size(); i != limit; i++)
  {
    int nl = numberLength(origin, i);
    if(nl != 0)
    {
      numbers_origin_start.push_back(i);
      numbers_origin_length.push_back(nl);
      i += nl-1;
      modified_origin.push_back(alphabet(L"<n>"));
    }
    else
    {
      modified_origin.push_back(origin[i]);
    }  
  }
    
  // compile information from meta
  for(int i = 0, limit = meta.size(); i != limit; i++)
  {
    unsigned int nl = numberLength(meta, i);
    if(nl != 0)
    {
      bool tocado = false;
      for(int j = 0, limit2 = numbers_origin_start.size(); j != limit2; j++)
      {
        if(nl == numbers_origin_length[j])
        {
          if(vectorcmp(origin, numbers_origin_start[j],
                       meta, i, nl))
          {
            modified_meta.push_back(L'@');
            modified_meta.push_back(L'(');
            wchar_t *valor = new wchar_t[8];
            swprintf(valor, 8, L"%d", j+1);
            for(int k = 0, limit3 = wcslen(valor); k != limit3; k++)
            {
              modified_meta.push_back(valor[k]);
            }
            delete[] valor;
            modified_meta.push_back(L')');
            i += nl-1;
            tocado = true;
            break;
          }
        }
      }
      
      if(!tocado)
      {
        if((unsigned int) i >= nl)
        {
          return;
        }          

        for(unsigned int j = i; j < nl; i++, j++)
        {
          modified_meta.push_back(meta[i]);
        }
        i--;
      }
    }  
    else
    {
      modified_meta.push_back(meta[i]);
    }
  }
  
  origin = modified_origin;
  meta = modified_meta;  
}

unsigned int
TMXCompiler::numberLength(vector<int> &v, unsigned int const position) const
{
  for(unsigned int i = position, limit = v.size(); i < limit; i++)
  {
    if(!iswdigit(v[i]) && (v[i] != L'.' || i == position) && (v[i] != L',' || i == position))
    {
      if(i == position)
      {
        return 0;
      }
      else
      {   
        while(i != position)
        {
          i--;
          if(iswdigit(v[i]))
          {
            return i - position + 1;
          }
        }  
      }    
    }
  }

  unsigned int i = v.size();
  
  while(i != position)
  {
    i--;
    if(iswdigit(v[i]))
    {
      return i - position + 1;
    }
  }
  
  return 0;
}

bool
TMXCompiler::vectorcmp(vector<int> const &orig, unsigned int const begin_orig,
                       vector<int> const &meta, unsigned int const begin_meta,
                       unsigned const int length) const
{
  for(unsigned int i = begin_orig, j = begin_meta, count = 0; count != length;
      i++, j++, count++)
  {
    if(orig[i] != meta[j])
    {
      return false;
    }
  }

  return true;
}

void 
TMXCompiler::printvector(vector<int> const &v, wostream &os)
{
  for(unsigned int i = 0, limit = v.size(); i != limit; i++)
  {
    if(i != 0)
    {
      os << L" ";
    }
    if(v[i] > 31)
    {
      os << v[i] << L" ('" << wchar_t(v[i]) << L"')";
    }
    else
    {
      os << v[i];
    }      
  }
  os << endl;
}

void 
TMXCompiler::setOriginLanguageCode(wstring const &code)
{
  // nada
}

void 
TMXCompiler::setMetaLanguageCode(wstring const &code)
{
  // nada
}

