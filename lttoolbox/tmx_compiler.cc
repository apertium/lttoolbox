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
#include <lttoolbox/tmx_compiler.h>
#include <lttoolbox/compression.h>
#include <lttoolbox/entry_token.h>
#include <lttoolbox/lt_locale.h>
#include <lttoolbox/xml_parse_util.h>

#include <cstdlib>
#include <iostream>
#include <libxml/encoding.h>

using namespace std;

UString const TMXCompiler::TMX_COMPILER_TMX_ELEM     = "tmx"_u;
UString const TMXCompiler::TMX_COMPILER_HEADER_ELEM  = "header"_u;
UString const TMXCompiler::TMX_COMPILER_BODY_ELEM    = "body"_u;
UString const TMXCompiler::TMX_COMPILER_TU_ELEM      = "tu"_u;
UString const TMXCompiler::TMX_COMPILER_TUV_ELEM     = "tuv"_u;
UString const TMXCompiler::TMX_COMPILER_HI_ELEM      = "hi"_u;
UString const TMXCompiler::TMX_COMPILER_PH_ELEM      = "ph"_u;
UString const TMXCompiler::TMX_COMPILER_XMLLANG_ATTR = "xml:lang"_u;
UString const TMXCompiler::TMX_COMPILER_LANG_ATTR    = "lang"_u;
UString const TMXCompiler::TMX_COMPILER_SEG_ELEM     = "seg"_u;
UString const TMXCompiler::TMX_COMPILER_PROP_ELEM    = "prop"_u;
UString const TMXCompiler::TMX_COMPILER_TEXT_NODE    = "#text"_u;
UString const TMXCompiler::TMX_COMPILER_COMMENT_NODE = "#comment"_u;
UString const TMXCompiler::TMX_COMPILER_NUMBER_TAG   = "<n>"_u;
UString const TMXCompiler::TMX_COMPILER_BLANK_TAG    = "<b>"_u;

TMXCompiler::TMXCompiler() :
reader(0),
default_weight(0.0000)
{
  LtLocale::tryToSetLocale();
  alphabet.includeSymbol(TMX_COMPILER_NUMBER_TAG); // -1 -> numbers
  alphabet.includeSymbol(TMX_COMPILER_BLANK_TAG); // -2 -> blanks
  number_tag = alphabet(TMX_COMPILER_NUMBER_TAG);
  blank_tag = alphabet(TMX_COMPILER_BLANK_TAG);
}

TMXCompiler::~TMXCompiler()
{
}

void
TMXCompiler::parse(string const &file, UString const &lo, UString const &lm)
{
  origin_language = lo;
  meta_language = lm;
  reader = xmlReaderForFile(file.c_str(), NULL, 0);
  if(reader == NULL)
  {
    cerr << "Error: Cannot open '" << file << "'." << endl;
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

  // Minimize transducer
  transducer.minimize();
}

void
TMXCompiler::requireEmptyError(UString const &name)
{
  if(!xmlTextReaderIsEmptyElement(reader))
  {
    cerr << "Error (" << xmlTextReaderGetParserLineNumber(reader);
    cerr << "): Non-empty element '<" << name << ">' should be empty." << endl;
    exit(EXIT_FAILURE);
  }
}

bool
TMXCompiler::allBlanks()
{
  UString text = XMLParseUtil::readValue(reader);

  for(auto c : text)
  {
    if (!u_isspace(c)) {
      return false;
    }
  }
  return true;
}

void
TMXCompiler::skipBlanks(UString &name)
{
  while(name == TMX_COMPILER_TEXT_NODE || name == TMX_COMPILER_COMMENT_NODE)
  {
    if(name != TMX_COMPILER_COMMENT_NODE)
    {
      if(!allBlanks())
      {
        cerr << "Error (" << xmlTextReaderGetParserLineNumber(reader);
        cerr << "): Invalid construction." << endl;
        exit(EXIT_FAILURE);
      }
    }

    xmlTextReaderRead(reader);
    name = XMLParseUtil::readName(reader);
  }
}

void
TMXCompiler::skip(UString &name, UString const &elem)
{
  xmlTextReaderRead(reader);
  name = XMLParseUtil::readName(reader);

  while(name == TMX_COMPILER_TEXT_NODE || name == TMX_COMPILER_COMMENT_NODE)
  {
    if(name != TMX_COMPILER_COMMENT_NODE)
    {
      if(!allBlanks())
      {
        cerr << "Error (" << xmlTextReaderGetParserLineNumber(reader);
        cerr << "): Invalid construction." << endl;
        exit(EXIT_FAILURE);
      }
    }
    xmlTextReaderRead(reader);
    name = XMLParseUtil::readName(reader);
  }

  if(name != elem)
  {
    cerr << "Error (" << xmlTextReaderGetParserLineNumber(reader);
    cerr << "): Expected '<" << elem << ">'." << endl;
    exit(EXIT_FAILURE);
  }
}

UString
TMXCompiler::attrib(UString const &name)
{
  return XMLParseUtil::attrib(reader, name);
}

void
TMXCompiler::requireAttribute(UString const &value, UString const &attrname,
                           UString const &elemname)
{
  if(value.empty())
  {
    cerr << "Error (" << xmlTextReaderGetParserLineNumber(reader);
    cerr << "): '<" << elemname;
    cerr << "' element must specify non-void '";
    cerr << attrname << "' attribute." << endl;
    exit(EXIT_FAILURE);
  }
}

UString
TMXCompiler::getTag(size_t const &val) const
{
  UChar cad[32];
  u_snprintf(cad, 32, "<%d>", val);
  return cad;
}

void
TMXCompiler::insertTU(vector<int> const &origin, vector<int> const &meta)
{
  if(origin.size() < 5 || meta.size() < 5)
  {
    return;
  }

  if(origin[0] == blank_tag || meta[0] == blank_tag)
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
      source = transducer.insertSingleTransduction(alphabet(s1, s2), source, default_weight);
    }
    transducer.setFinal(source, default_weight);
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

  vector<vector<int> > so, sm;

  split(o, so, blank_tag);
  split(m, sm, blank_tag);

  if(so.size() == sm.size())
  {
    for(unsigned int i = 0, limit = sm.size(); i != limit; i++)
    {
      trim(so[i]);
      trim(sm[i]);
      if(sm.size() - 1 != i)
      {
        sm[i].push_back('(');
        sm[i].push_back('#');
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
    o = join(so, ' ');
    m = join(sm, ')');
  }
  else
  {
    for(auto& s : so)
    {
      trim(s);
    }
    for(unsigned int i = 0, limit = sm.size(); i != limit; i++)
    {
      trim(sm[i]);
      if(sm.size() - 1 != i)
      {
        sm[i].push_back('(');
        sm[i].push_back('#');
      }
    }
    o = join(so, ' ');
    m = join(sm, ')');
  }
}

void
TMXCompiler::procTU()
{
  UString name = XMLParseUtil::readName(reader);
  int type = xmlTextReaderNodeType(reader);
  vector<int> origin;
  vector<int> meta;
  vector<int> foo;

  while(name != TMX_COMPILER_TU_ELEM || type != XML_READER_TYPE_END_ELEMENT)
  {
    if(name == TMX_COMPILER_TUV_ELEM && type != XML_READER_TYPE_END_ELEMENT)
    {
      UString l = attrib(TMX_COMPILER_XMLLANG_ATTR);
      if(l.empty()) {
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
        name = XMLParseUtil::readName(reader);
        type = xmlTextReaderNodeType(reader);

        if(name == TMX_COMPILER_TEXT_NODE)
        {
          XMLParseUtil::readValueInto32(reader, *ref);
        }
        else if(name == TMX_COMPILER_HI_ELEM || name == TMX_COMPILER_PH_ELEM)
        {
          if(type != XML_READER_TYPE_END_ELEMENT)
          {
            ref->push_back(blank_tag);
          }
        }
      }
    }
    xmlTextReaderRead(reader);
    name = XMLParseUtil::readName(reader);
    type = xmlTextReaderNodeType(reader);
  }

  trim(origin);
  trim(meta);

  align(origin, meta);
  align_blanks(origin, meta);
  insertTU(origin, meta);
}

void
TMXCompiler::procNode()
{
  UString name = XMLParseUtil::readName(reader);

  // HACER: optimizar el orden de ejecución de esta ristra de "ifs"

  if(name == TMX_COMPILER_TEXT_NODE)
  {
    /* ignorar */
  }
  else if(name == TMX_COMPILER_TMX_ELEM)
  {
    /* ignorar */
  }
  else if(name == TMX_COMPILER_HEADER_ELEM)
  {
    /* ignorar */
  }
  else if(name == TMX_COMPILER_BODY_ELEM)
  {
    /* ignorar */
  }
  else if(name == TMX_COMPILER_PROP_ELEM)
  {
    /* ignorar */
  }
  else if(name == TMX_COMPILER_TU_ELEM)
  {
    procTU();
  }
  else if(name== TMX_COMPILER_COMMENT_NODE)
  {
    /* ignorar */
  }
  else
  {
    cerr << "Error (" << xmlTextReaderGetParserLineNumber(reader);
    cerr << "): Invalid node '<" << name << ">'." << endl;
    exit(EXIT_FAILURE);
  }
}

void
TMXCompiler::write(FILE *output)
{
  fwrite_unlocked(HEADER_LTTOOLBOX, 1, 4, output);
  uint64_t features = 0;
  write_le(output, features);

  // letters (empty to keep the file format)
  Compression::multibyte_write(0, output);

  // symbols
  alphabet.write(output);

  // transducers (1, with empty name)
  Compression::multibyte_write(1, output); // keeping file format
  Compression::multibyte_write(0, output); // keeping file format
  transducer.write(output);

  cout << origin_language << "->" << meta_language << " ";
  cout << transducer.size() << " " << transducer.numberOfTransitions();
  cout << endl;
}

void
TMXCompiler::trim(vector<int> &v) const
{
  while(v.size() > 0)
  {
    if(u_isspace(v[v.size()-1]))
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
  for(auto c : v)
  {
    if(!u_isspace(c) || !principio)
    {
      principio = false;
      aux.push_back(c);
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
      modified_origin.push_back(number_tag);
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
            modified_meta.push_back('@');
            modified_meta.push_back('(');
            UChar valor[8]{};
            int limit3 = u_snprintf(valor, 8, "%d", j+1);
            for(int k = 0; k != limit3; k++)
            {
              modified_meta.push_back(valor[k]);
            }
            modified_meta.push_back(')');
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
    if(!u_isdigit(v[i]) && (v[i] != '.' || i == position) && (v[i] != ',' || i == position))
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
          if(u_isdigit(v[i]))
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
    if(u_isdigit(v[i]))
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
TMXCompiler::setOriginLanguageCode(UString const &code)
{
  // nada
}

void
TMXCompiler::setMetaLanguageCode(UString const &code)
{
  // nada
}
