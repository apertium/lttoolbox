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
#include <lttoolbox/string_utils.h>

#include <iostream>
#include <cerrno>
#include <climits>


UString const FSTProcessor::XML_TEXT_NODE           = "#text"_u;
UString const FSTProcessor::XML_COMMENT_NODE        = "#comment"_u;
UString const FSTProcessor::XML_IGNORED_CHARS_ELEM  = "ignored-chars"_u;
UString const FSTProcessor::XML_RESTORE_CHAR_ELEM   = "restore-char"_u;
UString const FSTProcessor::XML_RESTORE_CHARS_ELEM  = "restore-chars"_u;
UString const FSTProcessor::XML_VALUE_ATTR          = "value"_u;
UString const FSTProcessor::XML_CHAR_ELEM           = "char"_u;
UString const FSTProcessor::WBLANK_START            = "[["_u;
UString const FSTProcessor::WBLANK_END              = "]]"_u;
UString const FSTProcessor::WBLANK_FINAL            = "[[/]]"_u;


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

bool
FSTProcessor::wblankPostGen(InputFile& input, UFILE *output)
{
  UString result = WBLANK_START;
  UChar32 c = 0;
  bool in_content = false;

  while(!input.eof())
  {
    c = input.get();
    if(in_content && c == '~')
    {
      if(result[result.size()-1] == ']') {
        // We just saw the end of a wblank, may want to merge
        wblankqueue.push_back(result);
      }
      else {
        // wake-up-mark happened some characters into the wblanked word
        write(result, output);
      }
      return true;
    }
    else
    {
      result += c;
    }

    if(c == '\\')
    {
      if (input.eof()) streamError();
      result += input.get();
    }
    else if(c == ']')
    {
      c = input.get();
      result += c;

      if(c == ']')
      {
        int resultlen = result.size();
        if(result[resultlen-5] == '[' && result[resultlen-4] == '[' && result[resultlen-3] == '/') //ending blank [[/]]
        {
          write(result, output);
          break;
        }
        else
        {
          in_content = true; // Assumption: No nested wblanks, always balanced
        }
      }
    }
  }

  if(c != ']')
  {
    streamError();
  }

  return false;
}

int
FSTProcessor::readAnalysis(InputFile& input)
{
  if (!input_buffer.isEmpty())
  {
    UChar32 val = input_buffer.next();
    while ((useIgnoredChars || useDefaultIgnoredChars) && ignored_chars.find(val) != ignored_chars.end()) {
      val = input_buffer.next();
    }
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
    input_buffer.add(val);
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
          input_buffer.add(alphabet("<n>"_u));
          numbers.push_back(ws);
          return alphabet("<n>"_u);
        }
        break;

      default:
        streamError();
    }
  }

  input_buffer.add(val);
  return val;
}

int32_t
FSTProcessor::readPostgeneration(InputFile& input, UFILE *output)
{
  if(!input_buffer.isEmpty())
  {
    return input_buffer.next();
  }

  UChar32 val = input.get();
  int32_t altval = 0;
  is_wblank = false;
  if(input.eof())
  {
    return 0;
  }

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
        if(collect_wblanks)
        {
          wblankqueue.push_back(input.finishWBlank());
          is_wblank = true;
          return static_cast<int32_t>(' ');
        }
        else if(wblankPostGen(input, output))
        {
          return static_cast<int32_t>('~');
        }
        else
        {
          is_wblank = true;
          return static_cast<int32_t>(' ');
        }
      }
      else
      {
        input.unget(val);
        blankqueue.push(input.readBlock('[', ']'));

        input_buffer.add(static_cast<int32_t>(' '));
        return static_cast<int32_t>(' ');
      }

    case '\\':
      val = input.get();
      input_buffer.add(static_cast<int32_t>(val));
      return val;

    default:
      input_buffer.add(val);
      return val;
  }
}

int32_t
FSTProcessor::readTransliteration(InputFile& input)
{
  if (!input_buffer.isEmpty()) {
    //cerr << "checking for wblank loc at " << input_buffer.getPos() << endl;
    if (wblank_locs.find(input_buffer.getPos()) != wblank_locs.end()) {
      is_wblank = true;
    } else {
      is_wblank = false;
    }
    return input_buffer.next();
  }

  is_wblank = false;
  UChar32 val = input.get();
  int32_t sym = 0;
  if (input.eof()) {
    return 0;
  }

  if (val == '\\') {
    sym = static_cast<int32_t>(input.get());
  } else if (val == '<') {
    sym = alphabet(input.readBlock('<', '>'));
  } else if (val == '[' && input.peek() == '[') {
    sym = static_cast<int32_t>(' ');
    //if (!transliteration_last_was_space) {
    //transliteration_last_was_space = true;
      //blankqueue.push(""_u);
      //input.unget(val);
    //} else {
      input.get();
      wblankqueue.push_back(input.finishWBlank());
      is_wblank = true;
      //}
  } else if (val == '[' || u_isspace(val)) {
    sym = static_cast<int32_t>(' ');
    UString blank;
    while (!input.eof()) {
      if (val == '[') {
        if (input.peek() == '[') {
          input.unget(val);
          break;
        } else {
          blank += input.readBlock('[', ']');
          val = input.get();
        }
      } else if (u_isspace(val)) {
        blank += val;
        val = input.get();
      } else {
        input.unget(val);
        break;
      }
    }
    blankqueue.push(blank);
    transliteration_last_was_space = true;
  } else {
    sym = val;
  }
  if (is_wblank) {
    //cerr << "adding wblank loc " << (input_buffer.getPos()) << endl;
    //input_buffer.next();
    wblank_locs.insert(input_buffer.getPos());
    //input_buffer.back(1);
  }
  input_buffer.add(sym);
  if (sym != static_cast<int32_t>(' ')) transliteration_last_was_space = false;
  return sym;
}

void
FSTProcessor::skipUntil(InputFile& input, UFILE *output, UChar32 const character)
{
  while(true)
  {
    UChar32 val = input.get();
    if(input.eof())
    {
      return;
    }

    switch(val)
    {
      case '\\':
        val = input.get();
        if(input.eof())
        {
          return;
        }
        u_fputc('\\', output);
        u_fputc(val, output);
        break;

      case '\0':
        u_fputc(val, output);
        if(nullFlushGeneration)
        {
          u_fflush(output);
        }
        break;

      default:
        if(val == character)
        {
          return;
        }
        else
        {
          u_fputc(val, output);
        }
        break;
    }
  }
}

int
FSTProcessor::readGeneration(InputFile& input, UFILE *output)
{
  UChar32 val = input.get();

  if(input.eof())
  {
    return 0x7fffffff;
  }

  if(outOfWord)
  {
    if(val == '^')
    {
      val = input.get();
      if(input.eof())
      {
        return 0x7fffffff;
      }
    }
    else if(val == '\\')
    {
      u_fputc(val, output);
      val = input.get();
      if(input.eof())
      {
        return 0x7fffffff;
      }
      u_fputc(val,output);
      skipUntil(input, output, '^');
      val = input.get();
      if(input.eof())
      {
        return 0x7fffffff;
      }
    }
    else
    {
      u_fputc(val, output);
      skipUntil(input, output, '^');
      val = input.get();
      if(input.eof())
      {
        return 0x7fffffff;
      }
    }
    outOfWord = false;
  }

  if(val == '\\')
  {
    val = input.get();
    return static_cast<int32_t>(val);
  }
  else if(val == '$')
  {
    outOfWord = true;
    return static_cast<int32_t>('$');
  }
  else if(val == '<')
  {
    return alphabet(input.readBlock('<', '>'));
  }
  else if(val == '[')
  {
    val = input.get();
    if(val == '[')
    {
      write(input.finishWBlank(), output);
    }
    else
    {
      input.unget(val);
      write(input.readBlock('[', ']'), output);
    }

    return readGeneration(input, output);
  }
  else
  {
    return static_cast<int32_t>(val);
  }

  return 0x7fffffff;
}

std::pair<UString, int>
FSTProcessor::readBilingual(InputFile& input, UFILE *output)
{
  UChar32 val = input.get();
  UString symbol;

  if(input.eof())
  {
    return std::pair<UString, int>(symbol, 0x7fffffff);
  }

  if(outOfWord)
  {
    if(val == '^')
    {
      val = input.get();
      if(input.eof())
      {
        return std::pair<UString, int>(symbol, 0x7fffffff);
      }
    }
    else if(val == '\\')
    {
      u_fputc(val, output);
      val = input.get();
      if(input.eof())
      {
        return std::pair<UString, int>(symbol, 0x7fffffff);
      }
      u_fputc(val,output);
      skipUntil(input, output, '^');
      val = input.get();
      if(input.eof())
      {
        return std::pair<UString, int>(symbol, 0x7fffffff);
      }
    }
    else
    {
      u_fputc(val, output);
      skipUntil(input, output, '^');
      val = input.get();
      if(input.eof())
      {
        return std::pair<UString, int>(symbol, 0x7fffffff);
      }
    }
    outOfWord = false;
  }

  if(val == '\\')
  {
    val = input.get();
    return std::pair<UString, int>(symbol, val);
  }
  else if(val == '$')
  {
    outOfWord = true;
    return std::pair<UString, int>(symbol, static_cast<int32_t>('$'));
  }
  else if(val == '<')
  {
    UString cad = input.readBlock('<', '>');

    int res = alphabet(cad);

    if (res == 0)
    {
      symbol = cad;
    }
    return std::pair<UString, int>(symbol, res);
  }
  else if(val == '[')
  {
    val = input.get();
    if(val == '[')
    {
      write(input.finishWBlank(), output);
    }
    else
    {
      input.unget(val);
      write(input.readBlock('[', ']'), output);
    }

    return readBilingual(input, output);
  }

  return std::pair<UString, int>(symbol, val);
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
FSTProcessor::flushWblanks(UFILE *output)
{
  while(wblankqueue.size() > 0)
  {
    write(wblankqueue.front(), output);
    wblankqueue.pop_front();
  }
}

UString
FSTProcessor::combineWblanks()
{
  UString final_wblank;
  UString last_wblank;
  bool seen_wblank = false;

  while(wblankqueue.size() > 0)
  {
    if(wblankqueue.front().compare(WBLANK_FINAL) == 0)
    {
      if(seen_wblank) {
        if(final_wblank.empty())
        {
          final_wblank += WBLANK_START;
        }
        else if(final_wblank.size() > 2)
        {
          final_wblank += "; "_u;
        }

        final_wblank += last_wblank.substr(2,last_wblank.size()-4); //add wblank without brackets [[..]]
      }
      else {
        need_end_wblank = true;
      }
      last_wblank.clear();
    }
    else
    {
      seen_wblank = true;
      last_wblank = wblankqueue.front();
    }
    wblankqueue.pop_front();
  }

  if(!last_wblank.empty())
  {
    wblankqueue.push_back(last_wblank);
  }

  if(!final_wblank.empty())
  {
    final_wblank += WBLANK_END;
    need_end_wblank = true;
  }
  return final_wblank;
}

void
FSTProcessor::calcInitial()
{
  for(auto& it : transducers) {
    root.addTransition(0, 0, it.second.getInitial(), default_weight);
  }

  initial_state.init(&root);
}

bool
FSTProcessor::endsWith(UString const &str, UString const &suffix)
{
  if(str.size() < suffix.size())
  {
    return false;
  }
  else
  {
    return str.substr(str.size()-suffix.size()) == suffix;
  }
}

void
FSTProcessor::classifyFinals()
{
  for(auto& it : transducers) {
    if(endsWith(it.first, "@inconditional"_u))
    {
      inconditional.insert(it.second.getFinals().begin(),
                           it.second.getFinals().end());
    }
    else if(endsWith(it.first, "@standard"_u))
    {
      standard.insert(it.second.getFinals().begin(),
                      it.second.getFinals().end());
    }
    else if(endsWith(it.first, "@postblank"_u))
    {
      postblank.insert(it.second.getFinals().begin(),
                       it.second.getFinals().end());
    }
    else if(endsWith(it.first, "@preblank"_u))
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
FSTProcessor::filterFinals(const State& state, const UString& casefrom)
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
FSTProcessor::writeEscaped(UString const &str, UFILE *output)
{
  for (auto& c : str) {
    putc_esc(c, output);
  }
}

size_t
FSTProcessor::writeEscapedPopBlanks(UString const &str, UFILE *output)
{
  size_t postpop = 0;
  for (auto& c : str) {
    putc_esc(c, output);
    if (c == ' ') {
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
FSTProcessor::writeEscapedWithTags(UString const &str, UFILE *output)
{
  for(unsigned int i = 0, limit = str.size(); i < limit; i++)
  {
    if(str[i] == '<' && i >=1 && str[i-1] != '\\')
    {
      write(str.substr(i), output);
      return;
    }

    putc_esc(str[i], output);
  }
}



void
FSTProcessor::printWord(UString const &sf, UString const &lf, UFILE *output)
{
  u_fputc('^', output);
  writeEscaped(sf, output);
  write(lf, output);
  u_fputc('$', output);
}

void
FSTProcessor::printWordPopBlank(UString const &sf, UString const &lf, UFILE *output)
{
  u_fputc('^', output);
  size_t postpop = writeEscapedPopBlanks(sf, output);
  u_fprintf(output, "%S$", lf.c_str());
  while (postpop-- && blankqueue.size() > 0)
  {
    write(blankqueue.front(), output);
    blankqueue.pop();
  }
}

void
FSTProcessor::printWordBilingual(UString const &sf, UString const &lf, UFILE *output)
{
  u_fprintf(output, "^%S%S$", sf.c_str(), lf.c_str());
}

void
FSTProcessor::printUnknownWord(UString const &sf, UFILE *output)
{
  u_fputc('^', output);
  writeEscaped(sf, output);
  u_fputc('/', output);
  u_fputc('*', output);
  writeEscaped(sf, output);
  u_fputc('$', output);
}

unsigned int
FSTProcessor::lastBlank(UString const &str)
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
FSTProcessor::putc_esc(const UChar32 val, UFILE* output)
{
  if (val != '\0') {
    if (isEscaped(val)) {
      u_fputc('\\', output);
    }
    u_fputc(val, output);
  }
}

void
FSTProcessor::printSpace(UChar32 const val, UFILE *output, bool flush)
{
  if(blankqueue.size() > 0)
  {
    if (flush) {
      flushBlanks(output);
    } else {
      write(blankqueue.front(), output);
      blankqueue.pop();
    }
  }
  else
  {
    u_fputc(val, output);
  }
}

void
FSTProcessor::printChar(const UChar32 val, UFILE* output, bool flush)
{
  if (u_isspace(val)) {
    printSpace(val, output, flush);
  } else {
    putc_esc(val, output);
  }
}

bool
FSTProcessor::isEscaped(UChar32 const c) const
{
  return escaped_chars.find(c) != escaped_chars.end();
}

bool
FSTProcessor::isAlphabetic(UChar32 const c) const
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
FSTProcessor::initPostgeneration()
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

    current_state.step_case(val, caseSensitive);

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
  if((compoundOnlyLSymbol=alphabet("<:co:only-L>"_u)) == 0
     && (compoundOnlyLSymbol=alphabet("<:compound:only-L>"_u)) == 0
     && (compoundOnlyLSymbol=alphabet("<@co:only-L>"_u)) == 0
     && (compoundOnlyLSymbol=alphabet("<@compound:only-L>"_u)) == 0
     && (compoundOnlyLSymbol=alphabet("<compound-only-L>"_u)) == 0)
  {
    std::cerr << "Warning: Decomposition symbol <:compound:only-L> not found" << std::endl;
  }
  else if(!showControlSymbols)
  {
    alphabet.setSymbol(compoundOnlyLSymbol, ""_u);
  }

  if((compoundRSymbol=alphabet("<:co:R>"_u)) == 0
     && (compoundRSymbol=alphabet("<:compound:R>"_u)) == 0
     && (compoundRSymbol=alphabet("<@co:R>"_u)) == 0
     && (compoundRSymbol=alphabet("<@compound:R>"_u)) == 0
     && (compoundRSymbol=alphabet("<compound-R>"_u)) == 0)
  {
    std::cerr << "Warning: Decomposition symbol <:compound:R> not found" << std::endl;
  }
  else if(!showControlSymbols)
  {
    alphabet.setSymbol(compoundRSymbol, ""_u);
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
      if(!u_isupper(val) || caseSensitive)
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
      current_state.step_case(val, caseSensitive);
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

        unsigned int limit = firstNotAlpha(sf);
        unsigned int size = sf.size();
        limit = (limit == static_cast<unsigned int>(UString::npos)?size:limit);
        if(limit == 0)
        {
          input_buffer.back(sf.size());
          writeEscaped(sf.substr(0,1), output);
        }
        else
        {
          input_buffer.back(1+(size-limit));
          UString unknown_word = sf.substr(0, limit);
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
        unsigned int limit = firstNotAlpha(sf);
        unsigned int size = sf.size();
        limit = (limit == static_cast<unsigned int >(UString::npos)?size:limit);
        if(limit == 0)
        {
          input_buffer.back(sf.size());
          writeEscaped(sf.substr(0,1), output);
        }
        else
        {
          input_buffer.back(1+(size-limit));
          UString unknown_word = sf.substr(0, limit);
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
FSTProcessor::postgeneration_wrapper_null_flush(InputFile& input, UFILE *output)
{
  setNullFlush(false);
  while(!input.eof())
  {
    postgeneration(input, output);
    u_fputc('\0', output);
    u_fflush(output);
  }
}

void
FSTProcessor::intergeneration_wrapper_null_flush(InputFile& input, UFILE *output)
{
  setNullFlush(false);
  while (!input.eof())
  {
    intergeneration(input, output);
    u_fputc('\0', output);
    u_fflush(output);
  }
}

void
FSTProcessor::transliteration_wrapper_null_flush(InputFile& input, UFILE *output)
{
  setNullFlush(false);
  while(!input.eof())
  {
    transliteration(input, output);
    u_fputc('\0', output);
    u_fflush(output);
  }
}

void
FSTProcessor::tm_analysis(InputFile& input, UFILE *output)
{
  State current_state = initial_state;
  UString lf;     //lexical form
  UString sf;     //surface form
  int last = 0;

  while(int32_t val = readTMAnalysis(input))
  {
    // test for final states
    if(current_state.isFinal(all_finals))
    {
      if(u_ispunct(val))
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
        printChar(val, output, true);
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
    }
  }

  // print remaining blanks
  flushBlanks(output);
}


void
FSTProcessor::generation(InputFile& input, UFILE *output, GenerationMode mode)
{
  if(getNullFlush())
  {
    generation_wrapper_null_flush(input, output, mode);
  }

  State current_state = initial_state;
  UString sf;

  outOfWord = false;

  skipUntil(input, output, '^');
  int val;

  while((val = readGeneration(input, output)) != 0x7fffffff)
  {
    if(sf.empty() && val == '=')
    {
      u_fputc('=', output);
      val = readGeneration(input, output);
    }

    if(val == '$' && outOfWord)
    {
      if(sf[0] == '*' || sf[0] == '%')
      {
        if(mode != gm_clean && mode != gm_tagged_nm)
        {
          writeEscaped(sf, output);
        }
        else if (mode == gm_clean)
        {
          writeEscaped(sf.substr(1), output);
        }
        else if(mode == gm_tagged_nm)
        {
          u_fputc('^', output);
          writeEscaped(removeTags(sf.substr(1)), output);
          u_fputc('/', output);
          writeEscapedWithTags(sf, output);
          u_fputc('$', output);
        }
      }
      else if(sf[0] == '@')
      {
        if(mode == gm_all)
        {
          writeEscaped(sf, output);
        }
        else if(mode == gm_clean)
        {
          writeEscaped(removeTags(sf.substr(1)), output);
        }
        else if(mode == gm_unknown)
        {
          writeEscaped(removeTags(sf), output);
        }
        else if(mode == gm_tagged)
        {
          writeEscaped(removeTags(sf), output);
        }
        else if(mode == gm_tagged_nm)
        {
          u_fputc('^', output);
          writeEscaped(removeTags(sf.substr(1)), output);
          u_fputc('/', output);
          writeEscapedWithTags(sf, output);
          u_fputc('$', output);
        }
      }
      else if(current_state.isFinal(all_finals))
      {
        bool firstupper = false, uppercase = false;
        if(!dictionaryCase)
        {
          uppercase = sf.size() > 1 && u_isupper(sf[1]);
          firstupper= u_isupper(sf[0]);
        }

        if(mode == gm_tagged || mode == gm_tagged_nm)
        {
          u_fputc('^', output);
        }

        write(current_state.filterFinals(all_finals, alphabet,
                                         escaped_chars,
                                         displayWeightsMode, maxAnalyses, maxWeightClasses,
                                         uppercase, firstupper).substr(1), output);
        if(mode == gm_tagged || mode == gm_tagged_nm)
        {
          u_fputc('/', output);
          writeEscapedWithTags(sf, output);
          u_fputc('$', output);
        }

      }
      else
      {
        if(mode == gm_all)
        {
          u_fputc('#', output);
          writeEscaped(sf, output);
        }
        else if(mode == gm_clean)
        {
          writeEscaped(removeTags(sf), output);
        }
        else if(mode == gm_unknown)
        {
          if(!sf.empty())
          {
            u_fputc('#', output);
            writeEscaped(removeTags(sf), output);
          }
        }
        else if(mode == gm_tagged)
        {
          u_fputc('#', output);
          writeEscaped(removeTags(sf), output);
        }
        else if(mode == gm_tagged_nm)
        {
          u_fputc('^', output);
          writeEscaped(removeTags(sf), output);
          u_fputc('/', output);
          u_fputc('#', output);
          writeEscapedWithTags(sf, output);
          u_fputc('$', output);
        }
      }

      current_state = initial_state;
      sf.clear();
    }
    else if(u_isspace(val) && sf.size() == 0)
    {
      // do nothing
    }
    else if(sf.size() > 0 && (sf[0] == '*' || sf[0] == '%' ))
    {
      alphabet.getSymbol(sf, val);
    }
    else
    {
      alphabet.getSymbol(sf,val);
      if(current_state.size() > 0)
      {
        if(!alphabet.isTag(val) && u_isupper(val) && !caseSensitive)
        {
          if(mode == gm_carefulcase)
          {
            current_state.step_careful(val, u_tolower(val));
          }
          else
          {
            current_state.step(val, u_tolower(val));
          }
        }
        else
        {
          current_state.step(val);
        }
      }
    }
  }
}

void
FSTProcessor::postgeneration(InputFile& input, UFILE *output)
{
  //transliteration_reread_suffix = true;
  transliteration_drop_tilde = true;
  transliteration(input, output);
}

void
FSTProcessor::intergeneration(InputFile& input, UFILE *output)
{
  transliteration_reread_suffix = false;
  transliteration_drop_tilde = false;
  transliteration(input, output);
}

size_t
common_suffix(const UString& a, const UString& b)
{
  size_t ret = 0;
  while (ret < a.size() && ret < b.size()) {
    if (a[a.size() - 1 - ret] == b[b.size() - 1 - ret]) {
      ret++;
    } else {
      ret--;
      break;
    }
  }
  return ret;
}

UString
merge_wblanks(const UString& a, const UString& b)
{
  if (a.empty()) return b;
  if (b.empty()) return a;
  UString ret = a.substr(0, a.size()-2);
  ret += "; "_u;
  ret += b.substr(2);
  return ret;
}

void
debug_zone(const std::pair<std::pair<int, int>, bool>& zone)
{
  std::cerr << "(" << zone.first.first << "," << zone.first.second << "," << zone.second << ")";
}

bool
merge_zones(std::deque<std::pair<std::pair<int, int>, bool>>& zones,
            std::deque<UString>& wblanks, int cur_pos)
{
  //cerr << "  merge_zones(" << cur_pos << ")\n";
  if (zones.empty()) return false;
  auto current_zone = zones.front();
  zones.pop_front();
  UString wblank;
  if (current_zone.second) {
    wblank = wblanks.front();
    wblanks.pop_front();
  }
  //cerr << "    current ";
  //debug_zone(current_zone);
  //cerr << " wblank " << wblank << endl;
  bool skip = false;
  while (!zones.empty() && zones.front().first.first < cur_pos) {
    if (current_zone.second &&
        !zones.front().second &&
        zones.front().first.second == cur_pos &&
        zones.front().first.first == cur_pos-1) {
      //cerr << "    skipping following blank?\n";
      skip = true;
      break;
    }
    //cerr << "    adding ";
    //debug_zone(zones.front());
    //cerr << endl;
    if (zones.front().second) {
      wblank = merge_wblanks(wblank, wblanks.front());
      wblanks.pop_front();
      current_zone.second = true;
    }
    current_zone.first.second = zones.front().first.second;
    zones.pop_front();
  }
  if (current_zone.second) {
    wblanks.push_front(wblank);
  }
  zones.push_front(current_zone);
  return skip;
}

int
count_spaces(const UString& s)
{
  int c = 0;
  for (size_t i = 0; i < s.size(); i++) {
    if (s[i] == ' ') c++;
  }
  return c;
}

UString
next_space(std::queue<UString>& blankqueue, size_t& skip)
{
  while (skip > 0 && !blankqueue.empty() && blankqueue.front() == " "_u) {
    blankqueue.pop();
    skip--;
  }
  if (blankqueue.empty()) return " "_u;
  UString ret = blankqueue.front();
  blankqueue.pop();
  while (skip > 0 && !blankqueue.empty() && blankqueue.front() == " "_u) {
    blankqueue.pop();
    skip--;
  }
  return ret;
}

void
FSTProcessor::transliteration(InputFile& input, UFILE *output)
{
  if(getNullFlush())
  {
    transliteration_wrapper_null_flush(input, output);
  }

  State current_state = initial_state;
  UString lf;
  UString sf;
  UString last_lf;
  UString prefix;
  int rewind_point = 0;
  int last_match = 0;
  int32_t firstchar = 0;
  size_t spaces_to_skip = 0;
  UString last_sf;

  std::deque<std::pair<std::pair<int, int>, bool>> zones;
  std::deque<UString> local_wblanks;
  int last_zone_start = input_buffer.getPos();
  bool last_zone_is_wblank = false;
  std::pair<std::pair<int, int>, bool> prev_zone =
    std::make_pair(std::make_pair(0, 0), false);

  int32_t val = 0;
  bool has_stepped = false;

  while (true) {
    val = readTransliteration(input);
    if (val == 0) {
      //cerr << "EOF sf.size() == " << sf.size() << " prefix.size() == " << prefix.size() << endl;
    }
    if (val == 0 && sf.empty()) {
      write(prefix, output);
      prefix.clear();
      while (!blankqueue.empty()) {
        if (blankqueue.front() != " "_u) {
          write(blankqueue.front(), output);
        }
        blankqueue.pop();
      }
      spaces_to_skip = 0;
      if (input.eof()) break;
      else {
        u_fputc('\0', output);
        u_fflush(output);
        lf.clear();
        sf.clear();
        last_lf.clear();
        last_sf.clear();
        current_state = initial_state;
        rewind_point = input_buffer.getPos();
        last_match = input_buffer.getPos();
        firstchar = 0;
        spaces_to_skip = 0;
        zones.clear();
        local_wblanks.clear();
        last_zone_start = input_buffer.getPos();
        last_zone_is_wblank = false;
        prev_zone = std::make_pair(std::make_pair(last_zone_start,
                                                  last_zone_start),
                                   false);
        has_stepped = false;
        continue;
      }
    }
    //cerr << "val = " << val << " prefix = " << prefix << " last_sf = " << last_sf << " last_lf = " <<last_lf << " sf = " << sf;
    //cerr << " rewind = " << rewind_point << " match = " << last_match;
    //cerr << " pos = " << input_buffer.getPos();
    //if (!zones.empty()) {
      //cerr << " zone1 ";
      //debug_zone(zones.front());
    //}
    //cerr << " space_skip = " << spaces_to_skip;
    //cerr << " blanks = " << blankqueue.size();
    //cerr << " is_wblank = " << is_wblank;
    //cerr << " pz "; debug_zone(prev_zone);
    //cerr << endl;
    if (val == 0 ||                                      // this is EOF or
        (val == static_cast<int32_t>(' ') &&             // this is a boundary
         (int)input_buffer.getPos() > last_zone_start && // and it's new
         (is_wblank || !last_zone_is_wblank))) {         // and it's not a space
                                                         // inside a wblank
      int end = input_buffer.getPos();
      zones.push_back(std::make_pair(std::make_pair(last_zone_start, end),
                                     last_zone_is_wblank));
      //cerr << "  found end of zone " << last_zone_start << "-" << end << endl;
      if (!last_lf.empty() && last_match > last_zone_start) {
        merge_zones(zones, local_wblanks, last_match);
      }
      last_zone_start = end;
      if (is_wblank) {
        if (last_zone_is_wblank) {
          last_zone_is_wblank = false;
          if (wblankqueue.front() == WBLANK_FINAL) {
            wblankqueue.pop_front();
          } else {
            // there should have been a [[/]] in this situation, so just
            // pretend there's one immediately before the new wblank
            last_zone_is_wblank = true;
            local_wblanks.push_back(wblankqueue.front());
            wblankqueue.pop_front();
          }
        } else {
          last_zone_is_wblank = true;
          local_wblanks.push_back(wblankqueue.front());
          wblankqueue.pop_front();
        }
      }
    } else if (last_zone_start == -1) {
      last_zone_start = input_buffer.getPos();
    }

    if (sf.empty()) {
      if (!is_wblank) {
        firstchar = val;
      }
        rewind_point = input_buffer.getPos();
    } else {
      lf = filterFinals(current_state, sf);
      if (!lf.empty() && !(lf.substr(1) == sf) && sf != last_sf) {
        last_match = input_buffer.getPos();
        last_lf.swap(lf);
        last_sf = sf;
        merge_zones(zones, local_wblanks, last_match);
      }
    }
    if (val != 0 && !is_wblank) {
      current_state.step(val);
      has_stepped = true;
      alphabet.getSymbol(sf, val);
    }
    if (current_state.size() == 0 ||
        val == 0 ||
        (!has_stepped && is_wblank)
        ) {
      //cerr << "  done with something\n";
      if (last_lf.empty()) {
        bool at_boundary = false;
        if (val == static_cast<int32_t>(' ')) {
          //cerr << "    rewind = " << rewind_point << endl;
          for (auto& it : zones) {
            //cerr << "    "; debug_zone(it); cerr<<endl;
            if (it.first.second == rewind_point) {
              at_boundary = true;
              break;
            }
          }
        } else if (val == 0) {
          at_boundary = true;
        }
        if (is_wblank && prefix.empty()) continue;
        if (at_boundary && !(is_wblank && prefix.empty())) {
          //cerr << "  and we're at a boundary\n";
          //cerr << "    prefix = " << prefix << endl;
          bool add_blank = merge_zones(zones, local_wblanks, rewind_point);
          int len = zones.front().first.second - zones.front().first.first;
          bool has_wblank = zones.front().second;
          if (len > 1 || (len == 1 && !blankqueue.empty() && prefix.empty()) || !prefix.empty()) {
            if (is_wblank && rewind_point < zones.front().first.first) rewind_point = input_buffer.getPos();
            prev_zone = zones.front();
            add_blank = (prev_zone.first.second == input_buffer.getPos() &&
                         prev_zone.second && !is_wblank);
            zones.pop_front();
            if (has_wblank) {
              write(local_wblanks.front(), output);
              local_wblanks.pop_front();
            }
            write(prefix, output);
            prefix.clear();
            if (has_wblank) {
              write(WBLANK_FINAL, output);
              if (add_blank) {
                write(next_space(blankqueue, spaces_to_skip), output);
                prev_zone = zones.front();
                zones.pop_front();
              }
            } else {
              if (!((is_wblank || val == 0) && blankqueue.empty())) {
                write(next_space(blankqueue, spaces_to_skip), output);
              }
            }
          }
        } else {
          if (firstchar != static_cast<int32_t>('~') ||
              !transliteration_drop_tilde) {
            if (firstchar != static_cast<int32_t>(' ')) {
              if (isEscaped(firstchar)) {
                prefix += '\\';
              }
              alphabet.getSymbol(prefix, firstchar);
            } else if (!is_wblank) {
              prefix += next_space(blankqueue, spaces_to_skip);
            }
          }
        }
        //cerr << "  rewinding from " << input_buffer.getPos();
        input_buffer.setPos(rewind_point);
        //cerr << " to " << input_buffer.getPos() << endl;
        if (val == 0 && prefix.empty() && sf.empty()) {
          input_buffer.back(1);
        }
      } else {
        merge_zones(zones, local_wblanks, last_match);
        input_buffer.setPos(last_match);
        input_buffer.back(1);
        if (transliteration_reread_suffix) {
          size_t suf_len = common_suffix(last_lf, last_sf);
          //cerr << "    suf_len = " << suf_len <<endl;
          last_lf = last_lf.substr(0, last_lf.size()-suf_len);
          last_sf = last_sf.substr(0, last_sf.size()-suf_len);
          input_buffer.back(suf_len);
        }
        //cerr << "  moving rewind_point from " << rewind_point;
        rewind_point = input_buffer.getPos();
        //cerr << " to " << rewind_point << endl;
        if (last_sf[last_sf.size()-1] == ' ' &&
            last_lf[last_lf.size()-1] == ' ') {
          last_sf = last_sf.substr(0, last_sf.size()-1);
          last_lf = last_lf.substr(0, last_lf.size()-1);
          last_match--;
          input_buffer.back(1);
        }
        int sf_space_count = count_spaces(last_sf);
        int lf_space_count = count_spaces(last_lf);
        int spaces_printed = 0;
        if (sf_space_count > lf_space_count) {
          spaces_to_skip += (sf_space_count - lf_space_count);
        }
        //cerr << "    spaces: sf " << sf_space_count << " lf " <<lf_space_count << endl;
        for (auto c : last_lf.substr(1)) {
          if (c == '~' && transliteration_drop_tilde) {
            continue;
          }
          if (u_isspace(c)) {
            if (spaces_printed >= sf_space_count) {
              prefix += c;
            } else {
              prefix += next_space(blankqueue, spaces_to_skip);
            }
            spaces_printed++;
          } else {
            if (isEscaped(c)) prefix += '\\';
            prefix += c;
          }
        }
      }
      current_state = initial_state;
      has_stepped = false;
      sf.clear();
      lf.clear();
      last_lf.clear();
      last_sf.clear();
      if (wblankqueue.empty() && local_wblanks.empty()) {
        wblank_locs.clear();
      }
    }
  }

  // print remaining blanks
  flushBlanks(output);
}

UString
FSTProcessor::biltransfull(UString const &input_word, bool with_delim)
{
  State current_state = initial_state;
  UString result;
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
    return input_word;
  }

  if(input_word[start_point] == '=')
  {
    start_point++;
    mark = true;
  }

  bool firstupper = u_isupper(input_word[start_point]);
  bool uppercase = firstupper && u_isupper(input_word[start_point+1]);

  for(unsigned int i = start_point; i <= end_point; i++)
  {
    int val;
    UString symbol;

    if(input_word[i] == '\\')
    {
      i++;
      val = static_cast<int32_t>(input_word[i]);
    }
    else if(input_word[i] == '<')
    {
      symbol = '<';
      for(unsigned int j = i + 1; j <= end_point; j++)
      {
        symbol += input_word[j];
        if(input_word[j] == '>')
        {
          i = j;
          break;
        }
      }
      val = alphabet(symbol);
    }
    else
    {
      val = static_cast<int32_t>(input_word[i]);
    }
    if(current_state.size() != 0)
    {
      if(!alphabet.isTag(val) && u_isupper(val) && !caseSensitive)
      {
        current_state.step(val, u_tolower(val));
      }
      else
      {
        current_state.step(val);
      }
    }
    if(current_state.isFinal(all_finals))
    {
      result.clear();
      if(with_delim) {
        result += '^';
      }
      if(mark) {
        result += '=';
      }
      result += current_state.filterFinals(all_finals, alphabet,
                                           escaped_chars,
                                           displayWeightsMode, maxAnalyses, maxWeightClasses,
                                           uppercase, firstupper, 0).substr(1);
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
          result = "^@"_u + input_word.substr(1);
        }
        else
        {
          result = "@"_u + input_word;
        }
        return result;
      }
    }
  }

  if(start_point < (end_point - 3))
  {
    return "^$"_u;
  }
  // attach unmatched queue automatically

  if(!queue.empty())
  {
    UString result_with_queue;
    for(unsigned int i = 0, limit = result.size(); i != limit; i++)
    {
      switch(result[i])
      {
        case '\\':
          result_with_queue += '\\';
          i++;
          break;

        case '/':
          result_with_queue.append(queue);
          break;

        default:
          break;
      }
      result_with_queue += result[i];
    }
    result_with_queue.append(queue);

    if(with_delim)
    {
      result_with_queue += '$';
    }
    return result_with_queue;
  }
  else
  {
    if(with_delim)
    {
      result += '$';
    }
    return result;
  }
}



UString
FSTProcessor::biltrans(UString const &input_word, bool with_delim)
{
  State current_state = initial_state;
  UString result;
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
    return input_word;
  }

  if(input_word[start_point] == '=')
  {
    start_point++;
    mark = true;
  }

  bool firstupper = u_isupper(input_word[start_point]);
  bool uppercase = firstupper && u_isupper(input_word[start_point+1]);

  for(unsigned int i = start_point; i <= end_point; i++)
  {
    int val;
    UString symbol;

    if(input_word[i] == '\\')
    {
      i++;
      val = static_cast<int32_t>(input_word[i]);
    }
    else if(input_word[i] == '<')
    {
      symbol = '<';
      for(unsigned int j = i + 1; j <= end_point; j++)
      {
        symbol += input_word[j];
        if(input_word[j] == '>')
        {
          i = j;
          break;
        }
      }
      val = alphabet(symbol);
    }
    else
    {
      val = static_cast<int32_t>(input_word[i]);
    }
    if(current_state.size() != 0)
    {
      if(!alphabet.isTag(val) && u_isupper(val) && !caseSensitive)
      {
        current_state.step(val, u_tolower(val));
      }
      else
      {
        current_state.step(val);
      }
    }
    if(current_state.isFinal(all_finals))
    {
      result.clear();
      if (with_delim) {
        result += '^';
      }
      if (mark) {
        result += '=';
      }
      result += current_state.filterFinals(all_finals, alphabet,
                                           escaped_chars,
                                           displayWeightsMode, maxAnalyses, maxWeightClasses,
                                           uppercase, firstupper, 0).substr(1);
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
          result = "^@"_u + input_word.substr(1);
        }
        else
        {
          result = "@"_u + input_word;
        }
        return result;
      }
    }
  }

  // attach unmatched queue automatically

  if(!queue.empty())
  {
    UString result_with_queue;
    for(unsigned int i = 0, limit = result.size(); i != limit; i++)
    {
      switch(result[i])
      {
        case '\\':
          result_with_queue += '\\';
          i++;
          break;

        case '/':
          result_with_queue.append(queue);
          break;

        default:
          break;
      }
      result_with_queue += result[i];
    }
    result_with_queue.append(queue);

    if(with_delim)
    {
      result_with_queue += '$';
    }
    return result_with_queue;
  }
  else
  {
    if(with_delim)
    {
      result += '$';
    }
    return result;
  }
}

void
FSTProcessor::bilingual_wrapper_null_flush(InputFile& input, UFILE *output, GenerationMode mode)
{
  setNullFlush(false);
  nullFlushGeneration = true;

  while(!input.eof())
  {
    bilingual(input, output, mode);
    u_fputc('\0', output);
    u_fflush(output);
  }
}

UString
FSTProcessor::compose(UString const &lexforms, UString const &queue) const
{
  UString result;
  result.reserve(lexforms.size() + 2 * queue.size());
  result += '/';

  for(unsigned int i = 1; i< lexforms.size(); i++)
  {
    if(lexforms[i] == '\\')
    {
      result += '\\';
      i++;
    }
    else if(lexforms[i] == '/')
    {
      result.append(queue);
    }
    result += lexforms[i];
  }

  result += queue;
  return result;
}

void
FSTProcessor::bilingual(InputFile& input, UFILE *output, GenerationMode mode)
{
  if(getNullFlush())
  {
    bilingual_wrapper_null_flush(input, output, mode);
  }

  State current_state = initial_state;
  UString sf;                   // source language analysis
  UString queue;                // symbols to be added to each target
  UString result;               // result of looking up analysis in bidix

  outOfWord = false;

  skipUntil(input, output, '^');
  std::pair<UString,int> tr;           // readBilingual return value, containing:
  int val;                        // the alphabet value of current symbol, and
  UString symbol;           // the current symbol as a string
  bool seentags = false;          // have we seen any tags at all in the analysis?

  bool seensurface = false;
  UString surface;

  while(true)                   // ie. while(val != 0x7fffffff)
  {
    tr = readBilingual(input, output);
    symbol = tr.first;
    val = tr.second;

    //fprintf(stderr, "> %ls : %lc : %d\n", tr.first.c_str(), tr.second, tr.second);
    if(biltransSurfaceForms && !seensurface && !outOfWord)
    {
      while(val != '/' && val != 0x7fffffff)
      {
        surface = surface + symbol;
        alphabet.getSymbol(surface, val);
        tr = readBilingual(input, output);
        symbol = tr.first;
        val = tr.second;
        //fprintf(stderr, " == %ls : %lc : %d => %ls\n", symbol.c_str(), val, val, surface.c_str());
      }
      seensurface = true;
      tr = readBilingual(input, output);
      symbol = tr.first;
      val = tr.second;
    }

    if (val == 0x7fffffff)
    {
      break;
    }

    if(val == '$' && outOfWord)
    {
      if(!seentags)        // if no tags: only return complete matches
      {
        bool uppercase = sf.size() > 1 && u_isupper(sf[1]);
        bool firstupper= u_isupper(sf[0]);

        result = current_state.filterFinals(all_finals, alphabet,
                                            escaped_chars,
                                            displayWeightsMode, maxAnalyses, maxWeightClasses,
                                            uppercase, firstupper, 0);
      }

      if(sf[0] == '*')
      {
        if (mode == gm_clean) {
          printWordBilingual(sf, "/"_u + sf.substr(1), output);
        }
        else {
          printWordBilingual(sf, "/"_u + sf, output);
        }
      }
      else if(!result.empty())
      {
        printWordBilingual(sf, compose(result, queue), output);
      }
      else
      { //xxx
        UString prefix = (mode == gm_all ? "/#"_u : "/@"_u);
        if(biltransSurfaceForms)
        {
          printWordBilingual(surface, prefix + surface, output);
        }
        else
        {
          printWordBilingual(sf, prefix + sf, output);
        }
      }
      seensurface = false;
      surface.clear();
      queue.clear();
      result.clear();
      current_state = initial_state;
      sf.clear();
      seentags = false;
    }
    else if(u_isspace(val) && sf.size() == 0)
    {
      // do nothing
    }
    else if(sf.size() > 0 && sf[0] == '*')
    {
      if(escaped_chars.find(val) != escaped_chars.end())
      {
        sf += '\\';
      }
      alphabet.getSymbol(sf, val); // add symbol to sf iff alphabetic
      if(val == 0)  // non-alphabetic, possibly unknown tag; add to sf
      {
        sf += symbol;
      }
    }
    else
    {
      if(escaped_chars.find(val) != escaped_chars.end())
      {
        sf += '\\';
      }
      alphabet.getSymbol(sf, val); // add symbol to sf iff alphabetic
      if(val == 0)  // non-alphabetic, possibly unknown tag; add to sf
      {
        sf += symbol;
      }
      if(alphabet.isTag(val) || val == 0)
      {
        seentags = true;
      }
      if(current_state.size() != 0)
      {
        current_state.step_case(val, caseSensitive);
      }
      if(current_state.isFinal(all_finals))
      {
        bool uppercase = sf.size() > 1 && u_isupper(sf[1]);
        bool firstupper= u_isupper(sf[0]);

        queue.clear(); // the intervening tags were matched
        result = current_state.filterFinals(all_finals, alphabet,
                                            escaped_chars,
                                            displayWeightsMode, maxAnalyses, maxWeightClasses,
                                            uppercase, firstupper, 0);
      }
      else if(!result.empty())
      {
        // We already have a result, but there is still more to read
        // of the analysis; following tags are not consumed, but
        // output as target language tags (added to result on
        // end-of-word). This queue is reset if result is changed.
        if(alphabet.isTag(val)) // known tag
        {
          alphabet.getSymbol(queue, val);
        }
        else if (val == 0) // non-alphabetic, possibly unknown tag
        {
          queue += symbol;
        }
        else if(current_state.size() == 0)
        {
          // There are no more alive transductions and the current symbol is not a tag -- unknown word!
          result.clear();
        }
      }
    }
  }
}

std::pair<UString, int>
FSTProcessor::biltransWithQueue(UString const &input_word, bool with_delim)
{
  State current_state = initial_state;
  UString result;
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
    return std::pair<UString, int>(input_word, 0);
  }

  if(input_word[start_point] == '=')
  {
    start_point++;
    mark = true;
  }

  bool firstupper = u_isupper(input_word[start_point]);
  bool uppercase = firstupper && u_isupper(input_word[start_point+1]);

  for(unsigned int i = start_point; i <= end_point; i++)
  {
    int val = 0;
    UString symbol;

    if(input_word[i] == '\\')
    {
      i++;
      val = input_word[i];
    }
    else if(input_word[i] == '<')
    {
      seentags = true;
      symbol = '<';
      for(unsigned int j = i + 1; j <= end_point; j++)
      {
        symbol += input_word[j];
        if(input_word[j] == '>')
        {
          i = j;
          break;
        }
      }
      val = alphabet(symbol);
    }
    else
    {
      val = input_word[i];
    }
    if(current_state.size() != 0)
    {
      if(!alphabet.isTag(val) && u_isupper(val) && !caseSensitive)
      {
        current_state.step(val, u_tolower(val));
      }
      else
      {
        current_state.step(val);
      }
    }
    if(current_state.isFinal(all_finals))
    {
      result.clear();
      if (with_delim) {
        result += '^';
      }
      if (mark) {
        result += '=';
      }
      result += current_state.filterFinals(all_finals, alphabet,
                                           escaped_chars,
                                           displayWeightsMode, maxAnalyses, maxWeightClasses,
                                           uppercase, firstupper, 0).substr(1);
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
          result = "^@"_u + input_word.substr(1);
        }
        else
        {
          result = "@"_u + input_word;
        }
        return std::pair<UString, int>(result, 0);
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
      result = "^@"_u + input_word.substr(1);
    }
    else
    {
      result = "@"_u + input_word;
    }
    return std::pair<UString, int>(result, 0);
  }



  // attach unmatched queue automatically

  if(!queue.empty())
  {
    UString result_with_queue;
    for(unsigned int i = 0, limit = result.size(); i != limit; i++)
    {
      switch(result[i])
      {
        case '\\':
          result_with_queue += '\\';
          i++;
          break;

        case '/':
          result_with_queue.append(queue);
          break;

        default:
          break;
      }
      result_with_queue += result[i];
    }
    result_with_queue.append(queue);

    if(with_delim)
    {
      result_with_queue += '$';
    }
    return std::pair<UString, int>(result_with_queue, queue.size());
  }
  else
  {
    if(with_delim)
    {
      result += '$';
    }
    return std::pair<UString, int>(result, 0);
  }
}

UString
FSTProcessor::biltransWithoutQueue(UString const &input_word, bool with_delim)
{
  State current_state = initial_state;
  UString result;
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
    return input_word;
  }

  if(input_word[start_point] == '=')
  {
    start_point++;
    mark = true;
  }

  bool firstupper = u_isupper(input_word[start_point]);
  bool uppercase = firstupper && u_isupper(input_word[start_point+1]);

  for(unsigned int i = start_point; i <= end_point; i++)
  {
    int val;
    UString symbol;

    if(input_word[i] == '\\')
    {
      i++;
      val = static_cast<int32_t>(input_word[i]);
    }
    else if(input_word[i] == '<')
    {
      symbol = '<';
      for(unsigned int j = i + 1; j <= end_point; j++)
      {
        symbol += input_word[j];
        if(input_word[j] == '>')
        {
          i = j;
          break;
        }
      }
      val = alphabet(symbol);
    }
    else
    {
      val = static_cast<int32_t>(input_word[i]);
    }
    if(current_state.size() != 0)
    {
      if(!alphabet.isTag(val) && u_isupper(val) && !caseSensitive)
      {
        current_state.step(val, u_tolower(val));
      }
      else
      {
        current_state.step(val);
      }
    }
    if(current_state.isFinal(all_finals))
    {
      result.clear();
      if (with_delim) {
        result += '^';
      }
      if (mark) {
        result += '=';
      }
      result += current_state.filterFinals(all_finals, alphabet,
                                           escaped_chars,
                                           displayWeightsMode, maxAnalyses, maxWeightClasses,
                                           uppercase, firstupper, 0).substr(1);
    }

    if(current_state.size() == 0)
    {
      if(symbol.empty())
      {
        // word is not present
        if(with_delim)
        {
          result = "^@"_u + input_word.substr(1);
        }
        else
        {
          result = "@"_u + input_word;
        }
        return result;
      }
    }
  }

  if(with_delim)
  {
    result += '$';
  }
  return result;
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
      if(str.substr(0, 9) == "<![CDATA["_u)
      {
        while(str.substr(str.size()-3) != "]]>"_u)
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
FSTProcessor::printSAOWord(UString const &lf, UFILE *output)
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

    current_state.step_case(val, caseSensitive);

    if(current_state.size() != 0)
    {
      alphabet.getSymbol(sf, val);
    }
    else
    {
      if(!isAlphabetic(val) && sf.empty())
      {
        printChar(val, output, true);
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

        unsigned int limit = firstNotAlpha(sf);
        unsigned int size = sf.size();
        limit = (limit == static_cast<unsigned int>(UString::npos)?size:limit);
        input_buffer.back(1+(size-limit));
        u_fprintf(output, "<d>%S</d>", sf.c_str());
      }
      else if(lf.empty())
      {
        unsigned int limit = firstNotAlpha(sf);
        unsigned int size = sf.size();
        limit = (limit == static_cast<unsigned int>(UString::npos)?size:limit);
        input_buffer.back(1+(size-limit));
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

UString
FSTProcessor::removeTags(UString const &str)
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
FSTProcessor::setBiltransSurfaceForms(bool const value)
{
  biltransSurfaceForms = value;
}

void
FSTProcessor::setCaseSensitiveMode(bool const value)
{
  caseSensitive = value;
}

void
FSTProcessor::setDictionaryCaseMode(bool const value)
{
  dictionaryCase = value;
}

void
FSTProcessor::setNullFlush(bool const value)
{
  nullFlush = value;
}

void
FSTProcessor::setIgnoredChars(bool const value)
{
  useIgnoredChars = value;
}

void
FSTProcessor::setRestoreChars(bool const value)
{
  useRestoreChars = value;
}

void
FSTProcessor::setUseDefaultIgnoredChars(bool const value)
{
  useDefaultIgnoredChars = value;
}

void
FSTProcessor::setDisplayWeightsMode(bool const value)
{
  displayWeightsMode = value;
}

void
FSTProcessor::setMaxAnalysesValue(int const value)
{
  maxAnalyses = value;
}

void
FSTProcessor::setMaxWeightClassesValue(int const value)
{
  maxWeightClasses = value;
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

size_t
FSTProcessor::firstNotAlpha(UString const &sf)
{
  UCharCharacterIterator it = UCharCharacterIterator(sf.c_str(), sf.size());
  size_t i = 0;
  while (it.hasNext()) {
    UChar32 c = it.next32PostInc();
    if(!isAlphabetic(c))
    {
      return i;
    }
    i += c > UINT16_MAX ? 2 : 1;
  }
  return UString::npos;
}
