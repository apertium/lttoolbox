/*
 * Copyright (C) 2005-2019 Universitat d'Alacant / Universidad de Alicante
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
#include <lttoolbox/fst_processor.h>
#include <lttoolbox/compression.h>
#include <lttoolbox/exception.h>
#include <lttoolbox/xml_parse_util.h>
#include <lttoolbox/file_utils.h>
#include <lttoolbox/stream_reader.h>
#include <lttoolbox/string_utils.h>
#include <lttoolbox/symbol_iter.h>

#include <iostream>
#include <cerrno>
#include <climits>


FSTProcessor::FSTProcessor()
{
  // escaped_chars chars
  escaped_chars.insert('[');
  escaped_chars.insert(']');
  escaped_chars.insert('{');
  escaped_chars.insert('}');
  escaped_chars.insert('^');
  escaped_chars.insert('$');
  escaped_chars.insert('/');
  escaped_chars.insert('\\');
  escaped_chars.insert('@');
  escaped_chars.insert('<');
  escaped_chars.insert('>');

  if(useDefaultIgnoredChars)
  {
    initDefaultIgnoredCharacters();
  }
}

void
FSTProcessor::streamError()
{
  throw Exception("Error: Malformed input stream.");
}

void
FSTProcessor::maybeFlush(UFILE* output, bool at_null)
{
  if (at_null) {
    u_fputc('\0', output);
    u_fflush(output);
  }
}

void
FSTProcessor::parseICX(std::string const &file)
{
  if(useIgnoredChars)
  {
    reader = xmlReaderForFile(file.c_str(), NULL, 0);
    if(reader == NULL)
    {
      std::cerr << "Error: cannot open '" << file << "'." << std::endl;
      exit(EXIT_FAILURE);
    }
    int ret = xmlTextReaderRead(reader);
    while(ret == 1)
    {
      procNodeICX();
      ret = xmlTextReaderRead(reader);
    }
    // No point trying to process ignored chars if there are none
    if(ignored_chars.size() == 0)
    {
      useIgnoredChars = false;
    }
  }
}

void
FSTProcessor::parseRCX(std::string const &file)
{
  if(useRestoreChars)
  {
    reader = xmlReaderForFile(file.c_str(), NULL, 0);
    if(reader == NULL)
    {
      std::cerr << "Error: cannot open '" << file << "'." << std::endl;
      exit(EXIT_FAILURE);
    }
    int ret = xmlTextReaderRead(reader);
    while(ret == 1)
    {
      procNodeRCX();
      ret = xmlTextReaderRead(reader);
    }
  }
}

void
FSTProcessor::procNodeICX()
{
  UString name = XMLParseUtil::readName(reader);
  if(name == XML_TEXT_NODE)
  {
    /* ignore */
  }
  else if(name == XML_IGNORED_CHARS_ELEM)
  {
    /* ignore */
  }
  else if(name == XML_CHAR_ELEM)
  {
    ignored_chars.insert(static_cast<int32_t>(XMLParseUtil::attrib(reader, XML_VALUE_ATTR)[0]));
  }
  else if(name == XML_COMMENT_NODE)
  {
    /* ignore */
  }
  else
  {
    std::cerr << "Error in ICX file (" << xmlTextReaderGetParserLineNumber(reader);
    std::cerr << "): Invalid node '<" << name << ">'." << std::endl;
    exit(EXIT_FAILURE);
  }
}

void
FSTProcessor::initDefaultIgnoredCharacters()
{
  ignored_chars.insert(173); // '\u00AD', soft hyphen
}

void
FSTProcessor::procNodeRCX()
{
  UString name = XMLParseUtil::readName(reader);
  if(name == XML_TEXT_NODE)
  {
    /* ignore */
  }
  else if(name == XML_RESTORE_CHARS_ELEM)
  {
    /* ignore */
  }
  else if(name == XML_CHAR_ELEM)
  {
    rcx_current_char = static_cast<int32_t>(XMLParseUtil::attrib(reader, XML_VALUE_ATTR)[0]);
  }
  else if(name == XML_RESTORE_CHAR_ELEM)
  {
    rcx_map[rcx_current_char].insert(static_cast<int32_t>(XMLParseUtil::attrib(reader, XML_VALUE_ATTR)[0]));
  }
  else if(name == XML_COMMENT_NODE)
  {
    /* ignore */
  }
  else
  {
    std::cerr << "Error in RCX file (" << xmlTextReaderGetParserLineNumber(reader);
    std::cerr << "): Invalid node '<" << name << ">'." << std::endl;
    exit(EXIT_FAILURE);
  }
}

int
FSTProcessor::readAnalysis(InputFile& input)
{
  if (!input_buffer.isEmpty())
  {
    UChar32 val = input_buffer.next();
    return val;
  }

  UChar32 val = input.get();
  int32_t altval = 0;
  if(input.eof())
  {
    input_buffer.add(0);        // so it's treated like the NUL byte
    return 0;
  } else if(val == U_EOF) {
    val = 0;
  }

  while ((useIgnoredChars || useDefaultIgnoredChars) && ignored_chars.find(val) != ignored_chars.end())
  {
    val = input.get();
  }

  if(escaped_chars.find(val) != escaped_chars.end())
  {
    switch(val)
    {
      case '<':
        altval = alphabet(input.readBlock('<', '>'));
        input_buffer.add(altval);
        return altval;

      case '[':
        val = input.get();

        if(val == '[')
        {
          blankqueue.push(input.finishWBlank());
        }
        else
        {
          input.unget(val);
          blankqueue.push(input.readBlock('[', ']'));
        }

        input_buffer.add(static_cast<int32_t>(' '));
        return static_cast<int32_t>(' ');

      case '\\':
        val = input.get();
        input_buffer.add(static_cast<int32_t>(val));
        return val;

      default:
        streamError();
    }
  }
  if(val == ' ') {
    blankqueue.push(" "_u);
  }

  input_buffer.add(val);
  return val;
}

int
FSTProcessor::readTMAnalysis(InputFile& input)
{
  isLastBlankTM = false;
  if(!input_buffer.isEmpty())
  {
    return input_buffer.next();
  }

  UChar32 val = input.get();
  int32_t altval = 0;
  if(input.eof())
  {
    return 0;
  }

  if(escaped_chars.find(val) != escaped_chars.end() || u_isdigit(val))
  {
    switch(val)
    {
      case '<':
        altval = alphabet(input.readBlock('<', '>'));
        input_buffer.add(altval);
        return altval;

      case '[':
        val = input.get();

        if(val == '[')
        {
          blankqueue.push(input.finishWBlank());
        }
        else
        {
          input.unget(val);
          blankqueue.push(input.readBlock('[', ']'));
        }

        input_buffer.add(static_cast<int32_t>(' '));
        isLastBlankTM = true;
        return static_cast<int32_t>(' ');

      case '\\':
        val = input.get();
        input_buffer.add(static_cast<int32_t>(val));
        return val;
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
        {
          UString ws;
          do
          {
            ws += val;
            val = input.get();
          } while(u_isdigit(val));
          input.unget(val);
          input_buffer.add(alphabet(u"<n>"));
          numbers.push_back(ws);
          return alphabet(u"<n>");
        }
        break;

      default:
        streamError();
    }
  }

  input_buffer.add(val);
  return val;
}

bool
FSTProcessor::readTransliterationBlank(InputFile& input)
{
  UString blank;
  while (!input.eof()) {
    UChar32 c = input.get();
    if (u_isspace(c)) {
      blank += c;
    } else if (c == '[') {
      if (input.peek() == '[') {
        break;
      }
      blank += input.readBlock('[', ']');
    } else {
      input.unget(c);
      break;
    }
  }
  if (!blank.empty()) {
    blankqueue.push(blank);
  }
  return !blank.empty();
}

bool
FSTProcessor::readTransliterationWord(InputFile& input)
{
  if (input.eof() || input.peek() == '\0') {
    return false;
  }

  if (!readTransliterationBlank(input)) {
    blankqueue.push(""_u);
  }

  UString wblank;
  std::vector<int32_t> word;
  if (input.peek() == '[') {
    input.get();
    wblank = input.finishWBlank();
    while (!input.eof()) {
      if (readTransliterationBlank(input)) {
        word.push_back(static_cast<int32_t>(' '));
        if (input.peek() == '[') break;
      } else {
        UChar32 c = input.get();
        if (c == '[') {
          input.unget(c);
          break;
        } else if (c == '\\') {
          word.push_back(static_cast<int32_t>(input.get()));
        } else if (c == '<') {
          word.push_back(alphabet(input.readBlock('<', '>')));
        } else if (c == '\0') {
          input.unget(c);
          break;
        } else {
          word.push_back(static_cast<int32_t>(c));
        }
      }
    }
    if (input.peek() == '[') {
      input.get();
      input.finishWBlank();
    }
  } else {
    while (!input.eof()) {
      UChar32 c = input.get();
      if (u_isspace(c) || c == '[' || c == '\0') {
        input.unget(c);
        break;
      } else if (c == '\\') {
        word.push_back(static_cast<int32_t>(input.get()));
      } else if (c == '<') {
        word.push_back(alphabet(input.readBlock('<', '>')));
      } else {
        word.push_back(static_cast<int32_t>(c));
      }
    }
  }
  if (word.empty()) {
    return false;
  }
  wblankqueue.push_back(wblank);
  transliteration_queue.push_back(word);

  return true;
}

void
FSTProcessor::flushBlanks(UFILE *output)
{
  for(size_t i = blankqueue.size(); i > 0; i--)
  {
    write(blankqueue.front(), output);
    blankqueue.pop();
  }
}

void
FSTProcessor::calcInitial()
{
  for(auto& it : transducers) {
    root.addTransition(0, 0, it.second.getInitial(), default_weight);
  }

  initial_state.init(&root);
}

void
FSTProcessor::classifyFinals()
{
  for(auto& it : transducers) {
    if(StringUtils::endswith(it.first, u"@inconditional"))
    {
      inconditional.insert(it.second.getFinals().begin(),
                           it.second.getFinals().end());
    }
    else if(StringUtils::endswith(it.first, u"@standard"))
    {
      standard.insert(it.second.getFinals().begin(),
                      it.second.getFinals().end());
    }
    else if(StringUtils::endswith(it.first, u"@postblank"))
    {
      postblank.insert(it.second.getFinals().begin(),
                       it.second.getFinals().end());
    }
    else if(StringUtils::endswith(it.first, u"@preblank"))
    {
      preblank.insert(it.second.getFinals().begin(),
                      it.second.getFinals().end());
    }
    else
    {
      std::cerr << "Error: Unsupported transducer type for '";
      std::cerr << it.first << "'." << std::endl;
      exit(EXIT_FAILURE);
    }
  }
}

UString
FSTProcessor::filterFinals(const State& state, UStringView casefrom)
{
  bool firstupper = false, uppercase = false;
  if (!dictionaryCase) {
    firstupper = u_isupper(casefrom[0]);
    uppercase = (casefrom.size() > 1 &&
                 firstupper && u_isupper(casefrom[casefrom.size()-1]));
  }
  return state.filterFinals(all_finals, alphabet, escaped_chars,
                            displayWeightsMode, maxAnalyses, maxWeightClasses,
                            uppercase, firstupper, 0);
}

void
FSTProcessor::writeEscaped(UStringView str, UFILE *output)
{
  for(unsigned int i = 0, limit = str.size(); i < limit; i++)
  {
    if(escaped_chars.find(str[i]) != escaped_chars.end())
    {
      u_fputc('\\', output);
    }
    u_fputc(str[i], output);
  }
}

size_t
FSTProcessor::writeEscapedPopBlanks(UStringView str, UFILE *output)
{
  size_t postpop = 0;
  for (unsigned int i = 0, limit = str.size(); i < limit; i++)
  {
    if (escaped_chars.find(str[i]) != escaped_chars.end()) {
      u_fputc('\\', output);
    }
    u_fputc(str[i], output);
    if (str[i] == ' ') {
      if (blankqueue.front() == " "_u) {
        blankqueue.pop();
      } else {
        postpop++;
      }
    }
  }
  return postpop;
}

void
FSTProcessor::writeEscapedWithTags(UStringView str, UFILE *output)
{
  for(unsigned int i = 0, limit = str.size(); i < limit; i++)
  {
    if(str[i] == '<' && i >=1 && str[i-1] != '\\')
    {
      write(str.substr(i), output);
      return;
    }

    if(escaped_chars.find(str[i]) != escaped_chars.end())
    {
      u_fputc('\\', output);
    }
    u_fputc(str[i], output);
  }
}



void
FSTProcessor::printWord(UStringView sf, UStringView lf, UFILE *output)
{
  u_fputc('^', output);
  writeEscaped(sf, output);
  write(lf, output);
  u_fputc('$', output);
}

void
FSTProcessor::printWordPopBlank(UStringView sf, UStringView lf, UFILE *output)
{
  u_fputc('^', output);
  size_t postpop = writeEscapedPopBlanks(sf, output);
  u_fprintf(output, "%.*S$", lf.size(), lf.data());
  while (postpop-- && blankqueue.size() > 0)
  {
    write(blankqueue.front(), output);
    blankqueue.pop();
  }
}

void
FSTProcessor::printUnknownWord(UStringView sf, UFILE *output)
{
  u_fputc('^', output);
  writeEscaped(sf, output);
  u_fputc('/', output);
  u_fputc('*', output);
  writeEscaped(sf, output);
  u_fputc('$', output);
}

unsigned int
FSTProcessor::lastBlank(UStringView str)
{
  for(int i = static_cast<int>(str.size())-1; i >= 0; i--)
  {
    if(alphabetic_chars.find(str[i]) == alphabetic_chars.end())
    {
      return static_cast<unsigned int>(i);
    }
  }

  return 0;
}

void
FSTProcessor::printSpace(UChar32 val, UFILE *output)
{
  if(blankqueue.size() > 0)
  {
    flushBlanks(output);
  }
  else
  {
    u_fputc(val, output);
  }
}

void
FSTProcessor::printChar(UChar32 val, UFILE* output)
{
  if (u_isspace(val)) {
    if (blankqueue.size() > 0) {
      write(blankqueue.front(), output);
      blankqueue.pop();
    } else {
      u_fputc(val, output);
    }
  } else {
    if (isEscaped(val)) {
      u_fputc('\\', output);
    }
    if (val) {
      u_fputc(val, output);
    }
  }
}

bool
FSTProcessor::isEscaped(UChar32 c) const
{
  return escaped_chars.find(c) != escaped_chars.end();
}

bool
FSTProcessor::isAlphabetic(UChar32 c) const
{
  return u_isalnum(c) || alphabetic_chars.find(c) != alphabetic_chars.end();
}

void
FSTProcessor::load(FILE *input)
{
  readTransducerSet(input, alphabetic_chars, alphabet, transducers);
}

void
FSTProcessor::initAnalysis()
{
  calcInitial();
  classifyFinals();
  all_finals = standard;
  all_finals.insert(inconditional.begin(), inconditional.end());
  all_finals.insert(postblank.begin(), postblank.end());
  all_finals.insert(preblank.begin(), preblank.end());
}

void
FSTProcessor::initTMAnalysis()
{
  calcInitial();

  for(auto& it : transducers) {
    all_finals.insert(it.second.getFinals().begin(),
                      it.second.getFinals().end());
  }
}

void
FSTProcessor::initGeneration()
{
  setIgnoredChars(false);
  calcInitial();
  for(auto& it : transducers) {
    all_finals.insert(it.second.getFinals().begin(),
                      it.second.getFinals().end());
  }
}

void
FSTProcessor::initTransliteration()
{
  initGeneration();
}

void
FSTProcessor::initBiltrans()
{
  initGeneration();
}


UString
FSTProcessor::compoundAnalysis(UString input_word)
{
  const int MAX_COMBINATIONS = 32767;

  State current_state = initial_state;

  for(unsigned int i=0; i<input_word.size(); i++)
  {
    UChar val=input_word[i];

    current_state.step_case(val, beCaseSensitive(current_state));

    if(current_state.size() > MAX_COMBINATIONS)
    {
      std::cerr << "Warning: compoundAnalysis's MAX_COMBINATIONS exceeded for '" << input_word << "'" << std::endl;
      std::cerr << "         gave up at char " << i << " '" << val << "'." << std::endl;

      UString nullString;
      return  nullString;
    }

    if(i < input_word.size()-1)
    {
      current_state.restartFinals(all_finals, compoundOnlyLSymbol, &initial_state, '+');
    }

    if(current_state.size()==0)
    {
      UString nullString;
      return nullString;
    }
  }

  current_state.pruneCompounds(compoundRSymbol, '+', compound_max_elements);
  return filterFinals(current_state, input_word);
}



void
FSTProcessor::initDecompositionSymbols()
{
  if((compoundOnlyLSymbol=alphabet(u"<:co:only-L>")) == 0
     && (compoundOnlyLSymbol=alphabet(u"<:compound:only-L>")) == 0
     && (compoundOnlyLSymbol=alphabet(u"<@co:only-L>")) == 0
     && (compoundOnlyLSymbol=alphabet(u"<@compound:only-L>")) == 0
     && (compoundOnlyLSymbol=alphabet(u"<compound-only-L>")) == 0)
  {
    std::cerr << "Warning: Decomposition symbol <:compound:only-L> not found" << std::endl;
  }
  else if(!showControlSymbols)
  {
    alphabet.setSymbol(compoundOnlyLSymbol, u"");
  }

  if((compoundRSymbol=alphabet(u"<:co:R>")) == 0
     && (compoundRSymbol=alphabet(u"<:compound:R>")) == 0
     && (compoundRSymbol=alphabet(u"<@co:R>")) == 0
     && (compoundRSymbol=alphabet(u"<@compound:R>")) == 0
     && (compoundRSymbol=alphabet(u"<compound-R>")) == 0)
  {
    std::cerr << "Warning: Decomposition symbol <:compound:R> not found" << std::endl;
  }
  else if(!showControlSymbols)
  {
    alphabet.setSymbol(compoundRSymbol, u"");
  }
}


void
FSTProcessor::initDecomposition()
{
  do_decomposition = true;
  initAnalysis();
  initDecompositionSymbols();
}

void
FSTProcessor::analysis(InputFile& input, UFILE *output)
{
  if(getNullFlush())
  {
    analysis_wrapper_null_flush(input, output);
  }

  bool last_incond = false;
  bool last_postblank = false;
  bool last_preblank = false;
  State current_state = initial_state;
  UString lf;            // analysis (lexical form and tags)
  UString sf;            // surface form
  UString lf_spcmp;      // space compound analysis
  bool seen_cpL = false; // have we seen a <compound-only-L> tag so far
  size_t last_start = input_buffer.getPos(); // position in input_buffer when sf was last cleared
  size_t last = 0;       // position in input_buffer after last analysis
  size_t last_size = 0;  // size of sf at last analysis
  std::map<int, std::set<int> >::iterator rcx_map_ptr;

  UChar32 val;
  do
  {
    val = readAnalysis(input);
    // test for final states
    if(current_state.isFinal(all_finals))
    {
      if(current_state.isFinal(inconditional))
      {
        if(do_decomposition && compoundOnlyLSymbol != 0)
        {
          current_state.pruneStatesWithForbiddenSymbol(compoundOnlyLSymbol);
        }
        lf = filterFinals(current_state, sf);
        last_incond = true;
        last = input_buffer.getPos();
        last_size = sf.size();
      }
      else if(current_state.isFinal(postblank))
      {
        if(do_decomposition && compoundOnlyLSymbol != 0)
        {
          current_state.pruneStatesWithForbiddenSymbol(compoundOnlyLSymbol);
        }
        lf = filterFinals(current_state, sf);
        last_postblank = true;
        last = input_buffer.getPos();
        last_size = sf.size();
      }
      else if(current_state.isFinal(preblank))
      {
        if(do_decomposition && compoundOnlyLSymbol != 0)
        {
          current_state.pruneStatesWithForbiddenSymbol(compoundOnlyLSymbol);
        }
        lf = filterFinals(current_state, sf);
        last_preblank = true;
        last = input_buffer.getPos();
        last_size = sf.size();
      }
      else if(!isAlphabetic(val))
      {
        if(do_decomposition && compoundOnlyLSymbol != 0)
        {
          current_state.pruneStatesWithForbiddenSymbol(compoundOnlyLSymbol);
        }
        lf = filterFinals(current_state, sf);
        last_postblank = false;
        last_preblank = false;
        last_incond = false;
        last = input_buffer.getPos();
        last_size = sf.size();
      }
      else { // isAlphabetic, standard type section
        // Record if a compound might be possible
        if (do_decomposition && compoundOnlyLSymbol != 0
            && current_state.hasSymbol(compoundOnlyLSymbol)) {
          seen_cpL = true;
        }
      }
    }
    else if(sf.empty() && u_isspace(val))
    {
      lf = "/*"_u;
      lf.append(sf);
      last_postblank = false;
      last_preblank = false;
      last_incond = false;
      last = input_buffer.getPos();
      last_size = sf.size();
    }

    if(useRestoreChars && rcx_map.find(val) != rcx_map.end())
    {
      rcx_map_ptr = rcx_map.find(val);
      std::set<int> tmpset = rcx_map_ptr->second;
      if(!u_isupper(val) || beCaseSensitive(current_state))
      {
        current_state.step(val, tmpset);
      }
      else if(rcx_map.find(u_tolower(val)) != rcx_map.end())
      {
        rcx_map_ptr = rcx_map.find(tolower(val));
        tmpset.insert(tolower(val));
        tmpset.insert(rcx_map_ptr->second.begin(), rcx_map_ptr->second.end());
        current_state.step(val, tmpset);
      }
      else
      {
        tmpset.insert(tolower(val));
        current_state.step(val, tmpset);
      }
    }
    else
    {
       	    current_state.step_case(val, beCaseSensitive(current_state));
    }

    if(current_state.size() != 0)
    {
      if(val != 0)
      {
        alphabet.getSymbol(sf, val);
      }
    }
    else
    {
      // First try if blank-crossing compound analysis is possible; have
      // to fall back on the regular methods if this didn't work:
      lf_spcmp.clear();
      if (seen_cpL  // We've seen both a space and a <compund-only-L>
          && isAlphabetic(val)
          && !sf.empty()
          && last_size <= lastBlank(sf)) {
        int oldval = val;
        UString oldsf = sf;
        do {
          alphabet.getSymbol(sf, val);
        } while ((val = readAnalysis(input)) && isAlphabetic(val));
        lf_spcmp = compoundAnalysis(sf);
        if(lf_spcmp.empty()) {  // didn't work, rewind!
          input_buffer.back(sf.size() - oldsf.size());
          val = oldval;
          sf.swap(oldsf);
        }
        else {
          input_buffer.back(1);
          val = input_buffer.peek();
        }
      }
      seen_cpL = false;

      if(!lf_spcmp.empty()) {
        printWordPopBlank(sf, lf_spcmp, output);
      }
      else if(!isAlphabetic(val) && sf.empty())
      {
        printChar(val, output);
      }
      else if(last_postblank)
      {
        printWordPopBlank(sf.substr(0, last_size),
                          lf, output);
        u_fputc(' ', output);
        input_buffer.setPos(last);
        input_buffer.back(1);
      }
      else if(last_preblank)
      {
        u_fputc(' ', output);
        printWordPopBlank(sf.substr(0, last_size),
                          lf, output);
        input_buffer.setPos(last);
        input_buffer.back(1);
      }
      else if(last_incond)
      {
        printWordPopBlank(sf.substr(0, last_size),
                          lf, output);
        input_buffer.setPos(last);
        input_buffer.back(1);
      }
      else if(isAlphabetic(val) &&
               // we can't skip back a blank:
              (last_size > lastBlank(sf) ||
               // or we've failed to reach an analysis:
               lf.empty()))
      {
        do
        {
          alphabet.getSymbol(sf, val);
        }
        while((val = readAnalysis(input)) && isAlphabetic(val));

        auto limit = firstNotAlpha(sf);
        if(limit.i_codepoint == 0)
        {
          input_buffer.setPos(1 + last_start);
          writeEscaped(sf.substr(0,1), output);
        }
        else
        {
          input_buffer.setPos(last_start + limit.i_codepoint);
          UString unknown_word = sf.substr(0, limit.i_utf16);
          if(do_decomposition)
          {
            UString compound = compoundAnalysis(unknown_word);
            if(!compound.empty())
            {
              printWord(unknown_word, compound, output);
            }
            else
            {
              printUnknownWord(unknown_word, output);
            }
          }
          else
          {
            printUnknownWord(unknown_word, output);
          }
        }
      }
      else if(lf.empty())
      {
        auto limit = firstNotAlpha(sf);
        if(limit.i_codepoint == 0)
        {
          input_buffer.setPos(1 + last_start);
          writeEscaped(sf.substr(0,1), output);
        }
        else
        {
          input_buffer.setPos(last_start + limit.i_codepoint);
          UString unknown_word = sf.substr(0, limit.i_utf16);
          if(do_decomposition)
          {
            UString compound = compoundAnalysis(unknown_word);
            if(!compound.empty())
            {
              printWord(unknown_word, compound, output);
            }
            else
            {
              printUnknownWord(unknown_word, output);
            }
          }
          else
          {
            printUnknownWord(unknown_word, output);
          }
        }
      }
      else
      {
        printWordPopBlank(sf.substr(0, last_size),
                          lf, output);
        input_buffer.setPos(last);
        input_buffer.back(1);
      }
      if(val == 0) {
        if(!input_buffer.isEmpty()) {
          input_buffer.setPos(last+1);
        }
      }

      current_state = initial_state;
      lf.clear();
      sf.clear();
      last_start = input_buffer.getPos();
      last_incond = false;
      last_postblank = false;
      last_preblank = false;
    }
  }
  while(val);

  // print remaining blanks
  flushBlanks(output);
}

void
FSTProcessor::analysis_wrapper_null_flush(InputFile& input, UFILE *output)
{
  setNullFlush(false);
  while(!input.eof())
  {
    analysis(input, output);
    u_fputc('\0', output);
    u_fflush(output);
    // analysis() doesn't always leave input_buffer empty
    // which results in repeatedly analyzing the same string
    // so clear it here
    while (!input_buffer.isEmpty()) input_buffer.next();
  }
}

void
FSTProcessor::generation_wrapper_null_flush(InputFile& input, UFILE *output,
                                            GenerationMode mode)
{
  setNullFlush(false);
  nullFlushGeneration = true;

  while(!input.eof())
  {
    generation(input, output, mode);
    u_fputc('\0', output);
    u_fflush(output);
  }
}

void
FSTProcessor::tm_wrapper_null_flush(InputFile& input, UFILE *output,
                                    TranslationMemoryMode tm_mode)
{
  setNullFlush(false);
  nullFlushGeneration = true;

  while(!input.eof())
  {
    tm_analysis(input, output, tm_mode);
    u_fputc('\0', output);
    u_fflush(output);
  }
}


void
FSTProcessor::tm_analysis(InputFile& input, UFILE *output, TranslationMemoryMode tm_mode)
{
  if(getNullFlush())
  {
    tm_wrapper_null_flush(input, output, tm_mode);
  }

  State current_state = initial_state;
  UString lf;     //lexical form
  UString sf;     //surface form
  int last = 0;

  while(int32_t val = readTMAnalysis(input))
  {
    // test for final states
    if(current_state.isFinal(all_finals))
    {
      if(u_ispunct(val) || (tm_mode == tm_space && u_isspace(val)))
      {
        lf = current_state.filterFinalsTM(all_finals, alphabet,
                                          escaped_chars,
                                          blankqueue, numbers).substr(1);
        last = input_buffer.getPos();
        numbers.clear();
      }
    }
    else if(sf.empty() && u_isspace(val))
    {
      lf.append(sf);
      last = input_buffer.getPos();
    }

    current_state.step_case(val, false);

    if(current_state.size() != 0)
    {
      if(val == -1)
      {
        sf.append(numbers[numbers.size()-1]);
      }
      else if(isLastBlankTM && val == ' ')
      {
        sf.append(blankqueue.back());
      }
      else
      {
        alphabet.getSymbol(sf, val);
      }
    }
    else
    {
      if((u_isspace(val) || u_ispunct(val)) && sf.empty())
      {
        if(u_isspace(val))
        {
          printSpace(val, output);
        }
        else
        {
          if(isEscaped(val))
          {
            u_fputc('\\', output);
          }
          u_fputc(val, output);
        }
      }
      else if(!u_isspace(val) && !u_ispunct(val) &&
              ((sf.size()-input_buffer.diffPrevPos(last)) > lastBlank(sf) ||
               lf.empty()))
      {

        do
        {
          if(val == -1)
          {
            sf.append(numbers[numbers.size()-1]);
          }
          else if(isLastBlankTM && val == ' ')
          {
            sf.append(blankqueue.back());
          }
          else
          {
            alphabet.getSymbol(sf, val);
          }
        }
        while((val = readTMAnalysis(input)) && !u_isspace(val) && !u_ispunct(val));

        if(val == 0)
        {
          write(sf, output);
          return;
        }

        input_buffer.back(1);
        write(sf, output);

        while(blankqueue.size() > 0)
        {
          if(blankqueue.size() == 1 && isLastBlankTM)
          {
            break;
          }
          blankqueue.pop();
        }

/*
        unsigned int limit = sf.find(' ');
        unsigned int size = sf.size();
        limit = (limit == static_cast<unsigned int>(UString::npos)?size:limit);
        input_buffer.back(1+(size-limit));
        write(sf.substr(0, limit), output);
*/      }
      else if(lf.empty())
      {
/*        unsigned int limit = sf.find(' ');
        unsigned int size = sf.size();
        limit = (limit == static_cast<unsigned int >(UString::npos)?size:limit);
        input_buffer.back(1+(size-limit));
        write(sf.substr(0, limit), output);
*/
        input_buffer.back(1);
        write(sf, output);

        while(blankqueue.size() > 0)
        {
          if(blankqueue.size() == 1 && isLastBlankTM)
          {
            break;
          }
          blankqueue.pop();
        }

      }
      else
      {
        u_fprintf(output, "[%S]", lf.c_str());
        input_buffer.setPos(last);
        input_buffer.back(1);
      }

      current_state = initial_state;
      lf.clear();
      sf.clear();
      numbers.clear();
    }
  }

  // print remaining blanks
  flushBlanks(output);
}


void
FSTProcessor::generation(InputFile& input, UFILE *output, GenerationMode mode)
{
  StreamReader reader(&input);
  reader.alpha = &alphabet;
  State current_state;

  while (!reader.at_eof) {
    reader.next();
    write(reader.blank, output);
    write(reader.wblank, output);
    if (!reader.readings.empty()) {
      auto& rd = reader.readings[0];
      bool skip = false;
      switch (rd.mark) {
      case '=':
        u_fputc('=', output);
        break;
      case '*':
      case '%':
        skip = true;
        if (mode == gm_tagged_nm) {
          u_fputc('^', output);
          writeEscaped(removeTags(rd.content), output);
          u_fputc('/', output);
          u_fputc(rd.mark, output);
          writeEscapedWithTags(rd.content, output);
          u_fputc('$', output);
        } else {
          if (mode != gm_clean) u_fputc(rd.mark, output);
          writeEscaped(rd.content, output);
        }
        break;
      case '@':
        skip = true;
        switch (mode) {
        case gm_all:
          u_fputc(rd.mark, output);
          writeEscaped(rd.content, output);
          break;
        case gm_unknown:
        case gm_tagged:
          u_fputc(rd.mark, output);
          [[fallthrough]];
        case gm_clean:
          writeEscaped(removeTags(rd.content), output);
          break;
        case gm_tagged_nm:
          u_fputc('^', output);
          writeEscaped(removeTags(rd.content), output);
          u_fputc('/', output);
          u_fputc(rd.mark, output);
          writeEscapedWithTags(rd.content, output);
          u_fputc('$', output);
          break;
        default:
          break;
        }
        break;
      }
      if (!skip) {
        current_state = initial_state;
        for (auto& sym : reader.readings[0].symbols) {
          if (!alphabet.isTag(sym) && u_isupper(sym) &&
              !beCaseSensitive(current_state)) {
            if (mode == gm_carefulcase) {
              current_state.step_careful(sym, u_tolower(sym));
            }
            else {
              current_state.step(sym, u_tolower(sym));
            }
          }
          else current_state.step(sym);
        }
        if (current_state.isFinal(all_finals)) {
          bool firstupper = false, uppercase = false;
          if (!dictionaryCase) {
            uppercase = rd.content.size() > 1 && u_isupper(rd.content[1]);
            firstupper= u_isupper(rd.content[0]);
          }

          if (mode == gm_tagged || mode == gm_tagged_nm) {
            u_fputc('^', output);
          }

          write(current_state.filterFinals(all_finals, alphabet, escaped_chars,
                                           displayWeightsMode, maxAnalyses,
                                           maxWeightClasses,
                                           uppercase, firstupper).substr(1), output);
          if (mode == gm_tagged || mode == gm_tagged_nm) {
            u_fputc('/', output);
            writeEscapedWithTags(rd.content, output);
            u_fputc('$', output);
          }
        } else {
          switch (mode) {
          case gm_all:
            u_fputc('#', output);
            writeEscaped(rd.content, output);
            break;
          case gm_carefulcase:
          case gm_unknown:
          case gm_tagged:
            if (!rd.content.empty()) u_fputc('#', output);
            [[fallthrough]];
          case gm_clean:
            writeEscaped(removeTags(rd.content), output);
            break;
          case gm_tagged_nm:
            u_fputc('^', output);
            writeEscaped(removeTags(rd.content), output);
            u_fputc('/', output);
            u_fputc('#', output);
            writeEscapedWithTags(rd.content, output);
            u_fputc('$', output);
            break;
          }
        }
      }
    }
    if (reader.at_null) {
      u_fputc('\0', output);
      u_fflush(output);
    }
  }
}

void
FSTProcessor::postgeneration(InputFile& input, UFILE *output)
{
  transliteration_drop_tilde = true;
  transliteration(input, output);
}

void
FSTProcessor::intergeneration(InputFile& input, UFILE *output)
{
  transliteration_drop_tilde = false;
  transliteration(input, output);
}

void
FSTProcessor::transliteration(InputFile& input, UFILE *output)
{
  size_t start_pos = 0;
  size_t cur_word = 0;
  size_t cur_pos = 0;
  size_t match_pos = 0;
  State current_state = initial_state;
  UString last_match;
  int space_diff = 0;

  bool firstupper = false;
  bool uppercase = false;
  bool have_first = false;
  bool have_second = false;

  while (true) {
    if (transliteration_queue.empty()) {
      if (!blankqueue.empty()) {
        flushBlanks(output);
      }
      if (!readTransliterationWord(input)) {
        flushBlanks(output);
        if (input.eof()) {
          break;
        } else {
          u_fputc(input.get(), output);
          u_fflush(output);
          continue;
        }
      }
    }

    if (current_state.isFinal(all_finals)) {
      last_match = current_state.filterFinals(all_finals, alphabet,
                                              escaped_chars, displayWeightsMode,
                                              1, maxWeightClasses,
                                              uppercase, firstupper);
      while (cur_word > 0) {
        if (cur_word == 1) {
          if (cur_pos == 0 && last_match[last_match.size()-1] == ' ') {
            match_pos = transliteration_queue.front().size();
            last_match = last_match.substr(0, last_match.size()-1);
            break;
          } else {
            cur_pos += transliteration_queue.front().size() + 1;
          }
        }
        std::vector<int32_t> word = transliteration_queue.front();
        transliteration_queue.pop_front();
        word.push_back(static_cast<int32_t>(' '));
        word.insert(word.end(), transliteration_queue.front().begin(),
                    transliteration_queue.front().end());
        transliteration_queue.pop_front();
        transliteration_queue.push_front(word);
        UString wblank = wblankqueue.front();
        wblankqueue.pop_front();
        wblank = StringUtils::merge_wblanks(wblank, wblankqueue.front());
        wblankqueue.pop_front();
        wblankqueue.push_front(wblank);
        cur_word--;
      }
      if (cur_word == 0) {
        match_pos = cur_pos;
      }
    }

    int32_t sym = 0;
    bool is_end = false;
    if (cur_pos < transliteration_queue[cur_word].size()) {
      sym = transliteration_queue[cur_word][cur_pos];
      cur_pos++;
    } else {
      if (cur_word + 1 == transliteration_queue.size() &&
          !readTransliterationWord(input)) {
        is_end = true;
      } else {
        sym = static_cast<int32_t>(' ');
        cur_word++;
        cur_pos = 0;
      }
    }

    if (isAlphabetic(sym)) {
      if (!have_first) {
        have_first = true;
        if (u_isupper(sym)) {
          firstupper = true;
        } else {
          firstupper = false;
          have_second = true;
        }
      } else if (!have_second) {
        have_second = true;
        uppercase = u_isupper(sym);
      }
    }

    current_state.step_case_override(sym, beCaseSensitive(current_state));

    if (current_state.size() == 0 || is_end) {
      if (last_match.empty()) {
        start_pos++;
      } else {
        std::vector<int32_t> match = alphabet.tokenize(last_match.substr(1));
        last_match.clear();
        std::vector<int32_t> word = transliteration_queue.front();
        transliteration_queue.pop_front();
        size_t i = 0;
        for (; i < match.size() && i < match_pos - start_pos; i++) {
          if (match[match.size()-i-1] != word[match_pos-i-1]) {
            break;
          }
        }
        std::vector<int32_t> new_word;
        new_word.insert(new_word.end(), word.begin(), word.begin()+start_pos);
        new_word.insert(new_word.end(), match.begin(), match.end());
        new_word.insert(new_word.end(), word.begin()+match_pos, word.end());
        transliteration_queue.push_front(new_word);
        int sf_spaces = 0;
        int lf_spaces = 0;
        for (auto c : word) {
          if (c == static_cast<int32_t>(' ')) sf_spaces++;
        }
        for (auto c : new_word) {
          if (c == static_cast<int32_t>(' ')) lf_spaces++;
        }
        space_diff += (lf_spaces - sf_spaces);
        size_t last_start = start_pos;
        start_pos = match_pos - i;
        if (start_pos == last_start) start_pos++;
        cur_pos = start_pos;
        cur_word = 0;
      }
      if (start_pos >= transliteration_queue.front().size()) {
        write(blankqueue.front(), output);
        blankqueue.pop();
        bool has_wblank = !wblankqueue.front().empty();
        write(wblankqueue.front(), output);
        wblankqueue.pop_front();
        auto word = transliteration_queue.front();
        transliteration_queue.pop_front();
        int space_count = 0;
        for (auto c : word) {
          if (c == static_cast<int32_t>(' ')) space_count++;
        }
        int space_out = 0;
        UString out;
        for (auto c : word) {
          if (c == ' ') {
            if (space_out + space_diff >= space_count) {
              out += ' ';
            } else {
              out += blankqueue.front();
              blankqueue.pop();
            }
            space_out++;
          } else if (transliteration_drop_tilde &&
                     c == static_cast<int32_t>('~')) {
          } else {
            if (c > 0 && isEscaped(c)) {
              out += '\\';
            }
            alphabet.getSymbol(out, c);
          }
        }
        write(out, output);
        if (has_wblank) {
          write(WBLANK_FINAL, output);
        }
        while (space_diff < 0) {
          if (blankqueue.front() != " "_u) {
            write(blankqueue.front(), output);
          }
          blankqueue.pop();
          space_diff++;
        }
        space_diff = 0;
        start_pos = 0;
      }
      match_pos = 0;
      cur_pos = start_pos;
      cur_word = 0;
      uppercase = false;
      firstupper = false;
      have_first = false;
      have_second = false;
      current_state = initial_state;
    }
  }
}

bool
FSTProcessor::step_biltrans(UStringView word, std::vector<UString>& result, UString& queue)
{
  State current_state = initial_state;
  bool firstupper = u_isupper(word[0]);
  bool uppercase = firstupper && u_isupper(word[1]);
  for (auto symbol : symbol_iter(word)) {
    int32_t val = (symbol.size() == 1 ? symbol[0] : alphabet(symbol));
    if (current_state.size() != 0) {
      current_state.step(val, beCaseSensitive(current_state));
    }
    if (current_state.isFinal(all_finals)) {
      current_state.filterFinalsArray(result,
                                      all_finals, alphabet,
                                      escaped_chars,
                                      displayWeightsMode, maxAnalyses, maxWeightClasses,
                                      uppercase, firstupper, 0);
    }
    if (current_state.size() == 0) {
      if (!result.empty()) queue.append(symbol);
      else return false;
    }
  }
  return !result.empty();
}

UString
FSTProcessor::biltransfull(UStringView input_word, bool with_delim)
{
  std::vector<UString> result;
  unsigned int start_point = 1;
  unsigned int end_point = input_word.size()-2;
  UString queue;
  bool mark = false;

  if(with_delim == false)
  {
    start_point = 0;
    end_point = input_word.size()-1;
  }

  if(input_word[start_point] == '*')
  {
    return US(input_word);
  }

  if(input_word[start_point] == '=')
  {
    start_point++;
    mark = true;
  }

  auto word = input_word.substr(start_point, end_point-start_point);
  bool exists = step_biltrans(word, result, queue);
  if (!exists) {
    if (with_delim) return "^@"_u + US(input_word.substr(1));
    else return "@"_u + US(input_word);
  }

  if(start_point < (end_point - 3))
  {
    return "^$"_u;
  }
  // attach unmatched queue automatically

  return compose(result, queue, with_delim, mark);
}



UString
FSTProcessor::biltrans(UStringView input_word, bool with_delim)
{
  State current_state = initial_state;
  std::vector<UString> result;
  unsigned int start_point = 1;
  unsigned int end_point = input_word.size()-2;
  UString queue;
  bool mark = false;

  if(with_delim == false)
  {
    start_point = 0;
    end_point = input_word.size()-1;
  }

  if(input_word[start_point] == '*')
  {
    return US(input_word);
  }

  if(input_word[start_point] == '=')
  {
    start_point++;
    mark = true;
  }

  UStringView word = input_word.substr(start_point, end_point-start_point);
  bool exists = step_biltrans(word, result, queue);
  if (!exists) {
    if (with_delim) return "^@"_u + US(input_word.substr(1));
    else return "@"_u + US(input_word);
  }

  // attach unmatched queue automatically

  return compose(result, queue, with_delim, mark);
}

UString
FSTProcessor::compose(const std::vector<UString>& lexforms, UStringView queue,
                      bool delim, bool mark) const
{
  UString result;
  if (delim) result += '^';
  if (mark) result += '=';
  bool first = true;
  for (auto& it : lexforms) {
    if (!first) result += '/';
    first = false;
    result += it;
    result += queue;
  }
  if (delim) result += '$';
  return result;
}

void
FSTProcessor::bilingual(InputFile& input, UFILE *output, GenerationMode mode)
{
  StreamReader reader(&input);
  reader.alpha = &alphabet;
  reader.add_unknowns = true;

  size_t index = (biltransSurfaceForms || biltransSurfaceFormsKeep ? 1 : 0);

  while (!reader.at_eof) {
    reader.next();

    write(reader.blank, output);
    write(reader.wblank, output);

    if (biltransSurfaceFormsKeep && !reader.readings.empty()) {
      u_fputc('^', output);
      write(reader.readings[0].content, output);
      u_fputc((reader.readings.size() > 1 ? '/' : '$'), output);
    }

    if (index >= reader.readings.size()) {
      maybeFlush(output, reader.at_null);
      continue;
    }

    if (!biltransSurfaceFormsKeep) u_fputc('^', output);

    if (reader.readings[index].mark == '*') {
      u_fputc('*', output);
      write(reader.readings[index].content, output);
      u_fputc('/', output);
      if (mode != gm_clean) u_fputc('*', output);
      write(reader.readings[index].content, output);
      u_fputc('$', output);
      maybeFlush(output, reader.at_null);
      continue;
    }

    auto& symbols = reader.readings[index].symbols;

    if (symbols.empty()) {
      u_fputc('$', output);
      maybeFlush(output, reader.at_null);
      continue;
    }

    State current_state = initial_state;

    bool firstupper = (symbols[0] > 0 && u_isupper(symbols[0]));
    bool uppercase = (firstupper && symbols.size() > 1 &&
                      symbols[1] > 0 && u_isupper(symbols[1]));

    bool seenTags = false;
    size_t queue_start = 0;
    std::vector<UString> result;
    if (reader.readings[index].mark == '#') current_state.step('#');
    for (size_t i = 0; i < symbols.size(); i++) {
      seenTags = seenTags || alphabet.isTag(symbols[i]);
      current_state.step_case(symbols[i], beCaseSensitive(current_state));
      if (current_state.isFinal(all_finals)) {
        queue_start = i;
        current_state.filterFinalsArray(result,
                                        all_finals, alphabet, escaped_chars,
                                        displayWeightsMode, maxAnalyses,
                                        maxWeightClasses, uppercase,
                                        firstupper, 0);
      }
    }
    // if there are no tags, we only return complete matches
    if (!seenTags && queue_start + 1 < symbols.size()) result.clear();

    UString source;
    size_t queue_pos = 0;
    if (reader.readings[index].mark == '#') {
      source += '#';
      queue_pos = 1;
    }
    for (size_t i = 0; i < symbols.size(); i++) {
      if (isEscaped(symbols[i]) || (i == 0 && symbols[i] == '*')) source += '\\';
      alphabet.getSymbol(source, symbols[i]);
      if (i == queue_start) queue_pos = source.size();
    }

    write(source, output);
    u_fputc('/', output);

    if (!result.empty()) {
      write(compose(result, source.substr(queue_pos)), output);
    } else {
      u_fputc((mode == gm_all ? '#' : '@'), output);
      write(source, output);
    }
    u_fputc('$', output);

    if (reader.at_null) {
      u_fputc('\0', output);
      u_fflush(output);
    }
  }
}

std::pair<UString, int>
FSTProcessor::biltransWithQueue(UStringView input_word, bool with_delim)
{
  State current_state = initial_state;
  std::vector<UString> result;
  std::vector<UString> temp;
  unsigned int start_point = 1;
  unsigned int end_point = input_word.size()-2;
  UString queue;
  bool mark = false;
  bool seentags = false;  // have we seen any tags at all in the analysis?

  if(with_delim == false)
  {
    start_point = 0;
    end_point = input_word.size()-1;
  }

  if(input_word[start_point] == '*')
  {
    return {US(input_word), 0};
  }

  if(input_word[start_point] == '=')
  {
    start_point++;
    mark = true;
  }

  bool firstupper = u_isupper(input_word[start_point]);
  bool uppercase = firstupper && u_isupper(input_word[start_point+1]);

  UStringView word = input_word.substr(start_point, end_point-start_point);
  for (auto symbol : symbol_iter(word)) {
    int32_t val;
    if (symbol.size() == 1) {
      val = symbol[0];
    } else {
      val = alphabet(symbol);
      seentags = true;
    }
    if(current_state.size() != 0)
    {
      current_state.step_case(val, beCaseSensitive(current_state));
    }
    if(current_state.isFinal(all_finals))
    {
      current_state.filterFinalsArray(result, all_finals, alphabet,
                                      escaped_chars,
                                      displayWeightsMode, maxAnalyses, maxWeightClasses,
                                      uppercase, firstupper, 0);
    }

    if(current_state.size() == 0)
    {
      if(!symbol.empty() && !result.empty())
      {
        queue.append(symbol);
      }
      else
      {
        // word is not present
        if(with_delim)
        {
          return {"^@"_u + US(input_word.substr(1)), 0};
        }
        else
        {
          return {"@"_u + US(input_word), 0};
        }
      }
    }
  }

  if (!seentags
      && current_state.filterFinals(all_finals, alphabet, escaped_chars,
                                    displayWeightsMode, maxAnalyses, maxWeightClasses,
                                    uppercase, firstupper, 0).empty())
  {
    // word is not present
    if(with_delim)
    {
      return {"^@"_u + US(input_word.substr(1)), 0};
    }
    else
    {
      return {"@"_u + US(input_word), 0};
    }
  }



  // attach unmatched queue automatically
  return {compose(result, queue, with_delim, mark), queue.size()};
}

UString
FSTProcessor::biltransWithoutQueue(UStringView input_word, bool with_delim)
{
  State current_state = initial_state;
  std::vector<UString> result;
  unsigned int start_point = 1;
  unsigned int end_point = input_word.size()-2;
  bool mark = false;

  if(with_delim == false)
  {
    start_point = 0;
    end_point = input_word.size()-1;
  }

  if(input_word[start_point] == '*')
  {
    return US(input_word);
  }

  if(input_word[start_point] == '=')
  {
    start_point++;
    mark = true;
  }

  auto word = input_word.substr(start_point, end_point-start_point);
  UString queue;
  bool exists = step_biltrans(word, result, queue);
  if (!exists || !queue.empty()) {
    if (with_delim) return "^@"_u + US(input_word.substr(1));
    else return "@"_u + US(input_word);
  }

  return compose(result, ""_u, with_delim, mark);
}

void
FSTProcessor::quoteMerge(InputFile& input, UFILE *output)
{
  StreamReader reader(&input);
  reader.alpha = &alphabet;
  reader.add_unknowns = true;

  bool merging = false;
  UString surface;
  while (!reader.at_eof) {
    reader.next();

    bool end_merging = false;
    std::cerr << "CHUNK: " << reader.chunk << std::endl;

    for (StreamReader::Reading &it : reader.readings) {
      std::cerr << "Reading:  " << it.content << std::endl;
      // TODO: look up in it.symbols instead (but need to make an alphabet then)
      if(it.content.find(u"<MERGE_BEG>") != std::string::npos) {
        merging = true;
        std::cerr << "\033[1;35mSTART MERGE\033[0m" << std::endl;
      }
      if(it.content.find(u"<MERGE_END>") != std::string::npos) {
        end_merging = true;
      }
    }
    if(merging) {
      std::cerr << "\033[0;35mmerging\033[0m" << std::endl;
      if(surface.size() > 0) {
        surface += reader.blank;
        appendEscaped(surface, reader.wblank);
      }
      else {
        // The initial blank should just be output before the merged LU:
        write(reader.blank, output);
        write(reader.wblank, output);
      }
      if(reader.readings.size() > 0) {
        // Double-escape the form since we'll unescape during lt-unmerge:
        appendEscaped(surface, reader.readings[0].content);
      }
    }
    else {
      write(reader.blank, output);
      write(reader.wblank, output);
      if(reader.readings.size() > 0) {
        // NB. ^$ will produce a readings vector of length 1 where the single item is empty. EOF should give length 0.
        // (We *want* to keep ^$ in stream, but not print extra ^$ when there was no ^$)
        u_fputc('^', output);
        bool seen_reading = false;
        for (StreamReader::Reading &it : reader.readings) {
          if (seen_reading) {
            u_fputc('/', output);
          }
          write(it.content, output);
          seen_reading = true;
        }
        u_fputc('$', output);
      }
    }
    if(end_merging || reader.at_null) {
      std::cerr << "\033[1;35msurface=\t" << surface << "\033[0m" << std::endl;
      std::cerr << "\033[1;35mEND_MERGE\033[0m" << std::endl;
      if (merging) {
        u_fputc('^', output);
        write(surface, output);
        u_fputc('/', output);
        write(surface, output);
        write("<MERGED>$"_u, output);
        merging = false;
      }
      end_merging = false;
      surface.clear();
      if(reader.at_null) {
        u_fputc('\0', output);
        u_fflush(output);
      }
    }
  }
}


bool
FSTProcessor::valid() const
{
  if(initial_state.isFinal(all_finals))
  {
    std::cerr << "Error: Invalid dictionary (hint: the left side of an entry is empty)" << std::endl;
    return false;
  }
  else
  {
    State s = initial_state;
    s.step(' ');
    if(s.size() != 0)
    {
      std::cerr << "Error: Invalid dictionary (hint: entry beginning with whitespace)" << std::endl;
      return false;
    }
  }

  return true;
}

int
FSTProcessor::readSAO(InputFile& input)
{
  if(!input_buffer.isEmpty())
  {
    return input_buffer.next();
  }

  UChar32 val = input.get();
  if(input.eof())
  {
    return 0;
  }

  if(escaped_chars.find(val) != escaped_chars.end())
  {
    if(val == '<')
    {
      UString str = input.readBlock('<', '>');
      if(StringUtils::startswith(str, u"<![CDATA["))
      {
        while(!StringUtils::endswith(str, u"]]>"))
        {
          str.append(input.readBlock('<', '>').substr(1));
        }
        blankqueue.push(str);
        input_buffer.add(static_cast<int32_t>(' '));
        return static_cast<int32_t>(' ');
      }
      else
      {
        streamError();
      }
    }
    else if (val == '\\') {
      val = input.get();
      if(isEscaped(val))
      {
        input_buffer.add(val);
        return static_cast<int32_t>(val);
      }
      else
        streamError();
    }
    else
    {
      streamError();
    }
  }

  input_buffer.add(static_cast<int32_t>(val));
  return static_cast<int32_t>(val);
}

void
FSTProcessor::printSAOWord(UStringView lf, UFILE *output)
{
  for(unsigned int i = 1, limit = lf.size(); i != limit; i++)
  {
    if(lf[i] == '/')
    {
      break;
    }
    u_fputc(lf[i], output);
  }
}

void
FSTProcessor::SAO(InputFile& input, UFILE *output)
{
  bool last_incond = false;
  bool last_postblank = false;
  State current_state = initial_state;
  UString lf;
  UString sf;
  int last = 0;

  escaped_chars.clear();
  escaped_chars.insert('\\');
  escaped_chars.insert('<');
  escaped_chars.insert('>');

  while(UChar32 val = readSAO(input))
  {
    // test for final states
    if(current_state.isFinal(all_finals))
    {
      if(current_state.isFinal(inconditional))
      {
        bool firstupper = u_isupper(sf[0]);
        bool uppercase = firstupper && u_isupper(sf[sf.size()-1]);

        lf = current_state.filterFinalsSAO(all_finals, alphabet,
                                        escaped_chars,
                                        uppercase, firstupper);
        last_incond = true;
        last = input_buffer.getPos();
      }
      else if(current_state.isFinal(postblank))
      {
        bool firstupper = u_isupper(sf[0]);
        bool uppercase = firstupper && u_isupper(sf[sf.size()-1]);

        lf = current_state.filterFinalsSAO(all_finals, alphabet,
                                        escaped_chars,
                                        uppercase, firstupper);
        last_postblank = true;
        last = input_buffer.getPos();
      }
      else if(!isAlphabetic(val))
      {
        bool firstupper = u_isupper(sf[0]);
        bool uppercase = firstupper && u_isupper(sf[sf.size()-1]);

        lf = current_state.filterFinalsSAO(all_finals, alphabet,
                                        escaped_chars,
                                        uppercase, firstupper);
        last_postblank = false;
        last_incond = false;
        last = input_buffer.getPos();
      }
    }
    else if(sf.empty() && u_isspace(val))
    {
      lf = "/*"_u;
      lf.append(sf);
      last_postblank = false;
      last_incond = false;
      last = input_buffer.getPos();
    }

    current_state.step_case(val, beCaseSensitive(current_state));

    if(current_state.size() != 0)
    {
      alphabet.getSymbol(sf, val);
    }
    else
    {
      if(!isAlphabetic(val) && sf.empty())
      {
        if(u_isspace(val))
        {
          printSpace(val, output);
        }
        else
        {
          if(isEscaped(val))
          {
            u_fputc('\\', output);
          }
          u_fputc(val, output);
        }
      }
      else if(last_incond)
      {
        printSAOWord(lf, output);
        input_buffer.setPos(last);
        input_buffer.back(1);
      }
      else if(last_postblank)
      {
        printSAOWord(lf, output);
        u_fputc(' ', output);
        input_buffer.setPos(last);
        input_buffer.back(1);
      }
      else if(isAlphabetic(val) &&
              ((sf.size()-input_buffer.diffPrevPos(last)) > lastBlank(sf) ||
               lf.empty()))
      {
        do
        {
          alphabet.getSymbol(sf, val);
        }
        while((val = readSAO(input)) && isAlphabetic(val));

        auto limit = firstNotAlpha(sf);
        unsigned int size = sf.size(); // TODO: change these to character counts
        input_buffer.back(1+(size-limit.i_utf16));
        u_fprintf(output, "<d>%S</d>", sf.c_str());
      }
      else if(lf.empty())
      {
        auto limit = firstNotAlpha(sf);
        unsigned int size = sf.size(); // TODO: change these to character counts
        input_buffer.back(1+(size-limit.i_utf16));
        u_fprintf(output, "<d>%S</d>", sf.c_str());
      }
      else
      {
        printSAOWord(lf, output);
        input_buffer.setPos(last);
        input_buffer.back(1);
      }

      current_state = initial_state;
      lf.clear();
      sf.clear();
      last_incond = false;
      last_postblank = false;
    }
  }

  // print remaining blanks
  flushBlanks(output);
}

UStringView
FSTProcessor::removeTags(UStringView str)
{
  for(unsigned int i = 0; i < str.size(); i++)
  {
    if(str[i] == '<' && i >=1 && str[i-1] != '\\')
    {
      return str.substr(0, i);
    }
  }

  return str;
}


void
FSTProcessor::setBiltransSurfaceForms(bool value)
{
  biltransSurfaceForms = value;
}

void
FSTProcessor::setBiltransSurfaceFormsKeep(bool value)
{
  biltransSurfaceFormsKeep = value;
}

void
FSTProcessor::setCaseSensitiveMode(bool value)
{
  caseSensitive = value;
}

void
FSTProcessor::setDictionaryCaseMode(bool value)
{
  dictionaryCase = value;
}

void
FSTProcessor::setNullFlush(bool value)
{
  nullFlush = value;
}

void
FSTProcessor::setIgnoredChars(bool value)
{
  useIgnoredChars = value;
}

void
FSTProcessor::setRestoreChars(bool value)
{
  useRestoreChars = value;
}

void
FSTProcessor::setUseDefaultIgnoredChars(bool value)
{
  useDefaultIgnoredChars = value;
}

void
FSTProcessor::setDisplayWeightsMode(bool value)
{
  displayWeightsMode = value;
}

void
FSTProcessor::setMaxAnalysesValue(int value)
{
  maxAnalyses = value;
}

void
FSTProcessor::setMaxWeightClassesValue(int value)
{
  maxWeightClasses = value;
}

void
FSTProcessor::setCompoundMaxElements(int value)
{
  compound_max_elements = value;
}

bool
FSTProcessor::getDecompoundingMode()
{
  return do_decomposition;
}

bool
FSTProcessor::getNullFlush()
{
  return nullFlush;
}

FSTProcessor::Indices
FSTProcessor::firstNotAlpha(UStringView sf)
{
  FSTProcessor::Indices ix = { 0, 0 };
  UCharCharacterIterator it = UCharCharacterIterator(sf.data(), sf.size());
  while (it.hasNext()) {
    UChar32 c = it.next32PostInc();
    if(!isAlphabetic(c))
    {
      return ix;
    }
    ix.i_codepoint++;
    ix.i_utf16++;
    if(c > UINT16_MAX) {
      ix.i_utf16++;
    }
  }
  return ix;
}
