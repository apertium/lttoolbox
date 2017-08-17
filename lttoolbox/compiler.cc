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
#include <lttoolbox/compiler.h>
#include <lttoolbox/compression.h>
#include <lttoolbox/entry_token.h>
#include <lttoolbox/lt_locale.h>
#include <lttoolbox/xml_parse_util.h>
#include <lttoolbox/string_to_wostream.h>

#include <cstdlib>
#include <iostream>
#include <libxml/encoding.h>

using namespace std;

wstring const Compiler::COMPILER_DICTIONARY_ELEM    = L"dictionary";
wstring const Compiler::COMPILER_ALPHABET_ELEM      = L"alphabet";
wstring const Compiler::COMPILER_SDEFS_ELEM         = L"sdefs";
wstring const Compiler::COMPILER_SDEF_ELEM          = L"sdef";
wstring const Compiler::COMPILER_N_ATTR             = L"n";
wstring const Compiler::COMPILER_PARDEFS_ELEM       = L"pardefs";
wstring const Compiler::COMPILER_PARDEF_ELEM        = L"pardef";
wstring const Compiler::COMPILER_PAR_ELEM           = L"par";
wstring const Compiler::COMPILER_ENTRY_ELEM         = L"e";
wstring const Compiler::COMPILER_RESTRICTION_ATTR   = L"r";
wstring const Compiler::COMPILER_RESTRICTION_LR_VAL = L"LR";
wstring const Compiler::COMPILER_RESTRICTION_RL_VAL = L"RL";
wstring const Compiler::COMPILER_PAIR_ELEM          = L"p";
wstring const Compiler::COMPILER_LEFT_ELEM          = L"l";
wstring const Compiler::COMPILER_RIGHT_ELEM         = L"r";
wstring const Compiler::COMPILER_S_ELEM             = L"s";
wstring const Compiler::COMPILER_REGEXP_ELEM        = L"re";
wstring const Compiler::COMPILER_SECTION_ELEM       = L"section";
wstring const Compiler::COMPILER_ID_ATTR            = L"id";
wstring const Compiler::COMPILER_TYPE_ATTR	    = L"type";
wstring const Compiler::COMPILER_IDENTITY_ELEM      = L"i";
wstring const Compiler::COMPILER_IDENTITYGROUP_ELEM = L"ig";
wstring const Compiler::COMPILER_JOIN_ELEM	    = L"j";
wstring const Compiler::COMPILER_BLANK_ELEM	    = L"b";
wstring const Compiler::COMPILER_POSTGENERATOR_ELEM = L"a";
wstring const Compiler::COMPILER_GROUP_ELEM         = L"g";
wstring const Compiler::COMPILER_LEMMA_ATTR         = L"lm";
wstring const Compiler::COMPILER_IGNORE_ATTR        = L"i";
wstring const Compiler::COMPILER_IGNORE_YES_VAL     = L"yes";
wstring const Compiler::COMPILER_ALT_ATTR           = L"alt";
wstring const Compiler::COMPILER_V_ATTR             = L"v";
wstring const Compiler::COMPILER_VL_ATTR            = L"vl";
wstring const Compiler::COMPILER_VR_ATTR            = L"vr";

Compiler::Compiler() :
reader(0),
verbose(false),
first_element(false),
acx_current_char(0)
{
}

Compiler::~Compiler()
{
}

void
Compiler::parseACX(string const &fichero, wstring const &dir)
{
  if(dir == COMPILER_RESTRICTION_LR_VAL)
  {
    reader = xmlReaderForFile(fichero.c_str(), NULL, 0);
    if(reader == NULL)
    {
      wcerr << "Error: cannot open '" << fichero << "'." << endl;
      exit(EXIT_FAILURE);
    }
    int ret = xmlTextReaderRead(reader);
    while(ret == 1)
    {
      procNodeACX();
      ret = xmlTextReaderRead(reader);
    }
  }
}

void
Compiler::parse(string const &fichero, wstring const &dir)
{
  direction = dir;
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

 
  // Minimize transducers 
  for(map<wstring, Transducer, Ltstr>::iterator it = sections.begin(),
                                               limit = sections.end(); 
      it != limit; it++)
  {
    (it->second).minimize();
  }
}


void
Compiler::procAlphabet()
{
  int tipo=xmlTextReaderNodeType(reader);

  if(tipo != XML_READER_TYPE_END_ELEMENT)
  {
    int ret = xmlTextReaderRead(reader);
    if(ret == 1)
    {          
      xmlChar const *valor = xmlTextReaderConstValue(reader);
      letters = XMLParseUtil::towstring(valor);
      bool espai = true;
      for(unsigned int i = 0; i < letters.length(); i++)  
      {
        if(!isspace(letters.at(i))) 
        {
          espai = false;
          break;
        }
      }
      if(espai == true)  // libxml2 returns '\n' for <alphabet></alphabet>, should be empty
      {
        letters = L"";
      } 
    }
    else
    {
      wcerr << L"Error (" << xmlTextReaderGetParserLineNumber(reader);
      wcerr << L"): Missing alphabet symbols." << endl;
      exit(EXIT_FAILURE);
    }
  }
}

void
Compiler::procSDef()
{
  alphabet.includeSymbol(L"<"+attrib(COMPILER_N_ATTR)+L">");
}

void
Compiler::procParDef()
{
  int tipo=xmlTextReaderNodeType(reader);

  if(tipo != XML_READER_TYPE_END_ELEMENT)
  {
    current_paradigm = attrib(COMPILER_N_ATTR);
  }
  else
  {
    if(!paradigms[current_paradigm].isEmpty())
    {
      paradigms[current_paradigm].minimize();
      paradigms[current_paradigm].joinFinals();
      current_paradigm = L"";
    }
  }
}

int
Compiler::matchTransduction(list<int> const &pi, 
				 list<int> const &pd, 
				 int estado, Transducer &t)
{
  list<int>::const_iterator izqda, dcha, limizqda, limdcha;

  if(direction == COMPILER_RESTRICTION_LR_VAL)
  {
    izqda = pi.begin();
    dcha = pd.begin();
    limizqda = pi.end();
    limdcha = pd.end();
  }
  else
  {
    izqda = pd.begin();
    dcha = pi.begin();
    limizqda = pd.end();
    limdcha = pi.end();
  }
 

  if(pi.size() == 0 && pd.size() == 0)
  {
    estado = t.insertNewSingleTransduction(alphabet(0, 0), estado);
  }
  else
  {
    map<int, set<int> >::iterator acx_map_ptr;
    int rsymbol = 0;

    while(true)
    {
      int etiqueta;
      
      acx_map_ptr = acx_map.end();
      
      if(izqda == limizqda && dcha == limdcha)
      {
        break;
      }
      else if(izqda == limizqda)
      {
        etiqueta = alphabet(0, *dcha);
        dcha++;
      }
      else if(dcha == limdcha)
      {
        etiqueta = alphabet(*izqda, 0);
        acx_map_ptr = acx_map.find(*izqda);
        rsymbol = 0;
        izqda++;
      }
      else
      {
        etiqueta = alphabet(*izqda, *dcha);
        acx_map_ptr = acx_map.find(*izqda);
        rsymbol = *dcha;
        izqda++;
        dcha++;
      }

      int nuevo_estado = t.insertSingleTransduction(etiqueta, estado);
      
      if(acx_map_ptr != acx_map.end())
      {
        for(set<int>::iterator it = acx_map_ptr->second.begin(); 
            it != acx_map_ptr->second.end(); it++)
        { 
          t.linkStates(estado, nuevo_estado, alphabet(*it ,rsymbol));
        }
      }
      estado = nuevo_estado;
    }
  }

  return estado;
}


void
Compiler::requireEmptyError(wstring const &name)
{
  if(!xmlTextReaderIsEmptyElement(reader))
  {
    wcerr << L"Error (" << xmlTextReaderGetParserLineNumber(reader);
    wcerr << L"): Non-empty element '<" << name << L">' should be empty." << endl;
    exit(EXIT_FAILURE);
  }
}

bool 
Compiler::allBlanks()
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
Compiler::readString(list<int> &result, wstring const &name)
{
  if(name == L"#text")
  {
    wstring value = XMLParseUtil::towstring(xmlTextReaderConstValue(reader));
    for(unsigned int i = 0, limit = value.size(); i < limit; i++)
    {
      result.push_back(static_cast<int>(value[i]));
    }
  }
  else if(name == COMPILER_BLANK_ELEM)
  {
    requireEmptyError(name);
    result.push_back(static_cast<int>(L' '));
  }
  else if(name == COMPILER_JOIN_ELEM)
  {
    requireEmptyError(name);
    result.push_back(static_cast<int>(L'+'));
  }
  else if(name == COMPILER_POSTGENERATOR_ELEM)
  {
    requireEmptyError(name);
    result.push_back(static_cast<int>(L'~'));
  }
  else if(name == COMPILER_GROUP_ELEM)
  {
    int tipo=xmlTextReaderNodeType(reader);
    if(tipo != XML_READER_TYPE_END_ELEMENT)
    {
      result.push_back(static_cast<int>(L'#'));
    }
  }
  else if(name == COMPILER_S_ELEM)
  {
    requireEmptyError(name);
    wstring symbol = L"<" + attrib(COMPILER_N_ATTR) + L">";

    if(!alphabet.isSymbolDefined(symbol))
    {
      wcerr << L"Error (" << xmlTextReaderGetParserLineNumber(reader);
      wcerr << L"): Undefined symbol '" << symbol << L"'." << endl;
      exit(EXIT_FAILURE);
    }
    
    result.push_back(alphabet(symbol));
  }
  else
  {
    wcerr << L"Error (" << xmlTextReaderGetParserLineNumber(reader);
    wcerr << L"): Invalid specification of element '<" << name;
    wcerr << L">' in this context." << endl;
    exit(EXIT_FAILURE);
  }
}

void
Compiler::skipBlanks(wstring &name)
{
  while(name == L"#text" || name == L"#comment")
  {
    if(name != L"#comment")
    {
      if(!allBlanks())
      {
        wcerr << L"Error (" << xmlTextReaderGetParserLineNumber(reader); 
        wcerr << L"): Invalid construction." << endl;
        exit(EXIT_FAILURE);
      }
    }
    
    xmlTextReaderRead(reader);
    name = XMLParseUtil::towstring(xmlTextReaderConstName(reader));
  }
}

void
Compiler::skip(wstring &name, wstring const &elem)
{
  skip(name, elem, true);
}

void
Compiler::skip(wstring &name, wstring const &elem, bool open)
{
  xmlTextReaderRead(reader);
  name = XMLParseUtil::towstring(xmlTextReaderConstName(reader));
  wstring slash;

  if(!open)
  {
    slash = L"/";
  }
  
  while(name == L"#text" || name == L"#comment")
  {
    if(name != L"#comment")
    {
      if(!allBlanks())
      {
        wcerr << L"Error (" << xmlTextReaderGetParserLineNumber(reader);
        wcerr << L"): Invalid construction." << endl;
        exit(EXIT_FAILURE);
      }
    }
    xmlTextReaderRead(reader);
    name = XMLParseUtil::towstring(xmlTextReaderConstName(reader));  
  }    
    
  if(name != elem)
  {
    wcerr << L"Error (" << xmlTextReaderGetParserLineNumber(reader);
    wcerr << L"): Expected '<" << slash << elem << L">'." << endl;
    exit(EXIT_FAILURE);
  }  
}

EntryToken
Compiler::procIdentity(bool ig)
{
  list<int> both_sides;

  if(!xmlTextReaderIsEmptyElement(reader))
  {
    wstring name = L"";

    while(true)
    {
      xmlTextReaderRead(reader);
      name = XMLParseUtil::towstring(xmlTextReaderConstName(reader));
      if(name == COMPILER_IDENTITY_ELEM || name == COMPILER_IDENTITYGROUP_ELEM)
      {
        break;
      }
      readString(both_sides, name);
    }
  }
  
  if(verbose && first_element && (both_sides.front() == (int)L' ')) 
  {
    wcerr << L"Error (" << xmlTextReaderGetParserLineNumber(reader);
    wcerr << L"): Entry begins with space." << endl; 
  }
  first_element = false; 
  EntryToken e;
  if(ig)
  {
    list<int> right;
    right.push_back(static_cast<int>(L'#'));
    right.insert(right.end(), both_sides.begin(), both_sides.end());
    e.setSingleTransduction(both_sides, right);
  }
  else
  {
    e.setSingleTransduction(both_sides, both_sides);
  }
  return e;
}

EntryToken
Compiler::procTransduction()
{
  list<int> lhs, rhs;
  wstring name;
  
  skip(name, COMPILER_LEFT_ELEM);

  if(!xmlTextReaderIsEmptyElement(reader))
  {
    name = L"";
    while(true)
    {
      xmlTextReaderRead(reader);
      name = XMLParseUtil::towstring(xmlTextReaderConstName(reader));
      if(name == COMPILER_LEFT_ELEM)
      {
        break;
      }
      readString(lhs, name);
    }
  }

  if(verbose && first_element && (lhs.front() == (int)L' ')) 
  {
    wcerr << L"Error (" << xmlTextReaderGetParserLineNumber(reader);
    wcerr << L"): Entry begins with space." << endl;
  }
  first_element = false;
 
  skip(name, COMPILER_RIGHT_ELEM);

  if(!xmlTextReaderIsEmptyElement(reader))
  {
    name = L"";
    while(true)
    {
      xmlTextReaderRead(reader);
      name = XMLParseUtil::towstring(xmlTextReaderConstName(reader));
      if(name == COMPILER_RIGHT_ELEM)
      {
        break;
      }
      readString(rhs, name);
    }    
  }

  skip(name, COMPILER_PAIR_ELEM, false);
  
  EntryToken e;
  e.setSingleTransduction(lhs, rhs);
  return e;
}

wstring
Compiler::attrib(wstring const &name)
{
  return XMLParseUtil::attrib(reader, name);
} 

EntryToken
Compiler::procPar()
{
  EntryToken e;
  wstring nomparadigma = attrib(COMPILER_N_ATTR);
  first_element = false;

  if(current_paradigm != L"" && nomparadigma == current_paradigm)
  {
    wcerr << L"Error (" << xmlTextReaderGetParserLineNumber(reader);
    wcerr << L"): Paradigm refers to itself '" << nomparadigma << L"'." <<endl;
    exit(EXIT_FAILURE);
  }

  if(paradigms.find(nomparadigma) == paradigms.end())
  {
    wcerr << L"Error (" << xmlTextReaderGetParserLineNumber(reader);
    wcerr << L"): Undefined paradigm '" << nomparadigma << L"'." << endl;
    exit(EXIT_FAILURE);
  }
  e.setParadigm(nomparadigma);
  return e;
}

void
Compiler::insertEntryTokens(vector<EntryToken> const &elements)
{
  if(current_paradigm != L"")
  {
    // compilation of paradigms
    Transducer &t = paradigms[current_paradigm];
    int e = t.getInitial();
    
    for(unsigned int i = 0, limit = elements.size(); i < limit; i++)
    {
      if(elements[i].isParadigm())
      {
	e = t.insertTransducer(e, paradigms[elements[i].paradigmName()]);
      }
      else if(elements[i].isSingleTransduction())
      {
        e = matchTransduction(elements[i].left(), 
                                  elements[i].right(), e, t);
      }
      else if(elements[i].isRegexp())
      {
	RegexpCompiler analyzer;
	analyzer.initialize(&alphabet);
	analyzer.compile(elements[i].regExp());
	e = t.insertTransducer(e, analyzer.getTransducer(), alphabet(0,0));
      }
      else
      {
        wcerr << L"Error (" << xmlTextReaderGetParserLineNumber(reader);
        wcerr << L"): Invalid entry token." << endl;
        exit(EXIT_FAILURE);
      }
    }
    t.setFinal(e);
  }
  else
  {
    // compilación de dictionary

    Transducer &t = sections[current_section];
    int e = t.getInitial();

    for(unsigned int i = 0, limit = elements.size(); i < limit; i++)
    {
      if(elements[i].isParadigm())
      {
        if(i == elements.size()-1)
	{
	  // paradigma sufijo
	  if(suffix_paradigms[current_section].find(elements[i].paradigmName()) != suffix_paradigms[current_section].end())
	  {
	    t.linkStates(e, suffix_paradigms[current_section][elements[i].paradigmName()], 0);
            e = postsuffix_paradigms[current_section][elements[i].paradigmName()];
	  }
          else
          {
            e = t.insertNewSingleTransduction(alphabet(0, 0), e);
            suffix_paradigms[current_section][elements[i].paradigmName()] = e;
            e = t.insertTransducer(e, paradigms[elements[i].paradigmName()]);
            postsuffix_paradigms[current_section][elements[i].paradigmName()] = e;
	  }
	}
        else if(i == 0)
	{
          // paradigma prefijo
	  if(prefix_paradigms[current_section].find(elements[i].paradigmName()) != prefix_paradigms[current_section].end())
	  {
            e = prefix_paradigms[current_section][elements[i].paradigmName()];
	  }
	  else
	  {
            e = t.insertTransducer(e, paradigms[elements[i].paradigmName()]);
            prefix_paradigms[current_section][elements[i].paradigmName()] = e;
	  }
        }
	else
	{
          // paradigma intermedio
          e = t.insertTransducer(e, paradigms[elements[i].paradigmName()]);
	}
      }
      else if(elements[i].isRegexp())
      {
	RegexpCompiler analyzer;
	analyzer.initialize(&alphabet);
	analyzer.compile(elements[i].regExp());
	e = t.insertTransducer(e, analyzer.getTransducer(), alphabet(0,0));
      }
      else
      {
        e = matchTransduction(elements[i].left(), elements[i].right(), e, t);
      }
    }
    t.setFinal(e);
  }
}


void
Compiler::requireAttribute(wstring const &value, wstring const &attrname,
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
Compiler::procSection()
{
  int tipo=xmlTextReaderNodeType(reader);

  if(tipo != XML_READER_TYPE_END_ELEMENT)
  {
    wstring const &id = attrib(COMPILER_ID_ATTR);
    wstring const &type = attrib(COMPILER_TYPE_ATTR);
    requireAttribute(id, COMPILER_ID_ATTR, COMPILER_SECTION_ELEM);
    requireAttribute(type, COMPILER_TYPE_ATTR, COMPILER_SECTION_ELEM);
    
    current_section = id;
    current_section += L"@";
    current_section.append(type);
  }
  else
  {
    current_section = L"";
  }
}

void
Compiler::procEntry()
{
  wstring atributo=this->attrib(COMPILER_RESTRICTION_ATTR);
  wstring ignore = this->attrib(COMPILER_IGNORE_ATTR);
  wstring altval = this->attrib(COMPILER_ALT_ATTR);
  wstring varval = this->attrib(COMPILER_V_ATTR);
  wstring varl   = this->attrib(COMPILER_VL_ATTR);
  wstring varr   = this->attrib(COMPILER_VR_ATTR);

  // if entry is masked by a restriction of direction or an ignore mark
  if((atributo != L"" && atributo != direction) 
   || ignore == COMPILER_IGNORE_YES_VAL
   || (altval != L"" && altval != alt)
   || (direction == COMPILER_RESTRICTION_RL_VAL && varval != L"" && varval != variant)
   || (direction == COMPILER_RESTRICTION_RL_VAL && varl != L"" && varl != variant_left)
   || (direction == COMPILER_RESTRICTION_LR_VAL && varr != L"" && varr != variant_right))
  {
    // parse to the end of the entry
    wstring name = L"";

    while(name != COMPILER_ENTRY_ELEM)
    {
      xmlTextReaderRead(reader);
      name = XMLParseUtil::towstring(xmlTextReaderConstName(reader));
    }

    return;
  }

  vector<EntryToken> elements;

  while(true)
  {
    int ret = xmlTextReaderRead(reader);
    if(ret != 1)
    {
      wcerr << L"Error (" << xmlTextReaderGetParserLineNumber(reader);
      wcerr << L"): Parse error." << endl;
      exit(EXIT_FAILURE);
    }
    wstring name = XMLParseUtil::towstring(xmlTextReaderConstName(reader));
    skipBlanks(name);

    if(current_paradigm == L"" && verbose)
    {
      first_element = true;
    }

    int tipo = xmlTextReaderNodeType(reader);
    if(name == COMPILER_PAIR_ELEM)
    {      
      elements.push_back(procTransduction());
    }
    else if(name == COMPILER_IDENTITY_ELEM)
    {
      elements.push_back(procIdentity(false));
    }
    else if(name == COMPILER_IDENTITYGROUP_ELEM)
    {
      elements.push_back(procIdentity(true));
    }
    else if(name == COMPILER_REGEXP_ELEM)
    {
      elements.push_back(procRegexp());
    }
    else if(name == COMPILER_PAR_ELEM)
    {
      elements.push_back(procPar());

      // detección del uso de paradigmas no definidos

      wstring const &p = elements.rbegin()->paradigmName();

      if(paradigms.find(p) == paradigms.end())
      {
        wcerr << L"Error (" << xmlTextReaderGetParserLineNumber(reader);
        wcerr << L"): Undefined paradigm '" << p << L"'." <<endl;
        exit(EXIT_FAILURE);
      }
      // descartar entradas con paradigms vacíos (por las direciones,
      // normalmente
      if(paradigms[p].isEmpty())
      {
        while(name != COMPILER_ENTRY_ELEM || tipo != XML_READER_TYPE_END_ELEMENT)
        {
          xmlTextReaderRead(reader);
          name = XMLParseUtil::towstring(xmlTextReaderConstName(reader));
          tipo = xmlTextReaderNodeType(reader);
        }
        return;
      }
    }
    else if(name == COMPILER_ENTRY_ELEM && tipo == XML_READER_TYPE_END_ELEMENT)
    {
      // insertar elements into letter transducer
      insertEntryTokens(elements);
      return;
    }
    else if(name == L"#text" && allBlanks())
    {
    }
    else
    {
      wcerr << L"Error (" << xmlTextReaderGetParserLineNumber(reader);
      wcerr << L"): Invalid inclusion of '<" << name << L">' into '<" << COMPILER_ENTRY_ELEM;
      wcerr << L">'." << endl; 
      exit(EXIT_FAILURE);
    }
  }
}

void
Compiler::procNodeACX()
{
  xmlChar  const *xnombre = xmlTextReaderConstName(reader);
  wstring nombre = XMLParseUtil::towstring(xnombre);
  if(nombre == L"#text")
  {
    /* ignore */
  }
  else if(nombre == L"analysis-chars")
  {
    /* ignore */
  }
  else if(nombre == L"char")
  {
    acx_current_char = static_cast<int>(attrib(L"value")[0]);
  }
  else if(nombre == L"equiv-char")
  {
    acx_map[acx_current_char].insert(static_cast<int>(attrib(L"value")[0]));
  }
  else if(nombre == L"#comment")
  {
    /* ignore */
  }
  else
  {
    wcerr << L"Error in ACX file (" << xmlTextReaderGetParserLineNumber(reader);
    wcerr << L"): Invalid node '<" << nombre << L">'." << endl;
    exit(EXIT_FAILURE);
  }
}

void
Compiler::procNode()
{
  xmlChar const *xnombre = xmlTextReaderConstName(reader);
  wstring nombre = XMLParseUtil::towstring(xnombre);

  // HACER: optimizar el orden de ejecución de esta ristra de "ifs"

  if(nombre == L"#text")
  {
    /* ignorar */
  }
  else if(nombre == COMPILER_DICTIONARY_ELEM)
  {
    /* ignorar */
  }
  else if(nombre == COMPILER_ALPHABET_ELEM)
  {
    procAlphabet();
  }
  else if(nombre == COMPILER_SDEFS_ELEM)
  {
    /* ignorar */
  }
  else if(nombre == COMPILER_SDEF_ELEM)
  {
    procSDef();
  }
  else if(nombre == COMPILER_PARDEFS_ELEM)
  {
    /* ignorar */
  }
  else if(nombre == COMPILER_PARDEF_ELEM)
  {
    procParDef();
  }
  else if(nombre == COMPILER_ENTRY_ELEM)
  {
    procEntry();
  }
  else if(nombre == COMPILER_SECTION_ELEM)
  {
    procSection();
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

EntryToken
Compiler::procRegexp()
{
  EntryToken et;
  xmlTextReaderRead(reader);
  wstring re = XMLParseUtil::towstring(xmlTextReaderConstValue(reader));
  et.setRegexp(re);
  xmlTextReaderRead(reader);
  return et;
}

void 
Compiler::write(FILE *output)
{
  // letters
  Compression::wstring_write(letters, output);
  
  // symbols
  alphabet.write(output);
  
  // transducers
  Compression::multibyte_write(sections.size(), output);

  int conta=0;
  for(map<wstring, Transducer, Ltstr>::iterator it = sections.begin(),
                                               limit = sections.end(); 
      it != limit; it++)
  {
    conta++;
    wcout << it->first << " " << it->second.size();
    wcout << " " << it->second.numberOfTransitions() << endl;
    Compression::wstring_write(it->first, output);
    it->second.write(output);
  }
}

void
Compiler::setAltValue(string const &a)
{
  alt = XMLParseUtil::stows(a);
}

void
Compiler::setVariantValue(string const &v)
{
  variant = XMLParseUtil::stows(v);
}

void
Compiler::setVariantLeftValue(string const &v)
{
  variant_left = XMLParseUtil::stows(v);
}

void
Compiler::setVariantRightValue(string const &v)
{
  variant_right = XMLParseUtil::stows(v);
}

void
Compiler::setVerbose(bool verbosity)
{
  verbose = verbosity;
}
