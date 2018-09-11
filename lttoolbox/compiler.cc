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

#include <string>
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
wstring const Compiler::COMPILER_TYPE_ATTR          = L"type";
wstring const Compiler::COMPILER_IDENTITY_ELEM      = L"i";
wstring const Compiler::COMPILER_IDENTITYGROUP_ELEM = L"ig";
wstring const Compiler::COMPILER_JOIN_ELEM          = L"j";
wstring const Compiler::COMPILER_BLANK_ELEM         = L"b";
wstring const Compiler::COMPILER_POSTGENERATOR_ELEM = L"a";
wstring const Compiler::COMPILER_GROUP_ELEM         = L"g";
wstring const Compiler::COMPILER_LEMMA_ATTR         = L"lm";
wstring const Compiler::COMPILER_IGNORE_ATTR        = L"i";
wstring const Compiler::COMPILER_IGNORE_YES_VAL     = L"yes";
wstring const Compiler::COMPILER_ALT_ATTR           = L"alt";
wstring const Compiler::COMPILER_V_ATTR             = L"v";
wstring const Compiler::COMPILER_VL_ATTR            = L"vl";
wstring const Compiler::COMPILER_VR_ATTR            = L"vr";
wstring const Compiler::COMPILER_WEIGHT_ATTR        = L"w";

Compiler::Compiler() :
reader(0),
verbose(false),
first_element(false),
default_weight(0.0000),
acx_current_char(0)
{
}

Compiler::~Compiler()
{
}

void
Compiler::parseACX(string const &file, wstring const &dir)
{
  if(dir == COMPILER_RESTRICTION_LR_VAL)
  {
    reader = xmlReaderForFile(file.c_str(), NULL, 0);
    if(reader == NULL)
    {
      wcerr << "Error: cannot open '" << file << "'." << endl;
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
Compiler::parse(string const &file, wstring const &dir)
{
  direction = dir;
  reader = xmlReaderForFile(file.c_str(), NULL, 0);
  if(reader == NULL)
  {
    wcerr << "Error: Cannot open '" << file << "'." << endl;
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
  for(auto& it : sections)
  {
    it.second.minimize();
  }
}


void
Compiler::procAlphabet()
{
  int type=xmlTextReaderNodeType(reader);

  if(type != XML_READER_TYPE_END_ELEMENT)
  {
    int ret = xmlTextReaderRead(reader);
    if(ret == 1)
    {
      xmlChar const *value = xmlTextReaderConstValue(reader);
      letters = XMLParseUtil::towstring(value);
      bool space = true;
      for(unsigned int i = 0; i < letters.length(); i++)
      {
        if(!isspace(letters.at(i)))
        {
          space = false;
          break;
        }
      }
      if(space == true)  // libxml2 returns '\n' for <alphabet></alphabet>, should be empty
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
  int type=xmlTextReaderNodeType(reader);

  if(type != XML_READER_TYPE_END_ELEMENT)
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
                           int state, Transducer &t,
                           double const &entry_weight)
{
  list<int>::const_iterator left, right, limleft, limright;

  if(direction == COMPILER_RESTRICTION_LR_VAL)
  {
    left = pi.begin();
    right = pd.begin();
    limleft = pi.end();
    limright = pd.end();
  }
  else
  {
    left = pd.begin();
    right = pi.begin();
    limleft = pd.end();
    limright = pi.end();
  }


  if(pi.size() == 0 && pd.size() == 0)
  {
    state = t.insertNewSingleTransduction(alphabet(0, 0), state, default_weight);
  }
  else
  {
    map<int, set<int> >::iterator acx_map_ptr;
    int rsymbol = 0;

    while(true)
    {
      int tag;

      acx_map_ptr = acx_map.end();

      if(left == limleft && right == limright)
      {
        break;
      }
      else if(left == limleft)
      {
        tag = alphabet(0, *right);
        right++;
      }
      else if(right == limright)
      {
        tag = alphabet(*left, 0);
        acx_map_ptr = acx_map.find(*left);
        rsymbol = 0;
        left++;
      }
      else
      {
        tag = alphabet(*left, *right);
        acx_map_ptr = acx_map.find(*left);
        rsymbol = *right;
        left++;
        right++;
      }

      double weight_value;

      if(left == limleft && right == limright)
      {
        weight_value = entry_weight;
      }
      else
      {
        weight_value = default_weight;
      }

      int new_state = t.insertSingleTransduction(tag, state, weight_value);

      if(acx_map_ptr != acx_map.end())
      {
        for(auto& it : acx_map_ptr->second)
        {
          t.linkStates(state, new_state, alphabet(it ,rsymbol), weight_value);
        }
      }
      state = new_state;
    }
  }

  return state;
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

  for(auto c : text)
  {
    flag = flag && iswspace(c);
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
    int type=xmlTextReaderNodeType(reader);
    if(type != XML_READER_TYPE_END_ELEMENT)
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
Compiler::procIdentity(wstring const &wsweight, bool ig)
{
  list<int> both_sides;
  double entry_weight = stod(wsweight);

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
    e.setSingleTransduction(both_sides, right, entry_weight);
  }
  else
  {
    e.setSingleTransduction(both_sides, both_sides, entry_weight);
  }
  return e;
}

EntryToken
Compiler::procTransduction(wstring const &wsweight)
{
  list<int> lhs, rhs;
  double entry_weight = stod(wsweight);
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
  e.setSingleTransduction(lhs, rhs, entry_weight);
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
  wstring paradigm_name = attrib(COMPILER_N_ATTR);
  first_element = false;

  if(current_paradigm != L"" && paradigm_name == current_paradigm)
  {
    wcerr << L"Error (" << xmlTextReaderGetParserLineNumber(reader);
    wcerr << L"): Paradigm refers to itself '" << paradigm_name << L"'." <<endl;
    exit(EXIT_FAILURE);
  }

  if(paradigms.find(paradigm_name) == paradigms.end())
  {
    wcerr << L"Error (" << xmlTextReaderGetParserLineNumber(reader);
    wcerr << L"): Undefined paradigm '" << paradigm_name << L"'." << endl;
    exit(EXIT_FAILURE);
  }
  e.setParadigm(paradigm_name);
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

    for(auto& element : elements)
    {
      if(element.isParadigm())
      {
        e = t.insertTransducer(e, paradigms[element.paradigmName()]);
      }
      else if(element.isSingleTransduction())
      {
        e = matchTransduction(element.left(),
                              element.right(), e, t, element.entryWeight());
      }
      else if(element.isRegexp())
      {
        RegexpCompiler analyzer;
        analyzer.initialize(&alphabet);
        analyzer.compile(element.regExp());
        e = t.insertTransducer(e, analyzer.getTransducer(), alphabet(0,0));
      }
      else
      {
        wcerr << L"Error (" << xmlTextReaderGetParserLineNumber(reader);
        wcerr << L"): Invalid entry token." << endl;
        exit(EXIT_FAILURE);
      }
    }
    t.setFinal(e, default_weight);
  }
  else
  {
    // dictionary compilation

    Transducer &t = sections[current_section];
    int e = t.getInitial();

    for(size_t i = 0, limit = elements.size(); i < limit; i++)
    {
      if(elements[i].isParadigm())
      {
        if(i == elements.size()-1)
        {
          // suffix paradigm
          if(suffix_paradigms[current_section].find(elements[i].paradigmName()) != suffix_paradigms[current_section].end())
          {
            t.linkStates(e, suffix_paradigms[current_section][elements[i].paradigmName()], 0, elements[i].entryWeight());
            e = postsuffix_paradigms[current_section][elements[i].paradigmName()];
          }
          else
          {
            e = t.insertNewSingleTransduction(alphabet(0, 0), e, elements[i].entryWeight());
            suffix_paradigms[current_section][elements[i].paradigmName()] = e;
            e = t.insertTransducer(e, paradigms[elements[i].paradigmName()]);
            postsuffix_paradigms[current_section][elements[i].paradigmName()] = e;
          }
        }
        else if(i == 0)
        {
          // prefix paradigm
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
          // intermediate paradigm
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
        e = matchTransduction(elements[i].left(), elements[i].right(), e, t, elements[i].entryWeight());
      }
    }
    t.setFinal(e, default_weight);
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
  int type=xmlTextReaderNodeType(reader);

  if(type != XML_READER_TYPE_END_ELEMENT)
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
  wstring attribute = this->attrib(COMPILER_RESTRICTION_ATTR);
  wstring ignore    = this->attrib(COMPILER_IGNORE_ATTR);
  wstring altval    = this->attrib(COMPILER_ALT_ATTR);
  wstring varval    = this->attrib(COMPILER_V_ATTR);
  wstring varl      = this->attrib(COMPILER_VL_ATTR);
  wstring varr      = this->attrib(COMPILER_VR_ATTR);
  wstring wsweight  = this->attrib(COMPILER_WEIGHT_ATTR);

  // if entry is masked by a restriction of direction or an ignore mark
  if((attribute != L"" && attribute != direction)
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

  if(wsweight == L"")
  {
    wsweight = L"0.0000";
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

    int type = xmlTextReaderNodeType(reader);
    if(name == COMPILER_PAIR_ELEM)
    {
      elements.push_back(procTransduction(wsweight));
    }
    else if(name == COMPILER_IDENTITY_ELEM)
    {
      elements.push_back(procIdentity(wsweight, false));
    }
    else if(name == COMPILER_IDENTITYGROUP_ELEM)
    {
      elements.push_back(procIdentity(wsweight, true));
    }
    else if(name == COMPILER_REGEXP_ELEM)
    {
      elements.push_back(procRegexp());
    }
    else if(name == COMPILER_PAR_ELEM)
    {
      elements.push_back(procPar());

      // detection of the use of undefined paradigms

      wstring const &p = elements.rbegin()->paradigmName();

      if(paradigms.find(p) == paradigms.end())
      {
        wcerr << L"Error (" << xmlTextReaderGetParserLineNumber(reader);
        wcerr << L"): Undefined paradigm '" << p << L"'." <<endl;
        exit(EXIT_FAILURE);
      }
      // discard entries with empty paradigms (by the directions, normally)
      if(paradigms[p].isEmpty())
      {
        while(name != COMPILER_ENTRY_ELEM || type != XML_READER_TYPE_END_ELEMENT)
        {
          xmlTextReaderRead(reader);
          name = XMLParseUtil::towstring(xmlTextReaderConstName(reader));
          type = xmlTextReaderNodeType(reader);
        }
        return;
      }
    }
    else if(name == COMPILER_ENTRY_ELEM && type == XML_READER_TYPE_END_ELEMENT)
    {
      // insert elements into letter transducer
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
  xmlChar  const *xname = xmlTextReaderConstName(reader);
  wstring name = XMLParseUtil::towstring(xname);
  if(name == L"#text")
  {
    /* ignore */
  }
  else if(name == L"analysis-chars")
  {
    /* ignore */
  }
  else if(name == L"char")
  {
    acx_current_char = static_cast<int>(attrib(L"value")[0]);
  }
  else if(name == L"equiv-char")
  {
    acx_map[acx_current_char].insert(static_cast<int>(attrib(L"value")[0]));
  }
  else if(name == L"#comment")
  {
    /* ignore */
  }
  else
  {
    wcerr << L"Error in ACX file (" << xmlTextReaderGetParserLineNumber(reader);
    wcerr << L"): Invalid node '<" << name << L">'." << endl;
    exit(EXIT_FAILURE);
  }
}

void
Compiler::procNode()
{
  xmlChar const *xname = xmlTextReaderConstName(reader);
  wstring name = XMLParseUtil::towstring(xname);

  // TODO: optimize the execution order of the string "ifs"

  if(name == L"#text")
  {
    /* ignore */
  }
  else if(name == COMPILER_DICTIONARY_ELEM)
  {
    /* ignore */
  }
  else if(name == COMPILER_ALPHABET_ELEM)
  {
    procAlphabet();
  }
  else if(name == COMPILER_SDEFS_ELEM)
  {
    /* ignore */
  }
  else if(name == COMPILER_SDEF_ELEM)
  {
    procSDef();
  }
  else if(name == COMPILER_PARDEFS_ELEM)
  {
    /* ignore */
  }
  else if(name == COMPILER_PARDEF_ELEM)
  {
    procParDef();
  }
  else if(name == COMPILER_ENTRY_ELEM)
  {
    procEntry();
  }
  else if(name == COMPILER_SECTION_ELEM)
  {
    procSection();
  }
  else if(name== L"#comment")
  {
    /* ignore */
  }
  else
  {
    wcerr << L"Error (" << xmlTextReaderGetParserLineNumber(reader);
    wcerr << L"): Invalid node '<" << name << L">'." << endl;
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
  fwrite(HEADER_LTTOOLBOX, 1, 4, output);
  uint64_t features = 0;
  write_le(output, features);

  // letters
  Compression::wstring_write(letters, output);

  // symbols
  alphabet.write(output);

  // transducers
  Compression::multibyte_write(sections.size(), output);

  int count=0;
  for(auto& it : sections)
  {
    count++;
    wcout << it.first << " " << it.second.size();
    wcout << " " << it.second.numberOfTransitions() << endl;
    Compression::wstring_write(it.first, output);
    it.second.write(output);
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
