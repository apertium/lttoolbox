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
#include <lttoolbox/expander.h>
#include <lttoolbox/lt_locale.h>
#include <lttoolbox/xml_parse_util.h>
#include <lttoolbox/my_stdio.h>

#include <cstdlib>
#include <iostream>
#include <libxml/encoding.h>

#ifdef _WIN32
#include <utf8_fwrap.h>
#endif

using namespace std;

Expander::Expander() :
reader(0)
{
  LtLocale::tryToSetLocale();
}

Expander::~Expander()
{
}

void
Expander::expand(string const &fichero, FILE *output)
{
  reader = xmlReaderForFile(fichero.c_str(), NULL, 0);
  if(reader == NULL)
  {
    wcerr << "Error: Cannot open '" << fichero << "'." << endl;
    exit(EXIT_FAILURE);
  }

  int ret = xmlTextReaderRead(reader);
  while(ret == 1)
  {
    procNode(output);
    ret = xmlTextReaderRead(reader);
  }

  if(ret != 0)
  {
    wcerr << L"Error: Parse error at the end of input." << endl;
  }

  xmlFreeTextReader(reader);
  xmlCleanupParser();
}

void
Expander::procParDef()
{
  int tipo=xmlTextReaderNodeType(reader);

  if(tipo != XML_READER_TYPE_END_ELEMENT)
  {
    current_paradigm = attrib(Compiler::COMPILER_N_ATTR);
  }
  else
  {
    current_paradigm = L"";
  }
}

void
Expander::requireEmptyError(wstring const &name)
{
  if(!xmlTextReaderIsEmptyElement(reader))
  {
    wcerr << L"Error (" << xmlTextReaderGetParserLineNumber(reader);
    wcerr << L"): Non-empty element '<" << name << L">' should be empty." << endl;
    exit(EXIT_FAILURE);
  }
}

bool 
Expander::allBlanks()
{
  bool flag = true;
  wstring text = XMLParseUtil::towstring(xmlTextReaderConstValue(reader));
  
  for(unsigned int i = 0, limit = text.size(); i < limit; i++)
  {
    flag = flag && isspace(text[i]);
  }
  
  return flag;
}

void 
Expander::readString(wstring &result, wstring const &name)
{
  if(name == L"#text")
  {
    wstring value = XMLParseUtil::towstring(xmlTextReaderConstValue(reader));
    result.append(value);
  }
  else if(name == Compiler::COMPILER_BLANK_ELEM)
  {
    requireEmptyError(name);
    result += L' ';
  }
  else if(name == Compiler::COMPILER_JOIN_ELEM)
  {
    requireEmptyError(name);
    result += L'+';
  }
  else if(name == Compiler::COMPILER_POSTGENERATOR_ELEM)
  {
    requireEmptyError(name);
    result += L'~';
  }
  else if(name == Compiler::COMPILER_GROUP_ELEM)
  {
    int tipo=xmlTextReaderNodeType(reader);
    if(tipo != XML_READER_TYPE_END_ELEMENT)
    {
      result += L'#';
    }
  }
  else if(name == Compiler::COMPILER_S_ELEM)
  {
    requireEmptyError(name);
    result += L'<';
    result.append(attrib(Compiler::COMPILER_N_ATTR));
    result += L'>';
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
Expander::skipBlanks(wstring &name)
{
  if(name == L"#text")
  {
    if(!allBlanks())
    {
      wcerr << L"Error (" << xmlTextReaderGetParserLineNumber(reader); 
      wcerr << L"): Invalid construction." << endl;
      exit(EXIT_FAILURE);
    }
    xmlTextReaderRead(reader);
    name = XMLParseUtil::towstring(xmlTextReaderConstName(reader));
  }
}

void
Expander::skip(wstring &name, wstring const &elem)
{
  xmlTextReaderRead(reader);
  name = XMLParseUtil::towstring(xmlTextReaderConstName(reader));
  
  if(name == L"#text")
  {
    if(!allBlanks())
    {
      wcerr << L"Error (" << xmlTextReaderGetParserLineNumber(reader);
      wcerr << L"): Invalid construction." << endl;
      exit(EXIT_FAILURE);
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
Expander::procIdentity()
{
  wstring both_sides = L"";

  if(!xmlTextReaderIsEmptyElement(reader))
  {
    wstring name = L"";

    while(true)
    {
      xmlTextReaderRead(reader);
      name = XMLParseUtil::towstring(xmlTextReaderConstName(reader));
      if(name == Compiler::COMPILER_IDENTITY_ELEM)
      {
        break;
      }
      readString(both_sides, name);
    }
  }
  return both_sides;  
}

pair<wstring, wstring>
Expander::procIdentityGroup()
{
  wstring lhs = L"";
  wstring rhs = L"#";
  wstring both_sides = L"";

  if(!xmlTextReaderIsEmptyElement(reader))
  {
    wstring name = L"";

    while(true)
    {
      xmlTextReaderRead(reader);
      name = XMLParseUtil::towstring(xmlTextReaderConstName(reader));
      if(name == Compiler::COMPILER_IDENTITYGROUP_ELEM)
      {
        break;
      }
      readString(both_sides, name);
    }
  }
  lhs += both_sides;
  rhs += both_sides;

  pair<wstring, wstring> e(lhs, rhs);
  return e;
}

pair<wstring, wstring>
Expander::procTransduction()
{
  wstring lhs = L"", rhs = L""; 
  wstring name = L"";
  
  skip(name, Compiler::COMPILER_LEFT_ELEM);

  if(!xmlTextReaderIsEmptyElement(reader))
  {
    name = L"";
    while(true)
    {
      xmlTextReaderRead(reader);
      name = XMLParseUtil::towstring(xmlTextReaderConstName(reader));
      if(name == Compiler::COMPILER_LEFT_ELEM)
      {
        break;
      }
      readString(lhs, name);
    }
  }
 
  skip(name, Compiler::COMPILER_RIGHT_ELEM);

  if(!xmlTextReaderIsEmptyElement(reader))
  {
    name = L"";
    while(true)
    {
      xmlTextReaderRead(reader);
      name = XMLParseUtil::towstring(xmlTextReaderConstName(reader));
      if(name == Compiler::COMPILER_RIGHT_ELEM)
      {
        break;
      }
      readString(rhs, name);
    }    
  }

  skip(name, Compiler::COMPILER_PAIR_ELEM);  
  
  pair<wstring, wstring> e(lhs, rhs);
  return e;
}

wstring
Expander::attrib(wstring const &nombre)
{
  return XMLParseUtil::attrib(reader, nombre);
} 

wstring
Expander::procPar()
{
  EntryToken e;
  wstring nomparadigma = attrib(Compiler::COMPILER_N_ATTR);
  return nomparadigma;
}

void
Expander::requireAttribute(wstring const &value, wstring const &attrname,
                           wstring const &elemname)
{
  if(value == L"")
  {
    wcerr << L"Error (" << xmlTextReaderGetParserLineNumber(reader);  
    wcerr << L"): '<" << elemname;
    wcerr << L"' element must specify non-void '";
    wcerr<< attrname << L"' attribute." << endl;
    exit(EXIT_FAILURE);
  }  
}

void
Expander::procEntry(FILE *output)
{
  wstring atributo=this->attrib(Compiler::COMPILER_RESTRICTION_ATTR);
  wstring entrname=this->attrib(Compiler::COMPILER_LEMMA_ATTR);
  wstring altval = this->attrib(Compiler::COMPILER_ALT_ATTR);
  wstring varval = this->attrib(Compiler::COMPILER_V_ATTR);
  wstring varl   = this->attrib(Compiler::COMPILER_VL_ATTR);
  wstring varr   = this->attrib(Compiler::COMPILER_VR_ATTR);
  
  wstring myname = L"";
  if(this->attrib(Compiler::COMPILER_IGNORE_ATTR) == L"yes"
   || altval != L"" && altval != alt
   || (varval != L"" && varval != variant && atributo == Compiler::COMPILER_RESTRICTION_RL_VAL)
   || ((varl != L"" && varl != variant_left) && (varr != L"" && varr != variant_right))
   || (varl != L"" && varl != variant_left && atributo == Compiler::COMPILER_RESTRICTION_RL_VAL)
   || (varr != L"" && varr != variant_right && atributo == Compiler::COMPILER_RESTRICTION_LR_VAL))
  {    
    do
    {
      int ret = xmlTextReaderRead(reader);
      if(ret != 1)
      {
        wcerr << L"Error (" << xmlTextReaderGetParserLineNumber(reader);
        wcerr << L"): Parse error." << endl;
        exit(EXIT_FAILURE);
      }
      myname = XMLParseUtil::towstring(xmlTextReaderConstName(reader));
     // wcerr << L"Hola " << myname << L" " << Compiler::COMPILER_ENTRY_ELEM << endl;
    }
    while(myname != Compiler::COMPILER_ENTRY_ELEM);
    return;
  }
  
  EntList items, items_lr, items_rl;
  if(atributo == Compiler::COMPILER_RESTRICTION_LR_VAL 
   || (varval != L"" && varval != variant && atributo != Compiler::COMPILER_RESTRICTION_RL_VAL)
   || varl != L"" && varl != variant_left)
  {
    items_lr.push_back(pair<wstring, wstring>(L"", L""));
  }
  else if(atributo == Compiler::COMPILER_RESTRICTION_RL_VAL
        || (varr != L"" && varr != variant_right))
  {
    items_rl.push_back(pair<wstring, wstring>(L"", L""));
  }
  else
  {
    items.push_back(pair<wstring, wstring>(L"", L""));
  }

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

    int tipo = xmlTextReaderNodeType(reader);
    if(name == Compiler::COMPILER_PAIR_ELEM)
    {      
      pair<wstring, wstring> p = procTransduction();
      append(items, p);
      append(items_lr, p);
      append(items_rl, p);
    }
    else if(name == Compiler::COMPILER_IDENTITY_ELEM)
    {
      wstring val = procIdentity();
      append(items, val);
      append(items_lr, val);
      append(items_rl, val);
    }
    else if(name == Compiler::COMPILER_IDENTITYGROUP_ELEM)
    {      
      pair<wstring, wstring> p = procIdentityGroup();
      append(items, p);
      append(items_lr, p);
      append(items_rl, p);
    }
    else if(name == Compiler::COMPILER_REGEXP_ELEM)
    {
      wstring val = L"__REGEXP__" + procRegexp();
      append(items, val);
      append(items_lr, val);
      append(items_rl, val);
    }
    else if(name == Compiler::COMPILER_PAR_ELEM)
    {
      wstring p = procPar();
      // detección del uso de paradigmas no definidos

      if(paradigm.find(p) == paradigm.end() &&
         paradigm_lr.find(p) == paradigm_lr.end() &&
         paradigm_rl.find(p) == paradigm_rl.end())
      {
        wcerr << L"Error (" << xmlTextReaderGetParserLineNumber(reader);
        wcerr << L"): Undefined paradigm '" << p << L"'." <<endl;
        exit(EXIT_FAILURE);
      }
      
      if(atributo == Compiler::COMPILER_RESTRICTION_LR_VAL)
      {
        if(paradigm[p].size() == 0 && paradigm_lr[p].size() == 0)
        {
          skip(name, Compiler::COMPILER_ENTRY_ELEM);
          return;
        }
        EntList first = items_lr;
        append(first, paradigm[p]);
        append(items_lr, paradigm_lr[p]);
        items_lr.insert(items_lr.end(), first.begin(), first.end());
      }
      else if(atributo == Compiler::COMPILER_RESTRICTION_RL_VAL)
      {
        if(paradigm[p].size() == 0 && paradigm_rl[p].size() == 0)
        {
          skip(name, Compiler::COMPILER_ENTRY_ELEM);
          return;
        }
        EntList first = items_rl;
        append(first, paradigm[p]);
        append(items_rl, paradigm_rl[p]);
        items_rl.insert(items_rl.end(), first.begin(), first.end());
      }
      else
      {
        if(paradigm_lr[p].size() > 0)
        {
          items_lr.insert(items_lr.end(), items.begin(), items.end());
        }
        if(paradigm_rl[p].size() > 0)
        {
          items_rl.insert(items_rl.end(), items.begin(), items.end());
        }
        
        EntList aux_lr = items_lr;
        EntList aux_rl = items_rl;
        append(aux_lr, paradigm[p]);
        append(aux_rl, paradigm[p]);
        append(items_lr, paradigm_lr[p]);
        append(items_rl, paradigm_rl[p]);
        append(items, paradigm[p]);
        items_rl.insert(items_rl.end(), aux_rl.begin(), aux_rl.end());
        items_lr.insert(items_lr.end(), aux_lr.begin(), aux_lr.end());
      }
    }
    else if(name == Compiler::COMPILER_ENTRY_ELEM && tipo == XML_READER_TYPE_END_ELEMENT)
    {
      if(current_paradigm == L"")
      {
        for(EntList::iterator it = items.begin(),
                                                 limit = items.end();
            it != limit; it++)
        {
          fputws_unlocked(it->first.c_str(), output);
          fputwc_unlocked(L':', output);
          fputws_unlocked(it->second.c_str(), output);
          fputwc_unlocked(L'\n', output);
        }
        for(EntList::iterator it = items_lr.begin(),
                                                 limit = items_lr.end();
            it != limit; it++)
        {
          fputws_unlocked(it->first.c_str(), output);
          fputwc_unlocked(L':', output);
          fputwc_unlocked(L'>', output);
          fputwc_unlocked(L':', output);
          fputws_unlocked(it->second.c_str(), output);
          fputwc_unlocked(L'\n', output);
        }
        for(EntList::iterator it = items_rl.begin(),
                                                 limit = items_rl.end();
            it != limit; it++)
        {
          fputws_unlocked(it->first.c_str(), output);
          fputwc_unlocked(L':', output);
          fputwc_unlocked(L'<', output);
          fputwc_unlocked(L':', output);
          fputws_unlocked(it->second.c_str(), output);
          fputwc_unlocked(L'\n', output);
        }
      }
      else
      {
        paradigm_lr[current_paradigm].insert(paradigm_lr[current_paradigm].end(),
                                             items_lr.begin(), items_lr.end());
        paradigm_rl[current_paradigm].insert(paradigm_rl[current_paradigm].end(),
                                             items_rl.begin(), items_rl.end());
        paradigm[current_paradigm].insert(paradigm[current_paradigm].end(),
                                          items.begin(), items.end());
      }

      return;
    }
    else if(name == L"#text" && allBlanks())
    {
    }
    else if(name == L"#comment")
    {
    }
    else
    {
      wcerr << L"Error (" << xmlTextReaderGetParserLineNumber(reader);
      wcerr << L"): Invalid inclusion of '<" << name << L">' into '<" << Compiler::COMPILER_ENTRY_ELEM;
      wcerr << L">'." << endl;
      exit(EXIT_FAILURE);
    }
  }
}

void
Expander::procNode(FILE *output)
{
  xmlChar const *xnombre = xmlTextReaderConstName(reader);
  wstring nombre = XMLParseUtil::towstring(xnombre);

  // HACER: optimizar el orden de ejecución de esta ristra de "ifs"

  if(nombre == L"#text")
  {
    /* ignorar */
  }
  else if(nombre == Compiler::COMPILER_DICTIONARY_ELEM)
  {
    /* ignorar */
  }
  else if(nombre == Compiler::COMPILER_ALPHABET_ELEM)
  {
    /* ignorar */
  }
  else if(nombre == Compiler::COMPILER_SDEFS_ELEM)
  {
    /* ignorar */
  }
  else if(nombre == Compiler::COMPILER_SDEF_ELEM)
  {
    /* ignorar */
  }
  else if(nombre == Compiler::COMPILER_PARDEFS_ELEM)
  {
    /* ignorar */
  }
  else if(nombre == Compiler::COMPILER_PARDEF_ELEM)
  {
    procParDef();
  }
  else if(nombre == Compiler::COMPILER_ENTRY_ELEM)
  {
    procEntry(output);
  }
  else if(nombre == Compiler::COMPILER_SECTION_ELEM)
  {
    /* ignorar */
  }
  else if(nombre == L"#comment")
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

wstring
Expander::procRegexp()
{
  xmlTextReaderRead(reader);
  wstring re = XMLParseUtil::towstring(xmlTextReaderConstValue(reader));
  xmlTextReaderRead(reader);
  return re;
}

void
Expander::append(EntList &result,
		 EntList const &endings)
{
  EntList temp;
  EntList::iterator it, limit;
  EntList::const_iterator it2, limit2;

  for(it = result.begin(), limit = result.end(); it != limit; it++)
  {
    for(it2 = endings.begin(), limit2 = endings.end(); it2 != limit2; it2++)
    {
      temp.push_back(pair<wstring, wstring>(it->first + it2->first, 
		   		          it->second + it2->second));
    }
  }

  result = temp;
}

void
Expander::append(EntList &result, wstring const &endings)
{  
  EntList::iterator it, limit;
  for(it = result.begin(), limit = result.end(); it != limit; it++)
  {
    it->first.append(endings);
    it->second.append(endings);
  }
}

void
Expander::append(EntList &result, 
		 pair<wstring, wstring> const &endings)
{
  EntList::iterator it, limit;
  for(it = result.begin(), limit = result.end(); it != limit; it++)
  {
    it->first.append(endings.first);
    it->second.append(endings.second);
  }
}

void
Expander::setAltValue(string const &a)
{
  alt = XMLParseUtil::stows(a);
}

void
Expander::setVariantValue(string const &v)
{
  variant = XMLParseUtil::stows(v);
}

void
Expander::setVariantLeftValue(string const &v)
{
  variant_left = XMLParseUtil::stows(v);
}

void
Expander::setVariantRightValue(string const &v)
{
  variant_right = XMLParseUtil::stows(v);
}
