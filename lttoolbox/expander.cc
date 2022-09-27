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

#include <lttoolbox/compiler.h>
#include <lttoolbox/expander.h>
#include <lttoolbox/xml_parse_util.h>

#include <cstdlib>
#include <iostream>
#include <libxml/encoding.h>


Expander::Expander()
{
}

Expander::~Expander()
{
}

void
Expander::expand(std::string const &file, UFILE* output)
{
  reader = XMLParseUtil::open_or_exit(file.c_str());

  int ret = xmlTextReaderRead(reader);
  while(ret == 1)
  {
    procNode(output);
    ret = xmlTextReaderRead(reader);
  }

  if(ret != 0)
  {
    std::cerr << "Error: Parse error at the end of input." << std::endl;
  }

  xmlFreeTextReader(reader);
  xmlCleanupParser();
}

void
Expander::procParDef()
{
  int type=xmlTextReaderNodeType(reader);

  if(type != XML_READER_TYPE_END_ELEMENT)
  {
    current_paradigm = attrib(Compiler::COMPILER_N_ATTR);
  }
  else
  {
    current_paradigm.clear();
  }
}

void
Expander::requireEmptyError(UStringView name)
{
  if(!xmlTextReaderIsEmptyElement(reader))
  {
    std::cerr << "Error (" << xmlTextReaderGetParserLineNumber(reader);
    std::cerr << "): Non-empty element '<" << name << ">' should be empty." << std::endl;
    exit(EXIT_FAILURE);
  }
}

bool
Expander::allBlanks()
{
  return XMLParseUtil::allBlanks(reader);
}

void
Expander::readString(UString &result, UStringView name)
{
  if(name == Compiler::COMPILER_TEXT_NODE)
  {
    UString value = XMLParseUtil::readValue(reader);
    UString escaped = "^$/<>{}\\*@#+~:"_u;
    for(size_t i = value.size()-1; i > 0; i--)
    {
      if(escaped.find(value[i]) != UString::npos) {
        value.insert(value.begin()+i, '\\');
      }
    }
    result.append(value);
  }
  else if(name == Compiler::COMPILER_BLANK_ELEM)
  {
    requireEmptyError(name);
    result += ' ';
  }
  else if(name == Compiler::COMPILER_M_ELEM)
  {
    requireEmptyError(name);
    if(keep_boundaries)
    {
      result += '>';
    }
  }
  else if(name == Compiler::COMPILER_JOIN_ELEM)
  {
    requireEmptyError(name);
    result += '+';
  }
  else if(name == Compiler::COMPILER_POSTGENERATOR_ELEM)
  {
    requireEmptyError(name);
    result += '~';
  }
  else if(name == Compiler::COMPILER_GROUP_ELEM)
  {
    int type=xmlTextReaderNodeType(reader);
    if(type != XML_READER_TYPE_END_ELEMENT)
    {
      result += '#';
    }
  }
  else if(name == Compiler::COMPILER_S_ELEM)
  {
    requireEmptyError(name);
    result += '<';
    result.append(attrib(Compiler::COMPILER_N_ATTR));
    result += '>';
  }
  else if(name == Compiler::COMPILER_LSX_WB_ELEM) {
    result += "<$>"_u;
  }
  else if(name == Compiler::COMPILER_LSX_CHAR_ELEM) {
    result += "<ANY_CHAR>"_u;
  }
  else if(name == Compiler::COMPILER_LSX_TAG_ELEM) {
    result += "<ANY_TAG>"_u;
  }
  else
  {
    std::cerr << "Error (" << xmlTextReaderGetParserLineNumber(reader);
    std::cerr << "): Invalid specification of element '<" << name;
    std::cerr << ">' in this context." << std::endl;
    exit(EXIT_FAILURE);
  }
}

void
Expander::skipBlanks(UString &name)
{
  if(name == Compiler::COMPILER_TEXT_NODE)
  {
    if(!allBlanks())
    {
      std::cerr << "Error (" << xmlTextReaderGetParserLineNumber(reader);
      std::cerr << "): Invalid construction." << std::endl;
      exit(EXIT_FAILURE);
    }
    xmlTextReaderRead(reader);
    name = XMLParseUtil::readName(reader);
  }
}

void
Expander::skip(UString &name, UStringView elem)
{
  xmlTextReaderRead(reader);
  name = XMLParseUtil::readName(reader);

  if(name == Compiler::COMPILER_TEXT_NODE)
  {
    if(!allBlanks())
    {
      std::cerr << "Error (" << xmlTextReaderGetParserLineNumber(reader);
      std::cerr << "): Invalid construction." << std::endl;
      exit(EXIT_FAILURE);
    }
    xmlTextReaderRead(reader);
    name = XMLParseUtil::readName(reader);
  }

  if(name != elem)
  {
    std::cerr << "Error (" << xmlTextReaderGetParserLineNumber(reader);
    std::cerr << "): Expected '<" << elem << ">'." << std::endl;
    exit(EXIT_FAILURE);
  }
}

UString
Expander::procIdentity()
{
  UString both_sides;

  if(!xmlTextReaderIsEmptyElement(reader))
  {
    UString name;

    while(true)
    {
      xmlTextReaderRead(reader);
      name = XMLParseUtil::readName(reader);
      if(name == Compiler::COMPILER_IDENTITY_ELEM)
      {
        break;
      }
      readString(both_sides, name);
    }
  }
  return both_sides;
}

std::pair<UString, UString>
Expander::procIdentityGroup()
{
  UString lhs;
  UString rhs = "#"_u;
  UString both_sides;

  if(!xmlTextReaderIsEmptyElement(reader))
  {
    UString name;

    while(true)
    {
      xmlTextReaderRead(reader);
      name = XMLParseUtil::readName(reader);
      if(name == Compiler::COMPILER_IDENTITYGROUP_ELEM)
      {
        break;
      }
      readString(both_sides, name);
    }
  }
  lhs += both_sides;
  rhs += both_sides;

  std::pair<UString, UString> e(lhs, rhs);
  return e;
}

std::pair<UString, UString>
Expander::procTransduction()
{
  UString lhs, rhs;
  UString name;

  skip(name, Compiler::COMPILER_LEFT_ELEM);

  if(!xmlTextReaderIsEmptyElement(reader))
  {
    name.clear();
    while(true)
    {
      xmlTextReaderRead(reader);
      name = XMLParseUtil::readName(reader);
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
    name.clear();
    while(true)
    {
      xmlTextReaderRead(reader);
      name = XMLParseUtil::readName(reader);
      if(name == Compiler::COMPILER_RIGHT_ELEM)
      {
        break;
      }
      readString(rhs, name);
    }
  }

  skip(name, Compiler::COMPILER_PAIR_ELEM);

  std::pair<UString, UString> e(lhs, rhs);
  return e;
}

UString
Expander::attrib(UStringView name)
{
  return XMLParseUtil::attrib(reader, name);
}

UString
Expander::procPar()
{
  EntryToken e;
  UString paradigm_name = attrib(Compiler::COMPILER_N_ATTR);
  return paradigm_name;
}

void
Expander::requireAttribute(UStringView value, UStringView attrname, UStringView elemname)
{
  if(value.empty())
  {
    std::cerr << "Error (" << xmlTextReaderGetParserLineNumber(reader);
    std::cerr << "): '<" << elemname;
    std::cerr << "' element must specify non-void '";
    std::cerr<< attrname << "' attribute." << std::endl;
    exit(EXIT_FAILURE);
  }
}

void
Expander::procEntry(UFILE* output)
{
  UString attribute = this->attrib(Compiler::COMPILER_RESTRICTION_ATTR);
  UString entrname  = this->attrib(Compiler::COMPILER_LEMMA_ATTR);
  UString altval    = this->attrib(Compiler::COMPILER_ALT_ATTR);
  UString varval    = this->attrib(Compiler::COMPILER_V_ATTR);
  UString varl      = this->attrib(Compiler::COMPILER_VL_ATTR);
  UString varr      = this->attrib(Compiler::COMPILER_VR_ATTR);
  UString wsweight  = this->attrib(Compiler::COMPILER_WEIGHT_ATTR);

  UString myname;
  if(this->attrib(Compiler::COMPILER_IGNORE_ATTR) == Compiler::COMPILER_IGNORE_YES_VAL
   || (!altval.empty() && altval != alt)
   || (!varval.empty() && varval != variant && attribute == Compiler::COMPILER_RESTRICTION_RL_VAL)
   || ((!varl.empty() && varl != variant_left) && (!varr.empty() && varr != variant_right))
   || (!varl.empty() && varl != variant_left && attribute == Compiler::COMPILER_RESTRICTION_RL_VAL)
   || (!varr.empty() && varr != variant_right && attribute == Compiler::COMPILER_RESTRICTION_LR_VAL))
  {
    do
    {
      int ret = xmlTextReaderRead(reader);
      if(ret != 1)
      {
        std::cerr << "Error (" << xmlTextReaderGetParserLineNumber(reader);
        std::cerr << "): Parse error." << std::endl;
        exit(EXIT_FAILURE);
      }
      myname = XMLParseUtil::readName(reader);
    }
    while(myname != Compiler::COMPILER_ENTRY_ELEM);
    return;
  }

  EntList items, items_lr, items_rl;
  if(attribute == Compiler::COMPILER_RESTRICTION_LR_VAL
   || (!varval.empty() && varval != variant && attribute != Compiler::COMPILER_RESTRICTION_RL_VAL)
   || (!varl.empty() && varl != variant_left))
  {
    items_lr.push_back(make_pair(""_u, ""_u));
  }
  else if(attribute == Compiler::COMPILER_RESTRICTION_RL_VAL
        || (!varr.empty() && varr != variant_right))
  {
    items_rl.push_back(make_pair(""_u, ""_u));
  }
  else
  {
    items.push_back(make_pair(""_u, ""_u));
  }

  while(true)
  {
    int ret = xmlTextReaderRead(reader);
    if(ret != 1)
    {
      std::cerr << "Error (" << xmlTextReaderGetParserLineNumber(reader);
      std::cerr << "): Parse error." << std::endl;
      exit(EXIT_FAILURE);
    }
    UString name = XMLParseUtil::readName(reader);
    skipBlanks(name);

    int type = xmlTextReaderNodeType(reader);
    if(name == Compiler::COMPILER_PAIR_ELEM)
    {
      std::pair<UString, UString> p = procTransduction();
      append(items, p);
      append(items_lr, p);
      append(items_rl, p);
    }
    else if(name == Compiler::COMPILER_IDENTITY_ELEM)
    {
      UString val = procIdentity();
      append(items, val);
      append(items_lr, val);
      append(items_rl, val);
    }
    else if(name == Compiler::COMPILER_IDENTITYGROUP_ELEM)
    {
      std::pair<UString, UString> p = procIdentityGroup();
      append(items, p);
      append(items_lr, p);
      append(items_rl, p);
    }
    else if(name == Compiler::COMPILER_REGEXP_ELEM)
    {
      UString val = "__REGEXP__"_u + procRegexp();
      append(items, val);
      append(items_lr, val);
      append(items_rl, val);
    }
    else if(name == Compiler::COMPILER_PAR_ELEM)
    {
      UString p = procPar();
      // detection of the use of undefined paradigms

      if(paradigm.find(p) == paradigm.end() &&
         paradigm_lr.find(p) == paradigm_lr.end() &&
         paradigm_rl.find(p) == paradigm_rl.end())
      {
        std::cerr << "Error (" << xmlTextReaderGetParserLineNumber(reader);
        std::cerr << "): Undefined paradigm '" << p << "'." << std::endl;
        exit(EXIT_FAILURE);
      }

      if(attribute == Compiler::COMPILER_RESTRICTION_LR_VAL)
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
      else if(attribute == Compiler::COMPILER_RESTRICTION_RL_VAL)
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

        append(items_lr, paradigm_lr[p]);
        append(items_rl, paradigm_rl[p]);
        append(items, paradigm[p]);
      }
    }
    else if(name == Compiler::COMPILER_ENTRY_ELEM && type == XML_READER_TYPE_END_ELEMENT)
    {
      if(current_paradigm.empty())
      {
        for(auto& it : items)
        {
          u_fprintf(output, "%S:%S\n", it.first.c_str(), it.second.c_str());
        }
        for(auto& it : items_lr)
        {
          u_fprintf(output, "%S:>:%S\n", it.first.c_str(), it.second.c_str());
        }
        for(auto& it : items_rl)
        {
          u_fprintf(output, "%S:<:%S\n", it.first.c_str(), it.second.c_str());
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
    else if(name == Compiler::COMPILER_TEXT_NODE && allBlanks())
    {
    }
    else if(name == Compiler::COMPILER_COMMENT_NODE)
    {
    }
    else
    {
      std::cerr << "Error (" << xmlTextReaderGetParserLineNumber(reader);
      std::cerr << "): Invalid inclusion of '<" << name << ">' into '<" << Compiler::COMPILER_ENTRY_ELEM;
      std::cerr << ">'." << std::endl;
      exit(EXIT_FAILURE);
    }
  }
}

void
Expander::procNode(UFILE *output)
{
  UString name = XMLParseUtil::readName(reader);

  // TODO: optimize the execution order of this string "ifs"

  if(name == Compiler::COMPILER_TEXT_NODE)
  {
    /* ignorar */
  }
  else if(name == Compiler::COMPILER_DICTIONARY_ELEM)
  {
    /* ignorar */
  }
  else if(name == Compiler::COMPILER_ALPHABET_ELEM)
  {
    /* ignorar */
  }
  else if(name == Compiler::COMPILER_SDEFS_ELEM)
  {
    /* ignorar */
  }
  else if(name == Compiler::COMPILER_SDEF_ELEM)
  {
    /* ignorar */
  }
  else if(name == Compiler::COMPILER_PARDEFS_ELEM)
  {
    /* ignorar */
  }
  else if(name == Compiler::COMPILER_PARDEF_ELEM)
  {
    procParDef();
  }
  else if(name == Compiler::COMPILER_ENTRY_ELEM)
  {
    procEntry(output);
  }
  else if(name == Compiler::COMPILER_SECTION_ELEM)
  {
    /* ignorar */
  }
  else if(name == Compiler::COMPILER_COMMENT_NODE)
  {
    /* ignorar */
  }
  else
  {
    std::cerr << "Error (" << xmlTextReaderGetParserLineNumber(reader);
    std::cerr << "): Invalid node '<" << name << ">'." << std::endl;
    exit(EXIT_FAILURE);
  }
}

UString
Expander::procRegexp()
{
  xmlTextReaderRead(reader);
  UString val = XMLParseUtil::readValue(reader);
  UString escaped = "^$/<>{}*@#+~:"_u;
  UString ret;
  bool esc = false;
  for (auto& c : val) {
    if (esc) {
      ret += c;
      esc = false;
      continue;
    }
    if (escaped.find(c) != UString::npos) {
      ret += '\\';
    } else if (c == '\\') {
      esc = true;
    }
    ret += c;
  }
  xmlTextReaderRead(reader);
  return ret;
}

void
Expander::append(EntList &result,
                 EntList const &endings)
{
  EntList temp;

  for(auto& it : result)
  {
    for(auto& it2 : endings)
    {
      temp.push_back(std::pair<UString, UString>(it.first + it2.first,
                          it.second + it2.second));
    }
  }

  result = temp;
}

void
Expander::append(EntList &result, UStringView endings)
{
  for(auto& it : result)
  {
    it.first.append(endings);
    it.second.append(endings);
  }
}

void
Expander::append(EntList &result,
                 std::pair<UString, UString> const &endings)
{
  for(auto& it : result)
  {
    it.first.append(endings.first);
    it.second.append(endings.second);
  }
}

void
Expander::setAltValue(UStringView a)
{
  alt = a;
}

void
Expander::setVariantValue(UStringView v)
{
  variant = v;
}

void
Expander::setVariantLeftValue(UStringView v)
{
  variant_left = v;
}

void
Expander::setVariantRightValue(UStringView v)
{
  variant_right = v;
}

void
Expander::setKeepBoundaries(bool keep)
{
  keep_boundaries = keep;
}
