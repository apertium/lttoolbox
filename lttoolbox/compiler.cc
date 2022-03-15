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
#include <lttoolbox/compression.h>
#include <lttoolbox/endian_util.h>
#include <lttoolbox/entry_token.h>
#include <lttoolbox/lt_locale.h>
#include <lttoolbox/xml_parse_util.h>
#include <lttoolbox/string_utils.h>
#include <lttoolbox/string_writer.h>
#include <lttoolbox/file_utils.h>

#include <string>
#include <cstdlib>
#include <iostream>
#include <libxml/encoding.h>

using namespace std;

UString const Compiler::COMPILER_DICTIONARY_ELEM    = "dictionary"_u;
UString const Compiler::COMPILER_ALPHABET_ELEM      = "alphabet"_u;
UString const Compiler::COMPILER_SDEFS_ELEM         = "sdefs"_u;
UString const Compiler::COMPILER_SDEF_ELEM          = "sdef"_u;
UString const Compiler::COMPILER_N_ATTR             = "n"_u;
UString const Compiler::COMPILER_PARDEFS_ELEM       = "pardefs"_u;
UString const Compiler::COMPILER_PARDEF_ELEM        = "pardef"_u;
UString const Compiler::COMPILER_PAR_ELEM           = "par"_u;
UString const Compiler::COMPILER_ENTRY_ELEM         = "e"_u;
UString const Compiler::COMPILER_RESTRICTION_ATTR   = "r"_u;
UString const Compiler::COMPILER_RESTRICTION_LR_VAL = "LR"_u;
UString const Compiler::COMPILER_RESTRICTION_RL_VAL = "RL"_u;
UString const Compiler::COMPILER_PAIR_ELEM          = "p"_u;
UString const Compiler::COMPILER_LEFT_ELEM          = "l"_u;
UString const Compiler::COMPILER_RIGHT_ELEM         = "r"_u;
UString const Compiler::COMPILER_S_ELEM             = "s"_u;
UString const Compiler::COMPILER_M_ELEM             = "m"_u;
UString const Compiler::COMPILER_REGEXP_ELEM        = "re"_u;
UString const Compiler::COMPILER_SECTION_ELEM       = "section"_u;
UString const Compiler::COMPILER_ID_ATTR            = "id"_u;
UString const Compiler::COMPILER_TYPE_ATTR          = "type"_u;
UString const Compiler::COMPILER_IDENTITY_ELEM      = "i"_u;
UString const Compiler::COMPILER_IDENTITYGROUP_ELEM = "ig"_u;
UString const Compiler::COMPILER_JOIN_ELEM          = "j"_u;
UString const Compiler::COMPILER_BLANK_ELEM         = "b"_u;
UString const Compiler::COMPILER_POSTGENERATOR_ELEM = "a"_u;
UString const Compiler::COMPILER_GROUP_ELEM         = "g"_u;
UString const Compiler::COMPILER_LEMMA_ATTR         = "lm"_u;
UString const Compiler::COMPILER_IGNORE_ATTR        = "i"_u;
UString const Compiler::COMPILER_IGNORE_YES_VAL     = "yes"_u;
UString const Compiler::COMPILER_ALT_ATTR           = "alt"_u;
UString const Compiler::COMPILER_V_ATTR             = "v"_u;
UString const Compiler::COMPILER_VL_ATTR            = "vl"_u;
UString const Compiler::COMPILER_VR_ATTR            = "vr"_u;
UString const Compiler::COMPILER_WEIGHT_ATTR        = "w"_u;
UString const Compiler::COMPILER_TEXT_NODE          = "#text"_u;
UString const Compiler::COMPILER_COMMENT_NODE       = "#comment"_u;
UString const Compiler::COMPILER_ACX_ANALYSIS_ELEM  = "analysis-chars"_u;
UString const Compiler::COMPILER_ACX_CHAR_ELEM      = "char"_u;
UString const Compiler::COMPILER_ACX_EQUIV_CHAR_ELEM= "equiv-char"_u;
UString const Compiler::COMPILER_ACX_VALUE_ATTR     = "value"_u;
UString const Compiler::COMPILER_LSX_WB_ELEM        = "d"_u;
UString const Compiler::COMPILER_LSX_CHAR_ELEM      = "w"_u;
UString const Compiler::COMPILER_LSX_TAG_ELEM       = "t"_u;

Compiler::Compiler()
{
}

Compiler::~Compiler()
{
}

void
Compiler::parseACX(string const &file, UString const &dir)
{
  if(dir == COMPILER_RESTRICTION_LR_VAL)
  {
    reader = xmlReaderForFile(file.c_str(), NULL, 0);
    if(reader == NULL)
    {
      cerr << "Error: cannot open '" << file << "'." << endl;
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
Compiler::parse(string const &file, UString const &dir)
{
  direction = dir;
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


  // Minimize transducers
  for(auto& it : sections)
  {
    it.second.minimize();
  }

  if (!valid(dir)) {
    exit(EXIT_FAILURE);
  }
}

bool
Compiler::valid(UString const& dir) const
{
  const char* side = dir == COMPILER_RESTRICTION_RL_VAL ? "right" : "left";
  const set<int> epsilonSymbols = alphabet.symbolsWhereLeftIs(0);
  const set<int> spaceSymbols = alphabet.symbolsWhereLeftIs(' ');
  for (auto &section : sections) {
    auto &fst = section.second;
    auto finals = fst.getFinals();
    auto initial = fst.getInitial();
    for(const auto i : fst.closure(initial, epsilonSymbols)) {
      if (finals.count(i)) {
        cerr << "Error: Invalid dictionary (hint: the " << side << " side of an entry is empty)" << endl;
        return false;
      }
      if(fst.closure(i, spaceSymbols).size() > 1) { // >1 since closure always includes self
        cerr << "Error: Invalid dictionary (hint: entry on the " << side << " beginning with whitespace)" << endl;
        return false;
      }
    }
  }
  return true;
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
      letters = XMLParseUtil::readValue(reader);
      bool space = true;
      for(unsigned int i = 0; i < letters.length(); i++)
      {
        if(!u_isspace(letters[i]))
        {
          space = false;
          break;
        }
      }
      if(space == true)  // libxml2 returns '\n' for <alphabet></alphabet>, should be empty
      {
        letters.clear();
      }
    }
    else
    {
      cerr << "Error (" << xmlTextReaderGetParserLineNumber(reader);
      cerr << "): Missing alphabet symbols." << endl;
      exit(EXIT_FAILURE);
    }
  }
}

void
Compiler::procSDef()
{
  alphabet.includeSymbol("<"_u + attrib(COMPILER_N_ATTR) + ">"_u);
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
      current_paradigm.clear();
    }
  }
}

int
Compiler::matchTransduction(vector<int> const &pi,
                           vector<int> const &pd,
                           int state, Transducer &t,
                           double const &entry_weight)
{
  vector<int>::const_iterator left, right, limleft, limright;

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
Compiler::requireEmptyError(UString const &name)
{
  if(!xmlTextReaderIsEmptyElement(reader))
  {
    cerr << "Error (" << xmlTextReaderGetParserLineNumber(reader);
    cerr << "): Non-empty element '<" << name << ">' should be empty." << endl;
    exit(EXIT_FAILURE);
  }
}

bool
Compiler::allBlanks()
{
  bool flag = true;
  UString text = XMLParseUtil::readValue(reader);

  for(auto c : text)
  {
    flag = flag && u_isspace(c);
  }

  return flag;
}

void
Compiler::readString(vector<int> &result, UString const &name)
{
  if(name == COMPILER_TEXT_NODE)
  {
    XMLParseUtil::readValueInto32(reader, result);
  }
  else if(name == COMPILER_M_ELEM)
  {
    requireEmptyError(name);
    if(keep_boundaries)
    {
      result.push_back(static_cast<int>('>'));
    }
  }
  else if(name == COMPILER_BLANK_ELEM)
  {
    requireEmptyError(name);
    result.push_back(static_cast<int>(' '));
  }
  else if(name == COMPILER_JOIN_ELEM)
  {
    requireEmptyError(name);
    result.push_back(static_cast<int>('+'));
  }
  else if(name == COMPILER_POSTGENERATOR_ELEM)
  {
    requireEmptyError(name);
    result.push_back(static_cast<int>('~'));
  }
  else if(name == COMPILER_GROUP_ELEM)
  {
    int type=xmlTextReaderNodeType(reader);
    if(type != XML_READER_TYPE_END_ELEMENT)
    {
      result.push_back(static_cast<int>('#'));
    }
  }
  else if(name == COMPILER_S_ELEM)
  {
    requireEmptyError(name);
    UString symbol = "<"_u + attrib(COMPILER_N_ATTR) + ">"_u;

    if(!alphabet.isSymbolDefined(symbol))
    {
      cerr << "Error (" << xmlTextReaderGetParserLineNumber(reader);
      cerr << "): Undefined symbol '" << symbol << "'." << endl;
      exit(EXIT_FAILURE);
    }

    result.push_back(alphabet(symbol));
  }
  else
  {
    cerr << "Error (" << xmlTextReaderGetParserLineNumber(reader);
    cerr << "): Invalid specification of element '<" << name;
    cerr << ">' in this context." << endl;
    exit(EXIT_FAILURE);
  }
}

void
Compiler::skipBlanks(UString &name)
{
  while(name == COMPILER_TEXT_NODE || name == COMPILER_COMMENT_NODE)
  {
    if(name != COMPILER_COMMENT_NODE)
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
Compiler::skip(UString &name, UString const &elem)
{
  skip(name, elem, true);
}

void
Compiler::skip(UString &name, UString const &elem, bool open)
{
  xmlTextReaderRead(reader);
  name = XMLParseUtil::readName(reader);
  UString slash;

  if(!open)
  {
    slash = "/"_u;
  }

  while(name == COMPILER_TEXT_NODE || name == COMPILER_COMMENT_NODE)
  {
    if(name != COMPILER_COMMENT_NODE)
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
    cerr << "): Expected '<" << slash << elem << ">'." << endl;
    exit(EXIT_FAILURE);
  }
}

EntryToken
Compiler::procIdentity(double const entry_weight, bool ig)
{
  vector<int> both_sides;

  if(!xmlTextReaderIsEmptyElement(reader))
  {
    UString name;

    while(true)
    {
      xmlTextReaderRead(reader);
      name = XMLParseUtil::readName(reader);
      if(name == COMPILER_IDENTITY_ELEM || name == COMPILER_IDENTITYGROUP_ELEM)
      {
        break;
      }
      readString(both_sides, name);
    }
  }

  if(verbose && first_element && (both_sides.front() == (int)' '))
  {
    cerr << "Error (" << xmlTextReaderGetParserLineNumber(reader);
    cerr << "): Entry begins with space." << endl;
  }
  first_element = false;
  EntryToken e;
  if(ig)
  {
    vector<int> right;
    right.push_back(static_cast<int>('#'));
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
Compiler::procTransduction(double const entry_weight)
{
  vector<int> lhs, rhs;
  UString name;

  skip(name, COMPILER_LEFT_ELEM);

  if(!xmlTextReaderIsEmptyElement(reader))
  {
    name.clear();
    while(true)
    {
      xmlTextReaderRead(reader);
      name = XMLParseUtil::readName(reader);
      if(name == COMPILER_LEFT_ELEM)
      {
        break;
      }
      readString(lhs, name);
    }
  }

  if(verbose && first_element && (lhs.front() == (int)' '))
  {
    cerr << "Error (" << xmlTextReaderGetParserLineNumber(reader);
    cerr << "): Entry begins with space." << endl;
  }
  first_element = false;

  skip(name, COMPILER_RIGHT_ELEM);

  if(!xmlTextReaderIsEmptyElement(reader))
  {
    name.clear();
    while(true)
    {
      xmlTextReaderRead(reader);
      name = XMLParseUtil::readName(reader);
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

UString
Compiler::attrib(UString const &name)
{
  return XMLParseUtil::attrib(reader, name);
}

EntryToken
Compiler::procPar()
{
  EntryToken e;
  UString paradigm_name = attrib(COMPILER_N_ATTR);
  first_element = false;

  if(!current_paradigm.empty() && paradigm_name == current_paradigm)
  {
    cerr << "Error (" << xmlTextReaderGetParserLineNumber(reader);
    cerr << "): Paradigm refers to itself '" << paradigm_name << "'." <<endl;
    exit(EXIT_FAILURE);
  }

  if(paradigms.find(paradigm_name) == paradigms.end())
  {
    cerr << "Error (" << xmlTextReaderGetParserLineNumber(reader);
    cerr << "): Undefined paradigm '" << paradigm_name << "'." << endl;
    exit(EXIT_FAILURE);
  }
  e.setParadigm(paradigm_name);
  return e;
}

void
Compiler::insertEntryTokens(vector<EntryToken> const &elements)
{
  if(!current_paradigm.empty())
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
        cerr << "Error (" << xmlTextReaderGetParserLineNumber(reader);
        cerr << "): Invalid entry token." << endl;
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
Compiler::requireAttribute(UString const &value, UString const &attrname,
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


void
Compiler::procSection()
{
  int type=xmlTextReaderNodeType(reader);

  if(type != XML_READER_TYPE_END_ELEMENT)
  {
    UString const &id = attrib(COMPILER_ID_ATTR);
    UString const &type = attrib(COMPILER_TYPE_ATTR);
    requireAttribute(id, COMPILER_ID_ATTR, COMPILER_SECTION_ELEM);
    requireAttribute(type, COMPILER_TYPE_ATTR, COMPILER_SECTION_ELEM);

    current_section = id;
    current_section += '@';
    current_section.append(type);
  }
  else
  {
    current_section.clear();
  }
}

void
Compiler::procEntry()
{
  UString attribute = this->attrib(COMPILER_RESTRICTION_ATTR);
  UString ignore    = this->attrib(COMPILER_IGNORE_ATTR);
  UString altval    = this->attrib(COMPILER_ALT_ATTR);
  UString varval    = this->attrib(COMPILER_V_ATTR);
  UString varl      = this->attrib(COMPILER_VL_ATTR);
  UString varr      = this->attrib(COMPILER_VR_ATTR);
  UString wsweight  = this->attrib(COMPILER_WEIGHT_ATTR);

  // if entry is masked by a restriction of direction or an ignore mark
  if((!attribute.empty() && attribute != direction)
   || ignore == COMPILER_IGNORE_YES_VAL
   || (!altval.empty() && altval != alt)
   || (direction == COMPILER_RESTRICTION_RL_VAL && !varval.empty() && varval != variant)
   || (direction == COMPILER_RESTRICTION_RL_VAL && !varl.empty() && varl != variant_left)
   || (direction == COMPILER_RESTRICTION_LR_VAL && !varr.empty() && varr != variant_right))
  {
    // parse to the end of the entry
    UString name;

    while(name != COMPILER_ENTRY_ELEM)
    {
      xmlTextReaderRead(reader);
      name = XMLParseUtil::readName(reader);
    }

    return;
  }

  double weight = 0.0;
  if(!wsweight.empty())
  {
    weight = StringUtils::stod(wsweight);
  }

  vector<EntryToken> elements;

  while(true)
  {
    int ret = xmlTextReaderRead(reader);
    if(ret != 1)
    {
      cerr << "Error (" << xmlTextReaderGetParserLineNumber(reader);
      cerr << "): Parse error." << endl;
      exit(EXIT_FAILURE);
    }
    UString name = XMLParseUtil::readName(reader);
    skipBlanks(name);

    if(current_paradigm.empty() && verbose)
    {
      first_element = true;
    }

    int type = xmlTextReaderNodeType(reader);
    if(name == COMPILER_PAIR_ELEM)
    {
      elements.push_back(procTransduction(weight));
    }
    else if(name == COMPILER_IDENTITY_ELEM)
    {
      elements.push_back(procIdentity(weight, false));
    }
    else if(name == COMPILER_IDENTITYGROUP_ELEM)
    {
      elements.push_back(procIdentity(weight, true));
    }
    else if(name == COMPILER_REGEXP_ELEM)
    {
      elements.push_back(procRegexp());
    }
    else if(name == COMPILER_PAR_ELEM)
    {
      elements.push_back(procPar());

      // detection of the use of undefined paradigms

      UString const &p = elements.rbegin()->paradigmName();

      if(paradigms.find(p) == paradigms.end())
      {
        cerr << "Error (" << xmlTextReaderGetParserLineNumber(reader);
        cerr << "): Undefined paradigm '" << p << "'." <<endl;
        exit(EXIT_FAILURE);
      }
      // discard entries with empty paradigms (by the directions, normally)
      if(paradigms[p].isEmpty())
      {
        while(name != COMPILER_ENTRY_ELEM || type != XML_READER_TYPE_END_ELEMENT)
        {
          xmlTextReaderRead(reader);
          name = XMLParseUtil::readName(reader);
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
    else if(name == COMPILER_TEXT_NODE && allBlanks())
    {
    }
    else
    {
      cerr << "Error (" << xmlTextReaderGetParserLineNumber(reader);
      cerr << "): Invalid inclusion of '<" << name << ">' into '<" << COMPILER_ENTRY_ELEM;
      cerr << ">'." << endl;
      exit(EXIT_FAILURE);
    }
  }
}

void
Compiler::procNodeACX()
{
  UString name = XMLParseUtil::readName(reader);
  if(name == COMPILER_TEXT_NODE)
  {
    /* ignore */
  }
  else if(name == COMPILER_ACX_ANALYSIS_ELEM)
  {
    /* ignore */
  }
  else if(name == COMPILER_ACX_CHAR_ELEM)
  {
    acx_current_char = static_cast<int>(attrib(COMPILER_ACX_VALUE_ATTR)[0]);
  }
  else if(name == COMPILER_ACX_EQUIV_CHAR_ELEM)
  {
    acx_map[acx_current_char].insert(static_cast<int>(attrib(COMPILER_ACX_VALUE_ATTR)[0]));
  }
  else if(name == COMPILER_COMMENT_NODE)
  {
    /* ignore */
  }
  else
  {
    cerr << "Error in ACX file (" << xmlTextReaderGetParserLineNumber(reader);
    cerr << "): Invalid node '<" << name << ">'." << endl;
    exit(EXIT_FAILURE);
  }
}

void
Compiler::procNode()
{
  UString name = XMLParseUtil::readName(reader);

  // TODO: optimize the execution order of the string "ifs"

  if(name == COMPILER_TEXT_NODE)
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
  else if(name== COMPILER_COMMENT_NODE)
  {
    /* ignore */
  }
  else
  {
    cerr << "Error (" << xmlTextReaderGetParserLineNumber(reader);
    cerr << "): Invalid node '<" << name << ">'." << endl;
    exit(EXIT_FAILURE);
  }
}

EntryToken
Compiler::procRegexp()
{
  EntryToken et;
  xmlTextReaderRead(reader);
  et.readRegexp(reader);
  xmlTextReaderRead(reader);
  return et;
}

void
Compiler::write(FILE *output)
{
  writeTransducerSet(output, letters, alphabet, sections);
}

void
Compiler::setAltValue(UString const &a)
{
  alt = a;
}

void
Compiler::setVariantValue(UString const &v)
{
  variant = v;
}

void
Compiler::setVariantLeftValue(UString const &v)
{
  variant_left = v;
}

void
Compiler::setVariantRightValue(UString const &v)
{
  variant_right = v;
}

void
Compiler::setKeepBoundaries(bool keep)
{
  keep_boundaries = keep;
}

void
Compiler::setVerbose(bool verbosity)
{
  verbose = verbosity;
}
