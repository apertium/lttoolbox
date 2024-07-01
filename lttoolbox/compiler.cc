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
#include <lttoolbox/xml_parse_util.h>
#include <lttoolbox/string_utils.h>
#include <lttoolbox/file_utils.h>
#include <lttoolbox/acx.h>
#include <lttoolbox/regexp_compiler.h>

#include <iostream>
#include <thread>

Compiler::Compiler()
{
}

Compiler::~Compiler()
{
}

void
Compiler::parseACX(std::string const &file, UStringView dir)
{
  if(dir == COMPILER_RESTRICTION_LR_VAL)
  {
    acx_map = readACX(file.c_str());
  }
}

void
Compiler::parse(std::string const &file, UStringView dir)
{
  if (dir == COMPILER_RESTRICTION_U_VAL) {
    direction = COMPILER_RESTRICTION_LR_VAL;
    unified_compilation = true;
  } else {
    direction = dir;
  }
  reader = XMLParseUtil::open_or_exit(file.c_str());

  int ret = xmlTextReaderRead(reader);
  while(ret == 1)
  {
    procNode();
    ret = xmlTextReaderRead(reader);
  }

  if(ret != 0)
  {
    std::cerr << "Error: Parse error at the end of input." << std::endl;
  }

  xmlFreeTextReader(reader);
  xmlCleanupParser();


  // Minimize transducers: For each section, call transducer.minimize() in
  // its own thread. This is the major bottleneck of lt-comp and sections
  // are completely independent transducers.
  std::vector<std::thread> minimisations;
  for(auto& it : sections)
  {
    if(jobs) {
      minimisations.push_back(
        std::thread([](Transducer &t) { t.minimize(); },
                    std::ref(it.second)));
    }
    else {
      it.second.minimize();
    }
  }
  for (auto &thr : minimisations) {
    thr.join();
  }

  if (is_separable) {
    // ensure that all paths end in <$>, in case the user forgot to include
    // <d/>. This will result in some paths ending with multiple finals
    // and multiple finals, but lsx-proc only checks for finals upon reading
    // $, so it won't be an issue.
    int32_t end = alphabet(word_boundary, word_boundary);
    for (auto& it : sections) {
      for (auto fin : it.second.getFinals()) {
        int end_state = it.second.insertSingleTransduction(end, fin.first);
        it.second.setFinal(end_state);
      }
    }
  }

  if (!valid(dir)) {
    exit(EXIT_FAILURE);
  }
}

bool
Compiler::valid(UStringView dir) const
{
  const char* side = (dir == COMPILER_RESTRICTION_RL_VAL ? "right" : "left");
  const std::set<int> epsilonSymbols = alphabet.symbolsWhereLeftIs(0);
  const std::set<int> spaceSymbols = alphabet.symbolsWhereLeftIs(' ');
  for (auto &section : sections) {
    auto &fst = section.second;
    auto finals = fst.getFinals();
    auto initial = fst.getInitial();
    for(const auto i : fst.closure(initial, epsilonSymbols)) {
      if (finals.count(i)) {
        std::cerr << "Error: Invalid dictionary (hint: the " << side << " side of an entry is empty)" << std::endl;
        return false;
      }
      if(fst.closure(i, spaceSymbols).size() > 1) { // >1 since closure always includes self
        std::cerr << "Error: Invalid dictionary (hint: entry on the " << side << " beginning with whitespace)" << std::endl;
        return false;
      }
    }
  }
  return true;
}

void
Compiler::step(UString& name)
{
  int ret = xmlTextReaderRead(reader);
  if (ret == -1) {
    XMLParseUtil::error_and_die(reader, "Parse error.");
  }
  name = XMLParseUtil::readName(reader);
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
        if(!u_isspace(letters.at(i)))
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
      XMLParseUtil::error_and_die(reader, "Missing alphabet symbols.");
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
Compiler::matchTransduction(std::vector<int> const &pi,
                           std::vector<int> const &pd,
                           int state, Transducer &t,
                           double const &entry_weight)
{
  std::vector<int>::const_iterator left, right, limleft, limright;

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
    std::map<int32_t, sorted_vector<int32_t> >::iterator acx_map_ptr;
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

      if (is_separable) {
        // loop-back symbols for <ANY_TAG> and <ANY_CHAR>
        if (tag == alphabet(0, any_tag) || tag == alphabet(0, any_char)) {
          // rl compilation of a badly written rule
          // having an epsilon with wildcard output will produce
          // garbage output -- see https://github.com/apertium/apertium-separable/issues/8
          std::cerr << "Warning: Cannot insert <t/> from empty input. Ignoring. (You probably want to specify exact tags when deleting a word.)" << std::endl;
        } else if (tag == alphabet(any_tag, any_tag) ||
                   tag == alphabet(any_char, any_char) ||
                   tag == alphabet(any_tag, 0) ||
                   tag == alphabet(any_char, 0)) {
          t.linkStates(new_state, new_state, tag);
        }
      }

      if(acx_map_ptr != acx_map.end())
      {
        for(auto& it : acx_map_ptr->second)
        {
          t.linkStates(state, new_state, alphabet(it, rsymbol), weight_value);
        }
      }
      state = new_state;
    }
  }

  return state;
}


void
Compiler::requireEmptyError(UStringView name)
{
  if(!xmlTextReaderIsEmptyElement(reader))
  {
    std::cerr << "Error (" << xmlTextReaderGetParserLineNumber(reader);
    std::cerr << "): Non-empty element '<" << name << ">' should be empty." << std::endl;
    exit(EXIT_FAILURE);
  }
}

bool
Compiler::allBlanks()
{
  return XMLParseUtil::allBlanks(reader);
}

void
Compiler::readString(std::vector<int> &result, UStringView name)
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
      XMLParseUtil::error_and_die(reader, "Undefined symbol '%S'.", symbol.c_str());
    }

    result.push_back(alphabet(symbol));
  }
  else if (is_separable && name == COMPILER_LSX_TAG_ELEM) {
    requireEmptyError(name);
    result.push_back(any_tag);
  }
  else if (is_separable && name == COMPILER_LSX_CHAR_ELEM) {
    requireEmptyError(name);
    result.push_back(any_char);
  }
  else if (is_separable && name == COMPILER_LSX_WB_ELEM) {
    requireEmptyError(name);
    UString mode = attrib(COMPILER_LSX_SPACE_ATTR);
    if (mode == COMPILER_LSX_SPACE_YES_VAL) {
      result.push_back(word_boundary_s);
    } else if (mode == COMPILER_LSX_SPACE_NO_VAL) {
      result.push_back(word_boundary_ns);
    } else {
      result.push_back(word_boundary);
    }
  }
  else if (is_separable && name == COMPILER_LSX_FORM_SEP_ELEM) {
    requireEmptyError(name);
    result.push_back(reading_boundary);
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
Compiler::skipBlanks(UString &name)
{
  while(name == COMPILER_TEXT_NODE || name == COMPILER_COMMENT_NODE)
  {
    if(name != COMPILER_COMMENT_NODE)
    {
      if(!allBlanks())
      {
	XMLParseUtil::error_and_die(reader, "Invalid construction.");
      }
    }

    step(name);
  }
}

void
Compiler::skip(UString &name, UStringView elem, bool open)
{
  step(name);
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
	XMLParseUtil::error_and_die(reader, "Invalid construction.");
      }
    }
    step(name);
  }

  if(name != elem)
  {
    std::cerr << "Error (" << xmlTextReaderGetParserLineNumber(reader);
    std::cerr << "): Expected '<" << slash << elem << ">'." << std::endl;
    exit(EXIT_FAILURE);
  }
}

EntryToken
Compiler::procIdentity(double const entry_weight, bool ig)
{
  std::vector<int> both_sides;

  if(!xmlTextReaderIsEmptyElement(reader))
  {
    UString name;

    while(true)
    {
      step(name);
      if(name == COMPILER_IDENTITY_ELEM || name == COMPILER_IDENTITYGROUP_ELEM)
      {
        break;
      }
      readString(both_sides, name);
    }
  }

  if(verbose && first_element && (both_sides.front() == (int)' '))
  {
    std::cerr << "Error (" << xmlTextReaderGetParserLineNumber(reader);
    std::cerr << "): Entry begins with space." << std::endl;
  }
  first_element = false;
  EntryToken e;
  if(ig)
  {
    std::vector<int> right;
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
  std::vector<int> lhs, rhs;
  UString name;

  skip(name, COMPILER_LEFT_ELEM);

  if(!xmlTextReaderIsEmptyElement(reader))
  {
    while (true) {
      step(name);
      if (name == COMPILER_LEFT_ELEM) break;
      readString(lhs, name);
    }
  }

  if(verbose && first_element && (lhs.front() == (int)' '))
  {
    std::cerr << "Error (" << xmlTextReaderGetParserLineNumber(reader);
    std::cerr << "): Entry begins with space." << std::endl;
  }
  first_element = false;

  skip(name, COMPILER_RIGHT_ELEM);

  if(!xmlTextReaderIsEmptyElement(reader))
  {
    while (true) {
      step(name);
      if (name == COMPILER_RIGHT_ELEM) break;
      readString(rhs, name);
    }
  }

  skip(name, COMPILER_PAIR_ELEM, false);

  EntryToken e;
  e.setSingleTransduction(lhs, rhs, entry_weight);
  return e;
}

UString
Compiler::attrib(UStringView name)
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
    XMLParseUtil::error_and_die(reader, "Paradigm refers to itself '%S'.", paradigm_name.c_str());
  }

  if(paradigms.find(paradigm_name) == paradigms.end())
  {
    XMLParseUtil::error_and_die(reader, "Undefined paradigm '%S'.", paradigm_name.c_str());
  }
  e.setParadigm(paradigm_name);
  return e;
}

void
Compiler::insertEntryTokens(std::vector<EntryToken> const &elements)
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
	XMLParseUtil::error_and_die(reader, "Invalid entry token.");
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
Compiler::requireAttribute(UStringView value, UStringView attrname, UStringView elemname)
{
  if(value.empty())
  {
    std::cerr << "Error (" << xmlTextReaderGetParserLineNumber(reader);
    std::cerr << "): '<" << elemname;
    std::cerr << "' element must specify non-void '";
    std::cerr << attrname << "' attribute." << std::endl;
    exit(EXIT_FAILURE);
  }
}


void
Compiler::procSection()
{
  int type=xmlTextReaderNodeType(reader);

  if(type != XML_READER_TYPE_END_ELEMENT)
  {
    const auto& id = attrib(COMPILER_ID_ATTR);
    const auto& type = attrib(COMPILER_TYPE_ATTR);
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

bool
Compiler::filterEntry(UStringView value, UStringView filter, bool keep_on_empty_filter)
{
  if (value.empty()) return true;
  else if (keep_on_empty_filter && filter.empty()) return true;
  auto ops = StringUtils::split(value, u" ");
  for (auto& it : ops) {
    if (it == filter) return true;
  }
  return false;
}

void
Compiler::symbolFilters(UStringView value, UStringView prefix, std::vector<std::vector<int32_t>>& symbols)
{
  if (value.empty()) return;
  std::vector<int32_t> syms;
  for (auto& it : StringUtils::split(value, u" ")) {
    if (it.empty()) continue;
    UString tag;
    tag += '<';
    tag += prefix;
    tag += ':';
    tag += it;
    tag += '>';
    alphabet.includeSymbol(tag);
    syms.push_back(alphabet(tag));
  }
  if (!syms.empty()) symbols.push_back(syms);
}

void
Compiler::procEntry()
{
  UString attribute = attrib(COMPILER_RESTRICTION_ATTR);
  UString ignore    = attrib(COMPILER_IGNORE_ATTR);
  UString altval    = attrib(COMPILER_ALT_ATTR);
  UString varval    = attrib(COMPILER_V_ATTR);
  UString varl      = attrib(COMPILER_VL_ATTR);
  UString varr      = attrib(COMPILER_VR_ATTR);
  UString wsweight  = attrib(COMPILER_WEIGHT_ATTR);

  std::vector<EntryToken> elements;

  // if entry is masked by a restriction of direction or an ignore mark
  if (unified_compilation && ignore != COMPILER_IGNORE_YES_VAL) {
    std::vector<std::vector<int32_t>> symbols;
    symbolFilters(attribute, u"r", symbols);
    symbolFilters(altval, u"alt", symbols);
    symbolFilters(varval, u"v", symbols);
    symbolFilters(varl, u"vl", symbols);
    symbolFilters(varr, u"vr", symbols);
    if (!symbols.empty()) {
      bool multi = false;
      for (auto& it : symbols) {
        if (it.size() > 1) {
          multi = true;
          break;
        }
      }
      if (multi) {
        UString parname = "--"_u;
        parname += attribute;
        parname += '-';
        parname += altval;
        parname += '-';
        parname += varval;
        parname += '-';
        parname += varl;
        parname += '-';
        parname += varr;
        if (paradigms.find(parname) == paradigms.end()) {
          std::vector<int32_t> re;
          for (auto& it : symbols) {
            if (it.size() == 1) {
              re.push_back(it[0]);
            } else {
              re.push_back(static_cast<int32_t>('['));
              re.insert(re.end(), it.begin(), it.end());
              re.push_back(static_cast<int32_t>(']'));
            }
          }
          EntryToken e;
          e.setRegexp(re);
          std::vector<EntryToken> vec(1, e);
          parname.swap(current_paradigm);
          insertEntryTokens(vec);
          parname.swap(current_paradigm);
        }
        EntryToken e;
        e.setParadigm(parname);
        elements.push_back(e);
      }
      else {
        std::vector<int> syms;
        for (auto& it : symbols) {
          syms.push_back(it[0]);
        }
        EntryToken e;
        e.setSingleTransduction(syms, syms);
        elements.push_back(e);
      }
    }
  }
  else if((!attribute.empty() && attribute != direction)
   || ignore == COMPILER_IGNORE_YES_VAL
   || !filterEntry(altval, alt, false)
   || !filterEntry(varval, variant, true)
   || (direction == COMPILER_RESTRICTION_RL_VAL && !filterEntry(varl, variant_left, false))
   || (direction == COMPILER_RESTRICTION_LR_VAL && !filterEntry(varr, variant_right, false)))
  {
    // parse to the end of the entry
    UString name;

    while(name != COMPILER_ENTRY_ELEM)
    {
      step(name);
    }

    return;
  }

  double weight = 0.0;
  if(!wsweight.empty())
  {
    weight = StringUtils::stod(wsweight);
  }

  if (entry_debugging && current_paradigm.empty()) {
    UString ln = "Line near "_u;
    ln += StringUtils::itoa(xmlTextReaderGetParserLineNumber(reader));
    // Note that this line number will usually be a little bit wrong.
    // This function actually returns the current line of the *parser*
    // which is probably several lines past the element we're currently
    // looking at.
    UString c = attrib(u"c");
    if (!c.empty()) {
      ln += ' ';
      ln += c;
    }
    std::vector<int32_t> empty;
    std::vector<int32_t> debug_syms;
    ustring_to_vec32(ln, debug_syms);
    if (is_separable) {
      debug_syms.push_back(word_boundary_s);
    } else {
      debug_syms.push_back(static_cast<int32_t>(' '));
    }
    EntryToken e;
    e.setSingleTransduction(empty, debug_syms);
    elements.push_back(e);
  }

  bool contentful = !xmlTextReaderIsEmptyElement(reader);
  // do nothing if it's <e/>, but while(true) otherwise
  while (contentful) {
    UString name;
    step(name);
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

      const auto& p = elements.rbegin()->paradigmName();

      auto it = paradigms.find(p);
      if(it == paradigms.end())
      {
	XMLParseUtil::error_and_die(reader, "Undefined paradigm '%S'.", p.c_str());
      }
      // discard entries with empty paradigms (by the directions, normally)
      if(it->second.isEmpty())
      {
        while(name != COMPILER_ENTRY_ELEM || type != XML_READER_TYPE_END_ELEMENT)
        {
          step(name);
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
      XMLParseUtil::error_and_die(reader, "Invalid inclusion of '<%S>' into '<e>'.", name.c_str());
    }
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
    if (attrib(COMPILER_TYPE_ATTR) == COMPILER_SEPARABLE_VAL ||
        attrib(COMPILER_TYPE_ATTR) == COMPILER_SEQUENTIAL_VAL) {
      is_separable = true;
      alphabet.includeSymbol(Transducer::ANY_TAG_SYMBOL);
      alphabet.includeSymbol(Transducer::ANY_CHAR_SYMBOL);
      alphabet.includeSymbol(Transducer::LSX_BOUNDARY_SYMBOL);
      alphabet.includeSymbol(Transducer::LSX_BOUNDARY_SPACE_SYMBOL);
      alphabet.includeSymbol(Transducer::LSX_BOUNDARY_NO_SPACE_SYMBOL);
      alphabet.includeSymbol(Transducer::READING_SEPARATOR_SYMBOL);
      any_tag          = alphabet(Transducer::ANY_TAG_SYMBOL);
      any_char         = alphabet(Transducer::ANY_CHAR_SYMBOL);
      word_boundary    = alphabet(Transducer::LSX_BOUNDARY_SYMBOL);
      word_boundary_s  = alphabet(Transducer::LSX_BOUNDARY_SPACE_SYMBOL);
      word_boundary_ns = alphabet(Transducer::LSX_BOUNDARY_NO_SPACE_SYMBOL);
      reading_boundary = alphabet(Transducer::READING_SEPARATOR_SYMBOL);
    }
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
    if(current_paradigm.empty()) {
      n_section_entries++;
      if(max_section_entries >0 && n_section_entries % max_section_entries == 0) {
        current_section = "+"_u + current_section; // would be invalid as xml id -- this way we won't clobber existing names
      }
    }
    procEntry();
  }
  else if(name == COMPILER_SECTION_ELEM)
  {
    n_section_entries = 0;
    procSection();
  }
  else if(name== COMPILER_COMMENT_NODE)
  {
    /* ignore */
  }
  else
  {
    XMLParseUtil::error_and_die(reader, "Invalid node '<%S>'.", name.c_str());
  }
}

EntryToken
Compiler::procRegexp()
{
  EntryToken et;
  UString name;
  step(name);
  if (name != COMPILER_TEXT_NODE) {
    XMLParseUtil::error_and_die(reader, "Invalid inclusion of '<%S>' in '<re>'.", name.c_str());
  }
  et.readRegexp(reader);
  step(name);
  if (name != COMPILER_REGEXP_ELEM) {
    XMLParseUtil::error_and_die(reader, "Invalid inclusion of '<%S>' in '<re>'.", name.c_str());
  }
  return et;
}

void
Compiler::write(FILE *output)
{
  writeTransducerSet(output, letters, alphabet, sections);
}

void
Compiler::setAltValue(UStringView a)
{
  alt = a;
}

void
Compiler::setVariantValue(UStringView v)
{
  variant = v;
}

void
Compiler::setVariantLeftValue(UStringView v)
{
  variant_left = v;
}

void
Compiler::setVariantRightValue(UStringView v)
{
  variant_right = v;
}

void
Compiler::setKeepBoundaries(bool keep)
{
  keep_boundaries = keep;
}

void
Compiler::setJobs(bool j)
{
  jobs = j;
}

void
Compiler::setMaxSectionEntries(size_t m)
{
  max_section_entries = m;
}

void
Compiler::setVerbose(bool verbosity)
{
  verbose = verbosity;
}

void
Compiler::setEntryDebugging(bool debug)
{
  entry_debugging = debug;
}
