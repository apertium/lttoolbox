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
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */
#include <lttoolbox/fst_processor.h>
#include <lttoolbox/compression.h>
#include <lttoolbox/exception.h>
#include <lttoolbox/xml_parse_util.h>

#include <iostream>
#include <cerrno>
#include <climits>

#ifdef _WIN32
#include <utf8_fwrap.h>
#endif

using namespace std;


FSTProcessor::FSTProcessor() :
default_weight(0.0000),
outOfWord(false),
isLastBlankTM(false)
{
  // escaped_chars chars
  escaped_chars.insert(L'[');
  escaped_chars.insert(L']');
  escaped_chars.insert(L'{');
  escaped_chars.insert(L'}');
  escaped_chars.insert(L'^');
  escaped_chars.insert(L'$');
  escaped_chars.insert(L'/');
  escaped_chars.insert(L'\\');
  escaped_chars.insert(L'@');
  escaped_chars.insert(L'<');
  escaped_chars.insert(L'>');

  caseSensitive = false;
  dictionaryCase = false;
  do_decomposition = false;
  nullFlush = false;
  nullFlushGeneration = false;
  useIgnoredChars = false;
  useDefaultIgnoredChars = true;
  useRestoreChars = false;
  displayWeightsMode = false;
  showControlSymbols = false;
  biltransSurfaceForms = false;
  maxAnalyses = INT_MAX;
  maxWeightClasses = INT_MAX;
  compoundOnlyLSymbol = 0;
  compoundRSymbol = 0;
  compound_max_elements = 4;

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
FSTProcessor::parseICX(string const &file)
{
  if(useIgnoredChars)
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
FSTProcessor::parseRCX(string const &file)
{
  if(useRestoreChars)
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
      procNodeRCX();
      ret = xmlTextReaderRead(reader);
    }
  }
}

void
FSTProcessor::procNodeICX()
{
  xmlChar  const *xname = xmlTextReaderConstName(reader);
  wstring name = XMLParseUtil::towstring(xname);
  if(name == L"#text")
  {
    /* ignore */
  }
  else if(name == L"ignored-chars")
  {
    /* ignore */
  }
  else if(name == L"char")
  {
    ignored_chars.insert(static_cast<int>(XMLParseUtil::attrib(reader, L"value")[0]));
  }
  else if(name == L"#comment")
  {
    /* ignore */
  }
  else
  {
    wcerr << L"Error in ICX file (" << xmlTextReaderGetParserLineNumber(reader);
    wcerr << L"): Invalid node '<" << name << L">'." << endl;
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
  xmlChar  const *xname = xmlTextReaderConstName(reader);
  wstring name = XMLParseUtil::towstring(xname);
  if(name == L"#text")
  {
    /* ignore */
  }
  else if(name == L"restore-chars")
  {
    /* ignore */
  }
  else if(name == L"char")
  {
    rcx_current_char = static_cast<int>(XMLParseUtil::attrib(reader, L"value")[0]);
  }
  else if(name == L"restore-char")
  {
    rcx_map[rcx_current_char].insert(static_cast<int>(XMLParseUtil::attrib(reader, L"value")[0]));
  }
  else if(name == L"#comment")
  {
    /* ignore */
  }
  else
  {
    wcerr << L"Error in RCX file (" << xmlTextReaderGetParserLineNumber(reader);
    wcerr << L"): Invalid node '<" << name << L">'." << endl;
    exit(EXIT_FAILURE);
  }
}

wchar_t
FSTProcessor::readEscaped(FILE *input)
{
  if(feof(input))
  {
    streamError();
  }

  wchar_t val = static_cast<wchar_t>(fgetwc_unlocked(input));

  if(feof(input) || escaped_chars.find(val) == escaped_chars.end())
  {
    streamError();
  }

  return val;
}

wstring
FSTProcessor::readFullBlock(FILE *input, wchar_t const delim1, wchar_t const delim2)
{
  wstring result = L"";
  result += delim1;
  wchar_t c = delim1;

  while(!feof(input) && c != delim2)
  {
    c = static_cast<wchar_t>(fgetwc_unlocked(input));
    result += c;
    if(c != L'\\')
    {
      continue;
    }
    else
    {
      result += static_cast<wchar_t>(readEscaped(input));
    }
  }

  if(c != delim2)
  {
    streamError();
  }

  return result;
}

int
FSTProcessor::readAnalysis(FILE *input)
{
  if(!input_buffer.isEmpty())
  {
    return input_buffer.next();
  }

  wchar_t val = static_cast<wchar_t>(fgetwc_unlocked(input));
  int altval = 0;
  if(feof(input))
  {
    return 0;
  }

  if((useIgnoredChars || useDefaultIgnoredChars) && ignored_chars.find(val) != ignored_chars.end())
  {
    input_buffer.add(val);
    val = static_cast<wchar_t>(fgetwc_unlocked(input));
  }

  if(escaped_chars.find(val) != escaped_chars.end())
  {
    switch(val)
    {
      case L'<':
        altval = static_cast<int>(alphabet(readFullBlock(input, L'<', L'>')));
        input_buffer.add(altval);
        return altval;

      case L'[':
        blankqueue.push(readFullBlock(input, L'[', L']'));
        input_buffer.add(static_cast<int>(L' '));
        return static_cast<int>(L' ');

      case L'\\':
        val = static_cast<wchar_t>(fgetwc_unlocked(input));
        if(escaped_chars.find(val) == escaped_chars.end())
        {
          streamError();
        }
        input_buffer.add(static_cast<int>(val));
        return val;

      default:
        streamError();
    }
  }
  if(val == L' ') {
    blankqueue.push(L" ");
  }

  input_buffer.add(val);
  return val;
}

int
FSTProcessor::readTMAnalysis(FILE *input)
{
  isLastBlankTM = false;
  if(!input_buffer.isEmpty())
  {
    return input_buffer.next();
  }

  wchar_t val = static_cast<wchar_t>(fgetwc_unlocked(input));
  int altval = 0;
  if(feof(input))
  {
    return 0;
  }

  if(escaped_chars.find(val) != escaped_chars.end() || iswdigit(val))
  {
    switch(val)
    {
      case L'<':
        altval = static_cast<int>(alphabet(readFullBlock(input, L'<', L'>')));
        input_buffer.add(altval);
        return altval;

      case L'[':
        blankqueue.push(readFullBlock(input, L'[', L']'));
        input_buffer.add(static_cast<int>(L' '));
        isLastBlankTM = true;
        return static_cast<int>(L' ');

      case L'\\':
        val = static_cast<wchar_t>(fgetwc_unlocked(input));
        if(escaped_chars.find(val) == escaped_chars.end())
        {
          streamError();
        }
        input_buffer.add(static_cast<int>(val));
        return val;
      case L'0':
      case L'1':
      case L'2':
      case L'3':
      case L'4':
      case L'5':
      case L'6':
      case L'7':
      case L'8':
      case L'9':
        {
          wstring ws = L"";
          do
          {
            ws += val;
            val = static_cast<wchar_t>(fgetwc_unlocked(input));
          } while(iswdigit(val));
          ungetwc(val, input);
          input_buffer.add(alphabet(L"<n>"));
          numbers.push_back(ws);
          return alphabet(L"<n>");
        }
        break;

      default:
        streamError();
    }
  }

  input_buffer.add(val);
  return val;
}

int
FSTProcessor::readPostgeneration(FILE *input)
{
  if(!input_buffer.isEmpty())
  {
    return input_buffer.next();
  }

  wchar_t val = static_cast<wchar_t>(fgetwc_unlocked(input));
  int altval = 0;
  if(feof(input))
  {
    return 0;
  }

  switch(val)
  {
    case L'<':
      altval = static_cast<int>(alphabet(readFullBlock(input, L'<', L'>')));
      input_buffer.add(altval);
      return altval;

    case L'[':
      blankqueue.push(readFullBlock(input, L'[', L']'));
      input_buffer.add(static_cast<int>(L' '));
      return static_cast<int>(L' ');

    case L'\\':
      val = static_cast<wchar_t>(fgetwc_unlocked(input));
      if(escaped_chars.find(val) == escaped_chars.end())
      {
        streamError();
      }
      input_buffer.add(static_cast<int>(val));
      return val;

    default:
      input_buffer.add(val);
      return val;
  }
}

void
FSTProcessor::skipUntil(FILE *input, FILE *output, wint_t const character)
{
  while(true)
  {
    wint_t val = fgetwc_unlocked(input);
    if(feof(input))
    {
      return;
    }

    switch(val)
    {
      case L'\\':
        val = fgetwc_unlocked(input);
        if(feof(input))
        {
          return;
        }
        fputwc_unlocked(L'\\', output);
        fputwc_unlocked(val, output);
        break;

      case L'\0':
        fputwc_unlocked(val, output);
        if(nullFlushGeneration)
        {
          fflush(output);
        }
        break;

      default:
        if(val == character)
        {
          return;
        }
        else
        {
          fputwc_unlocked(val, output);
        }
        break;
    }
  }
}

int
FSTProcessor::readGeneration(FILE *input, FILE *output)
{
  wint_t val = fgetwc_unlocked(input);

  if(feof(input))
  {
    return 0x7fffffff;
  }

  if(outOfWord)
  {
    if(val == L'^')
    {
      val = fgetwc_unlocked(input);
      if(feof(input))
      {
        return 0x7fffffff;
      }
    }
    else if(val == L'\\')
    {
      fputwc_unlocked(val, output);
      val = fgetwc_unlocked(input);
      if(feof(input))
      {
        return 0x7fffffff;
      }
      fputwc_unlocked(val,output);
      skipUntil(input, output, L'^');
      val = fgetwc_unlocked(input);
      if(feof(input))
      {
        return 0x7fffffff;
      }
    }
    else
    {
      fputwc_unlocked(val, output);
      skipUntil(input, output, L'^');
      val = fgetwc_unlocked(input);
      if(feof(input))
      {
        return 0x7fffffff;
      }
    }
    outOfWord = false;
  }

  if(val == L'\\')
  {
    val = fgetwc_unlocked(input);
    return static_cast<int>(val);
  }
  else if(val == L'$')
  {
    outOfWord = true;
    return static_cast<int>(L'$');
  }
  else if(val == L'<')
  {
    wstring cad = L"";
    cad += static_cast<wchar_t>(val);
    while((val = fgetwc_unlocked(input)) != L'>')
    {
      if(feof(input))
      {
        streamError();
      }
      cad += static_cast<wchar_t>(val);
    }
    cad += static_cast<wchar_t>(val);

    return alphabet(cad);
  }
  else if(val == L'[')
  {
    fputws_unlocked(readFullBlock(input, L'[', L']').c_str(), output);
    return readGeneration(input, output);
  }
  else
  {
    return static_cast<int>(val);
  }

  return 0x7fffffff;
}

pair<wstring, int>
FSTProcessor::readBilingual(FILE *input, FILE *output)
{
  wint_t val = fgetwc_unlocked(input);
  wstring symbol = L"";

  if(feof(input))
  {
    return pair<wstring, int>(symbol, 0x7fffffff);
  }

  if(outOfWord)
  {
    if(val == L'^')
    {
      val = fgetwc_unlocked(input);
      if(feof(input))
      {
        return pair<wstring, int>(symbol, 0x7fffffff);
      }
    }
    else if(val == L'\\')
    {
      fputwc_unlocked(val, output);
      val = fgetwc_unlocked(input);
      if(feof(input))
      {
        return pair<wstring, int>(symbol, 0x7fffffff);
      }
      fputwc_unlocked(val,output);
      skipUntil(input, output, L'^');
      val = fgetwc_unlocked(input);
      if(feof(input))
      {
        return pair<wstring, int>(symbol, 0x7fffffff);
      }
    }
    else
    {
      fputwc_unlocked(val, output);
      skipUntil(input, output, L'^');
      val = fgetwc_unlocked(input);
      if(feof(input))
      {
        return pair<wstring, int>(symbol, 0x7fffffff);
      }
    }
    outOfWord = false;
  }

  if(val == L'\\')
  {
    val = fgetwc_unlocked(input);
    return pair<wstring, int>(symbol, val);
  }
  else if(val == L'$')
  {
    outOfWord = true;
    return pair<wstring, int>(symbol, static_cast<int>(L'$'));
  }
  else if(val == L'<')
  {
    wstring cad = L"";
    cad += static_cast<wchar_t>(val);
    while((val = fgetwc_unlocked(input)) != L'>')
    {
      if(feof(input))
      {
        streamError();
      }
      cad += static_cast<wchar_t>(val);
    }
    cad += static_cast<wchar_t>(val);

    int res = alphabet(cad);

    if (res == 0)
    {
      symbol = cad;
    }
    return pair<wstring, int>(symbol, res);
  }
  else if(val == L'[')
  {
    fputws_unlocked(readFullBlock(input, L'[', L']').c_str(), output);
    return readBilingual(input, output);
  }

  return pair<wstring, int>(symbol, val);
}

void
FSTProcessor::flushBlanks(FILE *output)
{
  for(size_t i = blankqueue.size(); i > 0; i--)
  {
    fputws_unlocked(blankqueue.front().c_str(), output);
    blankqueue.pop();
  }
}

void
FSTProcessor::calcInitial()
{
  for(map<wstring, TransExe, Ltstr>::iterator it = transducers.begin(),
                                             limit = transducers.end();
      it != limit; it++)
  {
    root.addTransition(0, 0, it->second.getInitial(), default_weight);
  }

  initial_state.init(&root);
}

bool
FSTProcessor::endsWith(wstring const &str, wstring const &suffix)
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
  for(map<wstring, TransExe, Ltstr>::iterator it = transducers.begin(),
                                             limit = transducers.end();
      it != limit; it++)
  {
    if(endsWith(it->first, L"@inconditional"))
    {
      inconditional.insert(it->second.getFinals().begin(),
                           it->second.getFinals().end());
    }
    else if(endsWith(it->first, L"@standard"))
    {
      standard.insert(it->second.getFinals().begin(),
                      it->second.getFinals().end());
    }
    else if(endsWith(it->first, L"@postblank"))
    {
      postblank.insert(it->second.getFinals().begin(),
                       it->second.getFinals().end());
    }
    else if(endsWith(it->first, L"@preblank"))
    {
      preblank.insert(it->second.getFinals().begin(),
                      it->second.getFinals().end());
    }
    else
    {
      wcerr << L"Error: Unsupported transducer type for '";
      wcerr << it->first << L"'." << endl;
      exit(EXIT_FAILURE);
    }
  }
}

void
FSTProcessor::writeEscaped(wstring const &str, FILE *output)
{
  for(unsigned int i = 0, limit = str.size(); i < limit; i++)
  {
    if(escaped_chars.find(str[i]) != escaped_chars.end())
    {
      fputwc_unlocked(L'\\', output);
    }
    fputwc_unlocked(str[i], output);
  }
}

size_t
FSTProcessor::writeEscapedPopBlanks(wstring const &str, FILE *output)
{
  size_t postpop = 0;
  for (unsigned int i = 0, limit = str.size(); i < limit; i++)
  {
    if (escaped_chars.find(str[i]) != escaped_chars.end()) {
      fputwc_unlocked(L'\\', output);
    }
    fputwc_unlocked(str[i], output);
    if (str[i] == L' ') {
      if (blankqueue.front() == L" ") {
        blankqueue.pop();
      } else {
        postpop++;
      }
    }
  }
  return postpop;
}

void
FSTProcessor::writeEscapedWithTags(wstring const &str, FILE *output)
{
  for(unsigned int i = 0, limit = str.size(); i < limit; i++)
  {
    if(str[i] == L'<' && i >=1 && str[i-1] != L'\\')
    {
      fputws_unlocked(str.substr(i).c_str(), output);
      return;
    }

    if(escaped_chars.find(str[i]) != escaped_chars.end())
    {
      fputwc_unlocked(L'\\', output);
    }
    fputwc_unlocked(str[i], output);
  }
}



void
FSTProcessor::printWord(wstring const &sf, wstring const &lf, FILE *output)
{
  fputwc_unlocked(L'^', output);
  writeEscaped(sf, output);
  fputws_unlocked(lf.c_str(), output);
  fputwc_unlocked(L'$', output);
}

void
FSTProcessor::printWordPopBlank(wstring const &sf, wstring const &lf, FILE *output)
{
  fputwc_unlocked(L'^', output);
  size_t postpop = writeEscapedPopBlanks(sf, output);
  fputws_unlocked(lf.c_str(), output);
  fputwc_unlocked(L'$', output);
  while (postpop-- && blankqueue.size() > 0)
  {
    fputws(blankqueue.front().c_str(), output);
    blankqueue.pop();
  }
}

void
FSTProcessor::printWordBilingual(wstring const &sf, wstring const &lf, FILE *output)
{
  fputwc_unlocked(L'^', output);
  fputws_unlocked(sf.c_str(), output);
  fputws_unlocked(lf.c_str(), output);
  fputwc_unlocked(L'$', output);
}

void
FSTProcessor::printUnknownWord(wstring const &sf, FILE *output)
{
  fputwc_unlocked(L'^', output);
  writeEscaped(sf, output);
  fputwc_unlocked(L'/', output);
  fputwc_unlocked(L'*', output);
  writeEscaped(sf, output);
  fputwc_unlocked(L'$', output);
}

unsigned int
FSTProcessor::lastBlank(wstring const &str)
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
FSTProcessor::printSpace(wchar_t const val, FILE *output)
{
  if(blankqueue.size() > 0)
  {
    flushBlanks(output);
  }
  else
  {
    fputwc_unlocked(val, output);
  }
}

bool
FSTProcessor::isEscaped(wchar_t const c) const
{
  return escaped_chars.find(c) != escaped_chars.end();
}

bool
FSTProcessor::isAlphabetic(wchar_t const c) const
{
  return (bool)std::iswalnum(c) || alphabetic_chars.find(c) != alphabetic_chars.end();
}

void
FSTProcessor::load(FILE *input)
{
  fpos_t pos;
  if (fgetpos(input, &pos) == 0) {
      char header[4]{};
      fread(header, 1, 4, input);
      if (strncmp(header, HEADER_LTTOOLBOX, 4) == 0) {
          auto features = read_le<uint64_t>(input);
          if (features >= LTF_UNKNOWN) {
              throw std::runtime_error("FST has features that are unknown to this version of lttoolbox - upgrade!");
          }
      }
      else {
          // Old binary format
          fsetpos(input, &pos);
      }
  }

  // letters
  int len = Compression::multibyte_read(input);
  while(len > 0)
  {
    alphabetic_chars.insert(static_cast<wchar_t>(Compression::multibyte_read(input)));
    len--;
  }

  // symbols
  alphabet.read(input);

  len = Compression::multibyte_read(input);

  while(len > 0)
  {
    int len2 = Compression::multibyte_read(input);
    wstring name = L"";
    while(len2 > 0)
    {
      name += static_cast<wchar_t>(Compression::multibyte_read(input));
      len2--;
    }
    transducers[name].read(input, alphabet);
    len--;
  }
}

void
FSTProcessor::lsx_wrapper_null_flush(FILE *input, FILE *output)
{
  setNullFlush(false);
  //nullFlushGeneration = true;

  while(!feof(input))
  {
    lsx(input, output);
    fputwc_unlocked(L'\0', output);
    int code = fflush(output);
    if(code != 0)
    {
        wcerr << L"Could not flush output " << errno << endl;
    }
  }
}

void
FSTProcessor::lsx(FILE *input, FILE *output)
{
  if(getNullFlush())
  {
    lsx_wrapper_null_flush(input, output);
  }

  vector<State> new_states, alive_states;
  wstring blank, out, in, alt_out, alt_in;
  bool outOfWord = true;
  bool finalFound = false;
  bool plus_thing = false;

  alive_states.push_back(initial_state);

  int val = -1;

  while(!feof(input) && val != 0)
  {
    val = fgetwc_unlocked(input);

    if(val == L'+' && isEscaped(val) && !outOfWord)
    {
      val = L'$';
      plus_thing = true;
    }

    if((val == L'^' && isEscaped(val) && outOfWord) || feof(input) || val == 0)
    {
      blankqueue.push(blank);

      if(alive_states.size() == 0)
      {
        if(blankqueue.size() > 0)
        {
          fputws(blankqueue.front().c_str(), output);
          fflush(output);
          blankqueue.pop();
        }

        alive_states.push_back(initial_state);

        alt_in = L"";
        for(int i=0; i < (int) in.size(); i++) // FIXME indexing
        {
          alt_in += in[i];
          if(in[i] == L'$' && in[i+1] == L'^' && blankqueue.size() > 0)
          {
            // in.insert(i+1, blankqueue.front().c_str());
            alt_in += blankqueue.front().c_str();
            blankqueue.pop();
          }
        }
        in = alt_in;
        fputws(in.c_str(), output);
        fflush(output);
        in = L"";
        finalFound = false;
      }
      else if(finalFound && alive_states.size() == 1)
      {
        finalFound = false;
      }

      blank = L"";
      in += val;
      outOfWord = false;
      continue;
    }

    // wcerr << L"\n[!] " << (wchar_t)val << L" ||| " << outOfWord << endl;

    if(outOfWord)
    {
      blank += val;
      continue;
    }

    if((val == 0 || feof(input) || val == L'$') && !outOfWord) // && isEscaped(val)
    {
      new_states.clear();
      for(vector<State>::const_iterator it = alive_states.begin(); it != alive_states.end(); it++)
      {
        State s = *it;
        //wcerr << endl << L"[0] FEOF | $ | " << s.size() << L" | " << s.isFinal(all_finals) << endl;
        s.step(alphabet(L"<$>"));
        //wcerr << endl << L"[1] FEOF | $ | " << s.size() << L" | " << s.isFinal(all_finals) << endl;
        if(s.size() > 0)
        {
          new_states.push_back(s);
        }

        /*if(s.isFinal(all_finals))
        {
          out += s.filterFinals(all_finals, alphabet, escaped_chars, displayWeightsMode, maxAnalyses, maxWeightClasses);
          new_states.push_back(*initial_state);
        }*/

        if(s.isFinal(all_finals))
        {
          new_states.clear();
          new_states.push_back(initial_state);
          out = s.filterFinals(all_finals, alphabet, escaped_chars, displayWeightsMode, maxAnalyses, maxWeightClasses);

          alt_out = L"";
          for (int i=0; i < (int) out.size(); i++)
          {
            wchar_t c = out.at(i);
            if(c == L'/')
            {
              alt_out += L'^';
            }
            else if(out[i-1] == L'<' && c == L'$' && out[i+1] == L'>') // indexing
            {
              alt_out += c;
              alt_out += L'^';
            }
            else if(!(c == L'<' && out[i+1] == L'$' && out[i+2] == L'>') && !(out[i-2] == L'<' && out[i-1] == L'$' && c == L'>'))
            {
              alt_out += c;
            }
          }
          out = alt_out;


          if(out[out.length()-1] == L'^')
          {
            out = out.substr(0, out.length()-1); // extra ^ at the end
            if(plus_thing)
            {
              out[out.size()-1] = L'+';
              plus_thing = false;
            }
          }
          else // take# out ... of
          {
            for(int i=out.length()-1; i>=0; i--) // indexing
            {
              if(out.at(i) == L'$')
              {
                out.insert(i+1, L" ");
                break;
              }
            }
            out += L'$';
          }

          if(blankqueue.size() > 0)
          {
            fputws(blankqueue.front().c_str(), output);
            blankqueue.pop();
          }

          alt_out = L"";
          for(int i=0; i < (int) out.size(); i++) // indexing
          {
            if((out.at(i) == L'$') && blankqueue.size() > 0)
            {
              alt_out += out.at(i);
              alt_out += blankqueue.front().c_str();
              blankqueue.pop();
            }
            else if((out.at(i) == L'$') && blankqueue.size() == 0 && i != (int) out.size()-1)
            {
              alt_out += out.at(i);
              alt_out += L' ';
            }
            else if(out.at(i) == L' ' && blankqueue.size() > 0)
            {
              alt_out += blankqueue.front().c_str();
              blankqueue.pop();
            }
            else
            {
              alt_out += out.at(i);
            }
          }
          out = alt_out;

          fputws(out.c_str(), output);
          flushBlanks(output);
          finalFound = true;
          out = L"";
          in = L"";
        }
      }

      alive_states.swap(new_states);
      outOfWord = true;

      if(!finalFound)
      {
        in += val; //do not remove
      }
      continue;
    }

    if(!outOfWord) // && (!(feof(input) || val == L'$')))
    {
      if(val == L'<') // tag
      {
        wstring tag = readFullBlock(input, L'<', L'>');
        in += tag;
        if(!alphabet.isSymbolDefined(tag))
        {
          alphabet.includeSymbol(tag);
        }
        val = static_cast<int>(alphabet(tag));
      }
      else
      {
        in += (wchar_t) val;
      }

      new_states.clear();
      for(vector<State>::const_iterator it = alive_states.begin(); it != alive_states.end(); it++)
      {
        State s = *it;
        if(val < 0)
        {
          s.step_override(val, alphabet(L"<ANY_TAG>"), val);
        }
        else if(val > 0)
        {
          int val_lowercase = towlower(val);
          s.step_override(val_lowercase, alphabet(L"<ANY_CHAR>"), val); // FIXME deal with cases! in step_override
        }

        if(s.size() > 0)
        {
          new_states.push_back(s);
        }

      }
      alive_states.swap(new_states);
    }
  }

  flushBlanks(output);
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

  for(map<wstring, TransExe, Ltstr>::iterator it = transducers.begin(),
                                             limit = transducers.end();
      it != limit; it++)
  {
    all_finals.insert(it->second.getFinals().begin(),
                      it->second.getFinals().end());
  }
}

void
FSTProcessor::initGeneration()
{
  setIgnoredChars(false);
  calcInitial();
  for(map<wstring, TransExe, Ltstr>::iterator it = transducers.begin(),
                                             limit = transducers.end();
      it != limit; it++)
  {
    all_finals.insert(it->second.getFinals().begin(),
                      it->second.getFinals().end());
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


wstring
FSTProcessor::compoundAnalysis(wstring input_word, bool uppercase, bool firstupper)
{
  const int MAX_COMBINATIONS = 32767;

  State current_state = initial_state;

  for(unsigned int i=0; i<input_word.size(); i++)
  {
    wchar_t val=input_word.at(i);

    current_state.step_case(val, caseSensitive);

    if(current_state.size() > MAX_COMBINATIONS)
    {
      wcerr << L"Warning: compoundAnalysis's MAX_COMBINATIONS exceeded for '" << input_word << L"'" << endl;
      wcerr << L"         gave up at char " << i << L" '" << val << L"'." << endl;

      wstring nullString = L"";
      return  nullString;
    }

    if(i < input_word.size()-1)
    {
      current_state.restartFinals(all_finals, compoundOnlyLSymbol, &initial_state, '+');
    }

    if(current_state.size()==0)
    {
      wstring nullString = L"";
      return nullString;
    }
  }

  current_state.pruneCompounds(compoundRSymbol, '+', compound_max_elements);
  wstring result = current_state.filterFinals(all_finals, alphabet, escaped_chars, displayWeightsMode, maxAnalyses, maxWeightClasses, uppercase, firstupper);

  return result;
}



void
FSTProcessor::initDecompositionSymbols()
{
  if((compoundOnlyLSymbol=alphabet(L"<:co:only-L>")) == 0
     && (compoundOnlyLSymbol=alphabet(L"<:compound:only-L>")) == 0
     && (compoundOnlyLSymbol=alphabet(L"<@co:only-L>")) == 0
     && (compoundOnlyLSymbol=alphabet(L"<@compound:only-L>")) == 0
     && (compoundOnlyLSymbol=alphabet(L"<compound-only-L>")) == 0)
  {
    wcerr << L"Warning: Decomposition symbol <:compound:only-L> not found" << endl;
  }
  else if(!showControlSymbols)
  {
    alphabet.setSymbol(compoundOnlyLSymbol, L"");
  }

  if((compoundRSymbol=alphabet(L"<:co:R>")) == 0
     && (compoundRSymbol=alphabet(L"<:compound:R>")) == 0
     && (compoundRSymbol=alphabet(L"<@co:R>")) == 0
     && (compoundRSymbol=alphabet(L"<@compound:R>")) == 0
     && (compoundRSymbol=alphabet(L"<compound-R>")) == 0)
  {
    wcerr << L"Warning: Decomposition symbol <:compound:R> not found" << endl;
  }
  else if(!showControlSymbols)
  {
    alphabet.setSymbol(compoundRSymbol, L"");
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
FSTProcessor::analysis(FILE *input, FILE *output)
{
  if(getNullFlush())
  {
    analysis_wrapper_null_flush(input, output);
  }

  bool last_incond = false;
  bool last_postblank = false;
  bool last_preblank = false;
  State current_state = initial_state;
  wstring lf = L"";   //lexical form
  wstring sf = L"";   //surface form
  int last = 0;
  bool firstupper = false, uppercase = false;
  map<int, set<int> >::iterator rcx_map_ptr;

  while(wchar_t val = readAnalysis(input))
  {
    // test for final states
    if(current_state.isFinal(all_finals))
    {
      if(current_state.isFinal(inconditional))
      {
        if(!dictionaryCase)
        {
          firstupper = iswupper(sf[0]);
          uppercase = firstupper && iswupper(sf[sf.size()-1]);
        }

        if(do_decomposition && compoundOnlyLSymbol != 0)
        {
          current_state.pruneStatesWithForbiddenSymbol(compoundOnlyLSymbol);
        }
        lf = current_state.filterFinals(all_finals, alphabet,
                                        escaped_chars,
                                        displayWeightsMode, maxAnalyses, maxWeightClasses,
                                        uppercase, firstupper);
        last_incond = true;
        last = input_buffer.getPos();
      }
      else if(current_state.isFinal(postblank))
      {
        if(!dictionaryCase)
        {
          firstupper = iswupper(sf[0]);
          uppercase = firstupper && iswupper(sf[sf.size()-1]);
        }

        if(do_decomposition && compoundOnlyLSymbol != 0)
        {
          current_state.pruneStatesWithForbiddenSymbol(compoundOnlyLSymbol);
        }
        lf = current_state.filterFinals(all_finals, alphabet,
                                        escaped_chars,
                                        displayWeightsMode, maxAnalyses, maxWeightClasses,
                                        uppercase, firstupper);
        last_postblank = true;
        last = input_buffer.getPos();
      }
      else if(current_state.isFinal(preblank))
      {
        if(!dictionaryCase)
        {
          firstupper = iswupper(sf[0]);
          uppercase = firstupper && iswupper(sf[sf.size()-1]);
        }

        if(do_decomposition && compoundOnlyLSymbol != 0)
        {
          current_state.pruneStatesWithForbiddenSymbol(compoundOnlyLSymbol);
        }
        lf = current_state.filterFinals(all_finals, alphabet,
                                        escaped_chars,
                                        displayWeightsMode, maxAnalyses, maxWeightClasses,
                                        uppercase, firstupper);
        last_preblank = true;
        last = input_buffer.getPos();
      }
      else if(!isAlphabetic(val))
      {
        if(!dictionaryCase)
        {
          firstupper = iswupper(sf[0]);
          uppercase = firstupper && iswupper(sf[sf.size()-1]);
        }

        if(do_decomposition && compoundOnlyLSymbol != 0)
        {
          current_state.pruneStatesWithForbiddenSymbol(compoundOnlyLSymbol);
        }
        lf = current_state.filterFinals(all_finals, alphabet,
                                        escaped_chars,
                                        displayWeightsMode, maxAnalyses, maxWeightClasses,
                                        uppercase, firstupper);
        last_postblank = false;
        last_preblank = false;
        last_incond = false;
        last = input_buffer.getPos();
      }
    }
    else if(sf == L"" && iswspace(val))
    {
      lf = L"/*";
      lf.append(sf);
      last_postblank = false;
      last_preblank = false;
      last_incond = false;
      last = input_buffer.getPos();
    }

    if(useRestoreChars && rcx_map.find(val) != rcx_map.end())
    {
      rcx_map_ptr = rcx_map.find(val);
      set<int> tmpset = rcx_map_ptr->second;
      if(!iswupper(val) || caseSensitive)
      {
        current_state.step(val, tmpset);
      }
      else if(rcx_map.find(towlower(val)) != rcx_map.end())
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
      if(!iswupper(val) || caseSensitive)
      {
        current_state.step(val);
      }
      else
      {
        current_state.step(val, towlower(val));
      }
    }

    if(current_state.size() != 0)
    {
      alphabet.getSymbol(sf, val);
    }
    else
    {
      if(!isAlphabetic(val) && sf == L"")
      {
        if(iswspace(val))
        {
          if (blankqueue.size() > 0)
          {
            fputws_unlocked(blankqueue.front().c_str(), output);
            blankqueue.pop();
          }
          else
          {
            fputwc_unlocked(val, output);
          }
        }
        else
        {
          if(isEscaped(val))
          {
            fputwc_unlocked(L'\\', output);
          }
          fputwc_unlocked(val, output);
        }
      }
      else if(last_postblank)
      {
        printWordPopBlank(sf.substr(0, sf.size()-input_buffer.diffPrevPos(last)),
                          lf, output);
        fputwc_unlocked(L' ', output);
        input_buffer.setPos(last);
        input_buffer.back(1);
      }
      else if(last_preblank)
      {
        fputwc_unlocked(L' ', output);
        printWordPopBlank(sf.substr(0, sf.size()-input_buffer.diffPrevPos(last)),
                          lf, output);
        input_buffer.setPos(last);
        input_buffer.back(1);
      }
      else if(last_incond)
      {
        printWordPopBlank(sf.substr(0, sf.size()-input_buffer.diffPrevPos(last)),
                          lf, output);
        input_buffer.setPos(last);
        input_buffer.back(1);
      }
      else if(isAlphabetic(val) &&
              ((sf.size()-input_buffer.diffPrevPos(last)) > lastBlank(sf) ||
               lf == L""))
      {
        do
        {
          alphabet.getSymbol(sf, val);
        }
        while((val = readAnalysis(input)) && isAlphabetic(val));

        unsigned int limit = firstNotAlpha(sf);
        unsigned int size = sf.size();
        limit = (limit == static_cast<unsigned int>(wstring::npos)?size:limit);
        if(limit == 0)
        {
          input_buffer.back(sf.size());
          writeEscaped(sf.substr(0,1), output);
        }
        else
        {
          input_buffer.back(1+(size-limit));
          wstring unknown_word = sf.substr(0, limit);
          if(do_decomposition)
          {
            if(!dictionaryCase)
            {
              firstupper = iswupper(sf[0]);
              uppercase = firstupper && iswupper(sf[sf.size()-1]);
            }

            wstring compound = L"";
            compound = compoundAnalysis(unknown_word, uppercase, firstupper);
            if(compound != L"")
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
      else if(lf == L"")
      {
        unsigned int limit = firstNotAlpha(sf);
        unsigned int size = sf.size();
        limit = (limit == static_cast<unsigned int >(wstring::npos)?size:limit);
        if(limit == 0)
        {
          input_buffer.back(sf.size());
          writeEscaped(sf.substr(0,1), output);
        }
        else
        {
          input_buffer.back(1+(size-limit));
          wstring unknown_word = sf.substr(0, limit);
          if(do_decomposition)
          {
            if(!dictionaryCase)
            {
              firstupper = iswupper(sf[0]);
              uppercase = firstupper && iswupper(sf[sf.size()-1]);
            }

            wstring compound = L"";
            compound = compoundAnalysis(unknown_word, uppercase, firstupper);
            if(compound != L"")
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
        printWordPopBlank(sf.substr(0, sf.size()-input_buffer.diffPrevPos(last)),
                          lf, output);
        input_buffer.setPos(last);
        input_buffer.back(1);
      }

      current_state = initial_state;
      lf = L"";
      sf = L"";
      last_incond = false;
      last_postblank = false;
      last_preblank = false;
    }
  }

  // print remaining blanks
  flushBlanks(output);
}

void
FSTProcessor::analysis_wrapper_null_flush(FILE *input, FILE *output)
{
  setNullFlush(false);
  while(!feof(input))
  {
    analysis(input, output);
    fputwc_unlocked(L'\0', output);
    int code = fflush(output);
    if(code != 0)
    {
        wcerr << L"Could not flush output " << errno << endl;
    }
  }
}

void
FSTProcessor::generation_wrapper_null_flush(FILE *input, FILE *output,
                                            GenerationMode mode)
{
  setNullFlush(false);
  nullFlushGeneration = true;

  while(!feof(input))
  {
    generation(input, output, mode);
    fputwc_unlocked(L'\0', output);
    int code = fflush(output);
    if(code != 0)
    {
        wcerr << L"Could not flush output " << errno << endl;
    }
  }
}

void
FSTProcessor::postgeneration_wrapper_null_flush(FILE *input, FILE *output)
{
  setNullFlush(false);
  while(!feof(input))
  {
    postgeneration(input, output);
    fputwc_unlocked(L'\0', output);
    int code = fflush(output);
    if(code != 0)
    {
        wcerr << L"Could not flush output " << errno << endl;
    }
  }
}

void
FSTProcessor::intergeneration_wrapper_null_flush(FILE *input, FILE *output)
{
  setNullFlush(false);
  while (!feof(input))
  {
    intergeneration(input, output);
    fputwc_unlocked(L'\0', output);
    int code = fflush(output);
    if (code != 0)
    {
      wcerr << L"Could not flush output " << errno << endl;
    }
  }
}

void
FSTProcessor::transliteration_wrapper_null_flush(FILE *input, FILE *output)
{
  setNullFlush(false);
  while(!feof(input))
  {
    transliteration(input, output);
    fputwc_unlocked(L'\0', output);
    int code = fflush(output);
    if(code != 0)
    {
        wcerr << L"Could not flush output " << errno << endl;
    }
  }
}

void
FSTProcessor::tm_analysis(FILE *input, FILE *output)
{
  State current_state = initial_state;
  wstring lf = L"";     //lexical form
  wstring sf = L"";     //surface form
  int last = 0;

  while(wchar_t val = readTMAnalysis(input))
  {
    // test for final states
    if(current_state.isFinal(all_finals))
    {
      if(iswpunct(val))
      {
        lf = current_state.filterFinalsTM(all_finals, alphabet,
                                          escaped_chars,
                                          blankqueue, numbers).substr(1);
        last = input_buffer.getPos();
        numbers.clear();
      }
    }
    else if(sf == L"" && iswspace(val))
    {
      lf.append(sf);
      last = input_buffer.getPos();
    }

    if(!iswupper(val))
    {
      current_state.step(val);
    }
    else
    {
      current_state.step(val, towlower(val));
    }

    if(current_state.size() != 0)
    {
      if(val == -1)
      {
        sf.append(numbers[numbers.size()-1]);
      }
      else if(isLastBlankTM && val == L' ')
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
      if((iswspace(val) || iswpunct(val)) && sf == L"")
      {
        if(iswspace(val))
        {
          printSpace(val, output);
        }
        else
        {
          if(isEscaped(val))
          {
            fputwc_unlocked(L'\\', output);
          }
          fputwc_unlocked(val, output);
        }
      }
      else if(!iswspace(val) && !iswpunct(val) &&
              ((sf.size()-input_buffer.diffPrevPos(last)) > lastBlank(sf) ||
               lf == L""))
      {

        do
        {
          if(val == -1)
          {
            sf.append(numbers[numbers.size()-1]);
          }
          else if(isLastBlankTM && val == L' ')
          {
            sf.append(blankqueue.back());
          }
          else
          {
            alphabet.getSymbol(sf, val);
          }
        }
        while((val = readTMAnalysis(input)) && !iswspace(val) && !iswpunct(val));

        if(val == 0)
        {
          fputws_unlocked(sf.c_str(), output);
          return;
        }

        input_buffer.back(1);
        fputws_unlocked(sf.c_str(), output);

        while(blankqueue.size() > 0)
        {
          if(blankqueue.size() == 1 && isLastBlankTM)
          {
            break;
          }
          blankqueue.pop();
        }

/*
        unsigned int limit = sf.find(L' ');
        unsigned int size = sf.size();
        limit = (limit == static_cast<unsigned int>(wstring::npos)?size:limit);
        input_buffer.back(1+(size-limit));
        fputws_unlocked(sf.substr(0, limit).c_str(), output);
*/      }
      else if(lf == L"")
      {
/*        unsigned int limit = sf.find(L' ');
        unsigned int size = sf.size();
        limit = (limit == static_cast<unsigned int >(wstring::npos)?size:limit);
        input_buffer.back(1+(size-limit));
        fputws_unlocked(sf.substr(0, limit).c_str(), output);
*/
        input_buffer.back(1);
        fputws_unlocked(sf.c_str(), output);

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
        fputwc_unlocked(L'[', output);
        fputws_unlocked(lf.c_str(), output);
        fputwc_unlocked(L']', output);
        input_buffer.setPos(last);
        input_buffer.back(1);
      }

      current_state = initial_state;
      lf = L"";
      sf = L"";
    }
  }

  // print remaining blanks
  flushBlanks(output);
}


void
FSTProcessor::generation(FILE *input, FILE *output, GenerationMode mode)
{
  if(getNullFlush())
  {
    generation_wrapper_null_flush(input, output, mode);
  }

  State current_state = initial_state;
  wstring sf = L"";

  outOfWord = false;

  skipUntil(input, output, L'^');
  int val;

  while((val = readGeneration(input, output)) != 0x7fffffff)
  {
    if(sf == L"" && val == L'=')
    {
      fputwc(L'=', output);
      val = readGeneration(input, output);
    }

    if(val == L'$' && outOfWord)
    {
      if(sf[0] == L'*' || sf[0] == L'%')
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
          fputwc_unlocked(L'^', output);
          writeEscaped(removeTags(sf.substr(1)), output);
          fputwc_unlocked(L'/', output);
          writeEscapedWithTags(sf, output);
          fputwc_unlocked(L'$', output);
        }
      }
      else if(sf[0] == L'@')
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
          fputwc_unlocked(L'^', output);
          writeEscaped(removeTags(sf.substr(1)), output);
          fputwc_unlocked(L'/', output);
          writeEscapedWithTags(sf, output);
          fputwc_unlocked(L'$', output);
        }
      }
      else if(current_state.isFinal(all_finals))
      {
        bool firstupper = false, uppercase = false;
        if(!dictionaryCase)
        {
          uppercase = sf.size() > 1 && iswupper(sf[1]);
          firstupper= iswupper(sf[0]);
        }

        if(mode == gm_tagged || mode == gm_tagged_nm)
        {
          fputwc_unlocked(L'^', output);
        }

        fputws_unlocked(current_state.filterFinals(all_finals, alphabet,
                                                   escaped_chars,
                                                   displayWeightsMode, maxAnalyses, maxWeightClasses,
                                                   uppercase, firstupper).substr(1).c_str(), output);
        if(mode == gm_tagged || mode == gm_tagged_nm)
        {
          fputwc_unlocked(L'/', output);
          writeEscapedWithTags(sf, output);
          fputwc_unlocked(L'$', output);
        }

      }
      else
      {
        if(mode == gm_all)
        {
          fputwc_unlocked(L'#', output);
          writeEscaped(sf, output);
        }
        else if(mode == gm_clean)
        {
          writeEscaped(removeTags(sf), output);
        }
        else if(mode == gm_unknown)
        {
          if(sf != L"")
          {
            fputwc_unlocked(L'#', output);
            writeEscaped(removeTags(sf), output);
          }
        }
        else if(mode == gm_tagged)
        {
          fputwc_unlocked(L'#', output);
          writeEscaped(removeTags(sf), output);
        }
        else if(mode == gm_tagged_nm)
        {
          fputwc_unlocked(L'^', output);
          writeEscaped(removeTags(sf), output);
          fputwc_unlocked(L'/', output);
          fputwc_unlocked(L'#', output);
          writeEscapedWithTags(sf, output);
          fputwc_unlocked(L'$', output);
        }
      }

      current_state = initial_state;
      sf = L"";
    }
    else if(iswspace(val) && sf.size() == 0)
    {
      // do nothing
    }
    else if(sf.size() > 0 && (sf[0] == L'*' || sf[0] == L'%' ))
    {
      alphabet.getSymbol(sf, val);
    }
    else
    {
      alphabet.getSymbol(sf,val);
      if(current_state.size() > 0)
      {
        if(!alphabet.isTag(val) && iswupper(val) && !caseSensitive)
        {
          if(mode == gm_carefulcase)
          {
            current_state.step_careful(val, towlower(val));
          }
          else
          {
            current_state.step(val, towlower(val));
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
FSTProcessor::postgeneration(FILE *input, FILE *output)
{
  if(getNullFlush())
  {
    postgeneration_wrapper_null_flush(input, output);
  }

  bool skip_mode = true;
  State current_state = initial_state;
  wstring lf = L"";
  wstring sf = L"";
  int last = 0;
  set<wchar_t> empty_escaped_chars;

  while(wchar_t val = readPostgeneration(input))
  {
    if(val == L'~')
    {
      skip_mode = false;
    }

    if(skip_mode)
    {
      if(iswspace(val))
      {
        printSpace(val, output);
      }
      else
      {
        if(isEscaped(val))
        {
          fputwc_unlocked(L'\\', output);
        }
        fputwc_unlocked(val, output);
      }
    }
    else
    {
      // test for final states
      if(current_state.isFinal(all_finals))
      {
        bool firstupper = iswupper(sf[1]);
        bool uppercase = sf.size() > 1 && firstupper && iswupper(sf[2]);
        lf = current_state.filterFinals(all_finals, alphabet,
                                        empty_escaped_chars,
                                        displayWeightsMode, maxAnalyses, maxWeightClasses,
                                        uppercase, firstupper, 0);

        // case of the beggining of the next word

        wstring mybuf = L"";
        for(size_t i = sf.size(); i > 0; --i)
        {
          if(!isalpha(sf[i-1]))
          {
            break;
          }
          else
          {
            mybuf = sf[i-1] + mybuf;
          }
        }

        if(mybuf.size() > 0)
        {
          bool myfirstupper = iswupper(mybuf[0]);
          bool myuppercase = mybuf.size() > 1 && iswupper(mybuf[1]);

          for(size_t i = lf.size(); i > 0; --i)
          {
            if(!isalpha(lf[i-1]))
            {
              if(myfirstupper && i != lf.size())
              {
                lf[i] = towupper(lf[i]);
              }
              else
              {
                lf[i] = towlower(lf[i]);
              }
              break;
            }
            else
            {
              if(myuppercase)
              {
                lf[i-1] = towupper(lf[i-1]);
              }
              else
              {
                lf[i-1] = towlower(lf[i-1]);
              }
            }
          }
        }

        last = input_buffer.getPos();
      }

      if(!iswupper(val) || caseSensitive)
      {
        current_state.step(val);
      }
      else
      {
        current_state.step(val, towlower(val));
      }

      if(current_state.size() != 0)
      {
        alphabet.getSymbol(sf, val);
      }
      else
      {
        if(lf == L"")
        {
          unsigned int mark = sf.size();
          for(unsigned int i = 1, limit = sf.size(); i < limit; i++)
          {
            if(sf[i] == L'~')
            {
              mark = i;
              break;
            }
          }
          fputws_unlocked(sf.substr(1, mark-1).c_str(), output);
          if(mark == sf.size())
          {
            input_buffer.back(1);
          }
          else
          {
            input_buffer.back(sf.size()-mark);
          }
        }
        else
        {
          fputws_unlocked(lf.substr(1,lf.size()-3).c_str(), output);
          input_buffer.setPos(last);
          input_buffer.back(2);
          val = lf[lf.size()-2];
          if(iswspace(val))
          {
            printSpace(val, output);
          }
          else
          {
            if(isEscaped(val))
            {
              fputwc_unlocked(L'\\', output);
            }
            fputwc_unlocked(val, output);
          }
        }

        current_state = initial_state;
        lf = L"";
        sf = L"";
        skip_mode = true;
      }
    }
  }

  // print remaining blanks
  flushBlanks(output);
}

void
FSTProcessor::intergeneration(FILE *input, FILE *output)
{
  if (getNullFlush())
  {
    intergeneration_wrapper_null_flush(input, output);
  }

  bool skip_mode = true;
  State current_state = initial_state;
  wstring target = L"";
  wstring source = L"";
  int last = 0;
  set<wchar_t> empty_escaped_chars;

  while (true)
  {
    wchar_t val = readPostgeneration(input);

    if (val == L'~')
    {
      skip_mode = false;
    }

    if (skip_mode)
    {
      if (iswspace(val))
      {
        printSpace(val, output);
      }
      else
      {
        if(val != L'\0')
        {
          if (isEscaped(val))
          {
            fputwc_unlocked(L'\\', output);
          }
          fputwc_unlocked(val, output);
        }
      }
    }
    else
    {
      // test for final states
      if (current_state.isFinal(all_finals))
      {
        bool firstupper = iswupper(source[1]);
        bool uppercase = source.size() > 1 && firstupper && iswupper(source[2]);
        target = current_state.filterFinals(all_finals, alphabet,
                                        empty_escaped_chars,
                                        displayWeightsMode, maxAnalyses, maxWeightClasses,
                                        uppercase, firstupper, 0);

        last = input_buffer.getPos();
      }

      if (val != L'\0')
      {
        if (!iswupper(val) || caseSensitive)
        {
          current_state.step(val);
        }
        else
        {
          current_state.step(val, towlower(val));
        }
      }

      if (val != L'\0' && current_state.size() != 0)
      {
        alphabet.getSymbol(source, val);
      }
      else
      {
        if (target == L"") // no match
        {
          if (val == L'\0')
          {
            // flush source
            fputws_unlocked(source.c_str(), output);
          }
          else
          {
            fputwc_unlocked(source[0], output);

            unsigned int mark, limit;
            for (mark = 1, limit = source.size(); mark < limit && source[mark] != L'~' ; mark++)
            {
              fputwc_unlocked(source[mark], output);
            }

            if (mark != source.size())
            {
              int back = source.size() - mark;
              input_buffer.back(back);
            }

            if (val == L'~')
            {
              input_buffer.back(1);
            } else {
               fputwc_unlocked(val, output);
            }
          }
        }
        else
        {
          for(unsigned int i=1; i<target.size(); i++) {
            wchar_t c = target[i];

            if (iswspace(c))
            {
              printSpace(c, output);
            }
            else
            {
              if (isEscaped(c))
              {
                fputwc_unlocked(L'\\', output);
              }
              fputwc_unlocked(c, output);
            }
          }

          if (val != L'\0')
          {
            input_buffer.setPos(last);
            input_buffer.back(1);
          }
        }

        current_state = initial_state;
        target = L"";
        source = L"";
        skip_mode = true;
      }
    }

    if (val == L'\0')
    {
      break;
    }
  }

  // print remaining blanks
  flushBlanks(output);
}

void
FSTProcessor::transliteration(FILE *input, FILE *output)
{
  if(getNullFlush())
  {
    transliteration_wrapper_null_flush(input, output);
  }

  State current_state = initial_state;
  wstring lf = L"";
  wstring sf = L"";
  int last = 0;

  while(wchar_t val = readPostgeneration(input))
  {
    if(iswpunct(val) || iswspace(val))
    {
      bool firstupper = iswupper(sf[1]);
      bool uppercase = sf.size() > 1 && firstupper && iswupper(sf[2]);
      lf = current_state.filterFinals(all_finals, alphabet, escaped_chars,
                                      displayWeightsMode, maxAnalyses, maxWeightClasses,
                                      uppercase, firstupper, 0);
      if(!lf.empty())
      {
        fputws_unlocked(lf.substr(1).c_str(), output);
        current_state = initial_state;
        lf = L"";
        sf = L"";
      }
      if(iswspace(val))
      {
        printSpace(val, output);
      }
      else
      {
        if(isEscaped(val))
        {
          fputwc_unlocked(L'\\', output);
        }
        fputwc_unlocked(val, output);
      }
    }
    else
    {
      if(current_state.isFinal(all_finals))
      {
        bool firstupper = iswupper(sf[1]);
        bool uppercase = sf.size() > 1 && firstupper && iswupper(sf[2]);
        lf = current_state.filterFinals(all_finals, alphabet, escaped_chars,
                                        displayWeightsMode, maxAnalyses, maxWeightClasses,
                                        uppercase, firstupper, 0);
        last = input_buffer.getPos();
      }

      current_state.step(val);
      if(current_state.size() != 0)
      {
        alphabet.getSymbol(sf, val);
      }
      else
      {
        if(!lf.empty())
        {
          fputws_unlocked(lf.substr(1).c_str(), output);
          input_buffer.setPos(last);
          input_buffer.back(1);
          val = lf[lf.size()-1];
        }
        else
        {
          if(iswspace(val))
          {
            printSpace(val, output);
          }
          else
          {
            if(isEscaped(val))
            {
              fputwc_unlocked(L'\\', output);
            }
            fputwc_unlocked(val, output);
          }
        }
        current_state = initial_state;
        lf = L"";
        sf = L"";
      }
    }
  }
  // print remaining blanks
  flushBlanks(output);
}

wstring
FSTProcessor::biltransfull(wstring const &input_word, bool with_delim)
{
  State current_state = initial_state;
  wstring result = L"";
  unsigned int start_point = 1;
  unsigned int end_point = input_word.size()-2;
  wstring queue = L"";
  bool mark = false;

  if(with_delim == false)
  {
    start_point = 0;
    end_point = input_word.size()-1;
  }

  if(input_word[start_point] == L'*')
  {
    return input_word;
  }

  if(input_word[start_point] == L'=')
  {
    start_point++;
    mark = true;
  }

  bool firstupper = iswupper(input_word[start_point]);
  bool uppercase = firstupper && iswupper(input_word[start_point+1]);

  for(unsigned int i = start_point; i <= end_point; i++)
  {
    int val;
    wstring symbol = L"";

    if(input_word[i] == L'\\')
    {
      i++;
      val = static_cast<int>(input_word[i]);
    }
    else if(input_word[i] == L'<')
    {
      symbol = L'<';
      for(unsigned int j = i + 1; j <= end_point; j++)
      {
        symbol += input_word[j];
        if(input_word[j] == L'>')
        {
          i = j;
          break;
        }
      }
      val = alphabet(symbol);
    }
    else
    {
      val = static_cast<int>(input_word[i]);
    }
    if(current_state.size() != 0)
    {
      if(!alphabet.isTag(val) && iswupper(val) && !caseSensitive)
      {
        current_state.step(val, towlower(val));
      }
      else
      {
        current_state.step(val);
      }
    }
    if(current_state.isFinal(all_finals))
    {
      result = current_state.filterFinals(all_finals, alphabet,
                                          escaped_chars,
                                          displayWeightsMode, maxAnalyses, maxWeightClasses,
                                          uppercase, firstupper, 0);
      if(with_delim)
      {
        if(mark)
        {
          result = L"^="+result.substr(1);
        }
        else
        {
          result[0] = L'^';
        }
      }
      else
      {
        if(mark)
        {
          result = L"=" + result.substr(1);
        }
        else
        {
          result = result.substr(1);
        }
      }
    }

    if(current_state.size() == 0)
    {
      if(symbol != L"" && result != L"")
      {
        queue.append(symbol);
      }
      else
      {
        // word is not present
        if(with_delim)
        {
          result = L"^@" + input_word.substr(1);
        }
        else
        {
          result = L"@" + input_word;
        }
        return result;
      }
    }
  }

  if(start_point < (end_point - 3))
  {
    return L"^$";
  }
  // attach unmatched queue automatically

  if(queue != L"")
  {
    wstring result_with_queue = L"";
    for(unsigned int i = 0, limit = result.size(); i != limit; i++)
    {
      switch(result[i])
      {
        case L'\\':
          result_with_queue += L'\\';
          i++;
          break;

        case L'/':
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
      result_with_queue += L'$';
    }
    return result_with_queue;
  }
  else
  {
    if(with_delim)
    {
      result += L'$';
    }
    return result;
  }
}



wstring
FSTProcessor::biltrans(wstring const &input_word, bool with_delim)
{
  State current_state = initial_state;
  wstring result = L"";
  unsigned int start_point = 1;
  unsigned int end_point = input_word.size()-2;
  wstring queue = L"";
  bool mark = false;

  if(with_delim == false)
  {
    start_point = 0;
    end_point = input_word.size()-1;
  }

  if(input_word[start_point] == L'*')
  {
    return input_word;
  }

  if(input_word[start_point] == L'=')
  {
    start_point++;
    mark = true;
  }

  bool firstupper = iswupper(input_word[start_point]);
  bool uppercase = firstupper && iswupper(input_word[start_point+1]);

  for(unsigned int i = start_point; i <= end_point; i++)
  {
    int val;
    wstring symbol = L"";

    if(input_word[i] == L'\\')
    {
      i++;
      val = static_cast<int>(input_word[i]);
    }
    else if(input_word[i] == L'<')
    {
      symbol = L'<';
      for(unsigned int j = i + 1; j <= end_point; j++)
      {
        symbol += input_word[j];
        if(input_word[j] == L'>')
        {
          i = j;
          break;
        }
      }
      val = alphabet(symbol);
    }
    else
    {
      val = static_cast<int>(input_word[i]);
    }
    if(current_state.size() != 0)
    {
      if(!alphabet.isTag(val) && iswupper(val) && !caseSensitive)
      {
        current_state.step(val, towlower(val));
      }
      else
      {
        current_state.step(val);
      }
    }
    if(current_state.isFinal(all_finals))
    {
      result = current_state.filterFinals(all_finals, alphabet,
                                          escaped_chars,
                                          displayWeightsMode, maxAnalyses, maxWeightClasses,
                                          uppercase, firstupper, 0);
      if(with_delim)
      {
        if(mark)
        {
          result = L"^="+result.substr(1);
        }
        else
        {
          result[0] = L'^';
        }
      }
      else
      {
        if(mark)
        {
          result = L"=" + result.substr(1);
        }
        else
        {
          result = result.substr(1);
        }
      }
    }

    if(current_state.size() == 0)
    {
      if(symbol != L"" && result != L"")
      {
        queue.append(symbol);
      }
      else
      {
        // word is not present
        if(with_delim)
        {
          result = L"^@" + input_word.substr(1);
        }
        else
        {
          result = L"@" + input_word;
        }
        return result;
      }
    }
  }

  // attach unmatched queue automatically

  if(queue != L"")
  {
    wstring result_with_queue = L"";
    for(unsigned int i = 0, limit = result.size(); i != limit; i++)
    {
      switch(result[i])
      {
        case L'\\':
          result_with_queue += L'\\';
          i++;
          break;

        case L'/':
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
      result_with_queue += L'$';
    }
    return result_with_queue;
  }
  else
  {
    if(with_delim)
    {
      result += L'$';
    }
    return result;
  }
}

void
FSTProcessor::bilingual_wrapper_null_flush(FILE *input, FILE *output)
{
  setNullFlush(false);
  nullFlushGeneration = true;

  while(!feof(input))
  {
    bilingual(input, output);
    fputwc_unlocked(L'\0', output);
    int code = fflush(output);
    if(code != 0)
    {
        wcerr << L"Could not flush output " << errno << endl;
    }
  }
}

wstring
FSTProcessor::compose(wstring const &lexforms, wstring const &queue) const
{
  wstring result = L"";

  for(unsigned int i = 1; i< lexforms.size(); i++)
  {
    if(lexforms[i] == L'\\')
    {
      result += L'\\';
      i++;
    }
    else if(lexforms[i] == L'/')
    {
      result.append(queue);
    }
    result += lexforms[i];
  }

  return L"/" + result + queue;
}

void
FSTProcessor::bilingual(FILE *input, FILE *output)
{
  if(getNullFlush())
  {
    bilingual_wrapper_null_flush(input, output);
  }

  State current_state = initial_state;
  wstring sf = L"";                   // source language analysis
  wstring queue = L"";                // symbols to be added to each target
  wstring result = L"";               // result of looking up analysis in bidix

  outOfWord = false;

  skipUntil(input, output, L'^');
  pair<wstring,int> tr;           // readBilingual return value, containing:
  int val;                        // the alphabet value of current symbol, and
  wstring symbol = L"";           // the current symbol as a string
  bool seentags = false;          // have we seen any tags at all in the analysis?

  bool seensurface = false;
  wstring surface = L"";

  while(true)                   // ie. while(val != 0x7fffffff)
  {
    tr = readBilingual(input, output);
    symbol = tr.first;
    val = tr.second;

    //fwprintf(stderr, L"> %S : %C : %d\n", tr.first.c_str(), tr.second, tr.second);
    if(biltransSurfaceForms && !seensurface && !outOfWord)
    {
      while(val != L'/' && val != 0x7fffffff)
      {
        surface = surface + symbol;
        alphabet.getSymbol(surface, val);
        tr = readBilingual(input, output);
        symbol = tr.first;
        val = tr.second;
        //fwprintf(stderr, L" == %S : %C : %d => %S\n", symbol.c_str(), val, val, surface.c_str());
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

    if(val == L'$' && outOfWord)
    {
      if(!seentags)        // if no tags: only return complete matches
      {
        bool uppercase = sf.size() > 1 && iswupper(sf[1]);
        bool firstupper= iswupper(sf[0]);

        result = current_state.filterFinals(all_finals, alphabet,
                                            escaped_chars,
                                            displayWeightsMode, maxAnalyses, maxWeightClasses,
                                            uppercase, firstupper, 0);
      }

      if(sf[0] == L'*')
      {
        printWordBilingual(sf, L"/"+sf, output);
      }
      else if(result != L"")
      {
        printWordBilingual(sf, compose(result, queue), output);
      }
      else
      { //xxx
        if(biltransSurfaceForms)
        {
          printWordBilingual(surface, L"/@"+surface, output);
        }
        else
        {
          printWordBilingual(sf, L"/@"+sf, output);
        }
      }
      seensurface = false;
      surface = L"";
      queue = L"";
      result = L"";
      current_state = initial_state;
      sf = L"";
      seentags = false;
    }
    else if(iswspace(val) && sf.size() == 0)
    {
      // do nothing
    }
    else if(sf.size() > 0 && sf[0] == L'*')
    {
      if(escaped_chars.find(val) != escaped_chars.end())
      {
        sf += L'\\';
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
        sf += L'\\';
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
        if(!alphabet.isTag(val) && iswupper(val) && !caseSensitive)
        {
          current_state.step(val, towlower(val));
        }
        else
        {
          current_state.step(val);
        }
      }
      if(current_state.isFinal(all_finals))
      {
        bool uppercase = sf.size() > 1 && iswupper(sf[1]);
        bool firstupper= iswupper(sf[0]);

        queue = L""; // the intervening tags were matched
        result = current_state.filterFinals(all_finals, alphabet,
                                            escaped_chars,
                                            displayWeightsMode, maxAnalyses, maxWeightClasses,
                                            uppercase, firstupper, 0);
      }
      else if(result != L"")
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
          result = L"";
        }
      }
    }
  }
}

pair<wstring, int>
FSTProcessor::biltransWithQueue(wstring const &input_word, bool with_delim)
{
  State current_state = initial_state;
  wstring result = L"";
  unsigned int start_point = 1;
  unsigned int end_point = input_word.size()-2;
  wstring queue = L"";
  bool mark = false;
  bool seentags = false;  // have we seen any tags at all in the analysis?

  if(with_delim == false)
  {
    start_point = 0;
    end_point = input_word.size()-1;
  }

  if(input_word[start_point] == L'*')
  {
    return pair<wstring, int>(input_word, 0);
  }

  if(input_word[start_point] == L'=')
  {
    start_point++;
    mark = true;
  }

  bool firstupper = iswupper(input_word[start_point]);
  bool uppercase = firstupper && iswupper(input_word[start_point+1]);

  for(unsigned int i = start_point; i <= end_point; i++)
  {
    int val = 0;
    wstring symbol = L"";

    if(input_word[i] == L'\\')
    {
      i++;
      val = input_word[i];
    }
    else if(input_word[i] == L'<')
    {
      seentags = true;
      symbol = L'<';
      for(unsigned int j = i + 1; j <= end_point; j++)
      {
        symbol += input_word[j];
        if(input_word[j] == L'>')
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
      if(!alphabet.isTag(val) && iswupper(val) && !caseSensitive)
      {
        current_state.step(val, towlower(val));
      }
      else
      {
        current_state.step(val);
      }
    }
    if(current_state.isFinal(all_finals))
    {
      result = current_state.filterFinals(all_finals, alphabet,
                                          escaped_chars,
                                          displayWeightsMode, maxAnalyses, maxWeightClasses,
                                          uppercase, firstupper, 0);
      if(with_delim)
      {
        if(mark)
        {
          result = L"^=" + result.substr(1);
        }
        else
        {
          result[0] = L'^';
        }
      }
      else
      {
        if(mark)
        {
          result = L"=" + result.substr(1);
        }
        else
        {
          result = result.substr(1);
        }
      }
    }

    if(current_state.size() == 0)
    {
      if(symbol != L"" && result != L"")
      {
        queue.append(symbol);
      }
      else
      {
        // word is not present
        if(with_delim)
        {
          result = L"^@" + input_word.substr(1);
        }
        else
        {
          result = L"@" + input_word;
        }
        return pair<wstring, int>(result, 0);
      }
    }
  }

  if (!seentags
      && L"" == current_state.filterFinals(all_finals, alphabet,
                                           escaped_chars,
                                           displayWeightsMode, maxAnalyses, maxWeightClasses,
                                           uppercase, firstupper, 0))
  {
    // word is not present
    if(with_delim)
    {
      result = L"^@" + input_word.substr(1);
    }
    else
    {
      result = L"@" + input_word;
    }
    return pair<wstring, int>(result, 0);
  }



  // attach unmatched queue automatically

  if(queue != L"")
  {
    wstring result_with_queue = L"";
    for(unsigned int i = 0, limit = result.size(); i != limit; i++)
    {
      switch(result[i])
      {
        case L'\\':
          result_with_queue += L'\\';
          i++;
          break;

        case L'/':
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
      result_with_queue += L'$';
    }
    return pair<wstring, int>(result_with_queue, queue.size());
  }
  else
  {
    if(with_delim)
    {
      result += L'$';
    }
    return pair<wstring, int>(result, 0);
  }
}

wstring
FSTProcessor::biltransWithoutQueue(wstring const &input_word, bool with_delim)
{
  State current_state = initial_state;
  wstring result = L"";
  unsigned int start_point = 1;
  unsigned int end_point = input_word.size()-2;
  bool mark = false;

  if(with_delim == false)
  {
    start_point = 0;
    end_point = input_word.size()-1;
  }

  if(input_word[start_point] == L'*')
  {
    return input_word;
  }

  if(input_word[start_point] == L'=')
  {
    start_point++;
    mark = true;
  }

  bool firstupper = iswupper(input_word[start_point]);
  bool uppercase = firstupper && iswupper(input_word[start_point+1]);

  for(unsigned int i = start_point; i <= end_point; i++)
  {
    int val;
    wstring symbol = L"";

    if(input_word[i] == L'\\')
    {
      i++;
      val = static_cast<int>(input_word[i]);
    }
    else if(input_word[i] == L'<')
    {
      symbol = L'<';
      for(unsigned int j = i + 1; j <= end_point; j++)
      {
        symbol += input_word[j];
        if(input_word[j] == L'>')
        {
          i = j;
          break;
        }
      }
      val = alphabet(symbol);
    }
    else
    {
      val = static_cast<int>(input_word[i]);
    }
    if(current_state.size() != 0)
    {
      if(!alphabet.isTag(val) && iswupper(val) && !caseSensitive)
      {
        current_state.step(val, towlower(val));
      }
      else
      {
        current_state.step(val);
      }
    }
    if(current_state.isFinal(all_finals))
    {
      result = current_state.filterFinals(all_finals, alphabet,
                                          escaped_chars,
                                          displayWeightsMode, maxAnalyses, maxWeightClasses,
                                          uppercase, firstupper, 0);
      if(with_delim)
      {
        if(mark)
        {
          result = L"^=" + result.substr(1);
        }
        else
        {
          result[0] = L'^';
        }
      }
      else
      {
        if(mark)
        {
          result = L"=" + result.substr(1);
        }
        else
        {
          result = result.substr(1);
        }
      }
    }

    if(current_state.size() == 0)
    {
      if(symbol == L"")
      {
        // word is not present
        if(with_delim)
        {
          result = L"^@" + input_word.substr(1);
        }
        else
        {
          result = L"@" + input_word;
        }
        return result;
      }
    }
  }

  if(with_delim)
  {
    result += L'$';
  }
  return result;
}


bool
FSTProcessor::valid() const
{
  if(initial_state.isFinal(all_finals))
  {
    wcerr << L"Error: Invalid dictionary (hint: the left side of an entry is empty)" << endl;
    return false;
  }
  else
  {
    State s = initial_state;
    s.step(L' ');
    if(s.size() != 0)
    {
      wcerr << L"Error: Invalid dictionary (hint: entry beginning with whitespace)" << endl;
      return false;
    }
  }

  return true;
}

int
FSTProcessor::readSAO(FILE *input)
{
  if(!input_buffer.isEmpty())
  {
    return input_buffer.next();
  }

  wchar_t val = static_cast<wchar_t>(fgetwc_unlocked(input));
  if(feof(input))
  {
    return 0;
  }

  if(escaped_chars.find(val) != escaped_chars.end())
  {
    if(val == L'<')
    {
      wstring str = readFullBlock(input, L'<', L'>');
      if(str.substr(0, 9) == L"<![CDATA[")
      {
        while(str.substr(str.size()-3) != L"]]>")
        {
          str.append(readFullBlock(input, L'<', L'>').substr(1));
        }
        blankqueue.push(str);
        input_buffer.add(static_cast<int>(L' '));
        return static_cast<int>(L' ');
      }
      else
      {
        streamError();
      }
    }
    else if (val == L'\\') {
      val = static_cast<wchar_t>(fgetwc_unlocked(input));
      if(isEscaped(val))
      {
        input_buffer.add(val);
        return static_cast<int>(val);
      }
      else
        streamError();
    }
    else
    {
      streamError();
    }
  }

  input_buffer.add(val);
  return static_cast<int>(val);
}

void
FSTProcessor::printSAOWord(wstring const &lf, FILE *output)
{
  for(unsigned int i = 1, limit = lf.size(); i != limit; i++)
  {
    if(lf[i] == L'/')
    {
      break;
    }
    fputwc_unlocked(lf[i], output);
  }
}

void
FSTProcessor::SAO(FILE *input, FILE *output)
{
  bool last_incond = false;
  bool last_postblank = false;
  State current_state = initial_state;
  wstring lf = L"";
  wstring sf = L"";
  int last = 0;

  escaped_chars.clear();
  escaped_chars.insert(static_cast<wchar_t>(L'\\'));
  escaped_chars.insert(static_cast<wchar_t>(L'<'));
  escaped_chars.insert(static_cast<wchar_t>(L'>'));

  while(wchar_t val = readSAO(input))
  {
    // test for final states
    if(current_state.isFinal(all_finals))
    {
      if(current_state.isFinal(inconditional))
      {
        bool firstupper = iswupper(sf[0]);
        bool uppercase = firstupper && iswupper(sf[sf.size()-1]);

        lf = current_state.filterFinalsSAO(all_finals, alphabet,
                                        escaped_chars,
                                        uppercase, firstupper);
        last_incond = true;
        last = input_buffer.getPos();
      }
      else if(current_state.isFinal(postblank))
      {
        bool firstupper = iswupper(sf[0]);
        bool uppercase = firstupper && iswupper(sf[sf.size()-1]);

        lf = current_state.filterFinalsSAO(all_finals, alphabet,
                                        escaped_chars,
                                        uppercase, firstupper);
        last_postblank = true;
        last = input_buffer.getPos();
      }
      else if(!isAlphabetic(val))
      {
        bool firstupper = iswupper(sf[0]);
        bool uppercase = firstupper && iswupper(sf[sf.size()-1]);

        lf = current_state.filterFinalsSAO(all_finals, alphabet,
                                        escaped_chars,
                                        uppercase, firstupper);
        last_postblank = false;
        last_incond = false;
        last = input_buffer.getPos();
      }
    }
    else if(sf == L"" && iswspace(val))
    {
      lf = L"/*";
      lf.append(sf);
      last_postblank = false;
      last_incond = false;
      last = input_buffer.getPos();
    }

    if(!iswupper(val) || caseSensitive)
    {
      current_state.step(val);
    }
    else
    {
      current_state.step(val, towlower(val));
    }

    if(current_state.size() != 0)
    {
      alphabet.getSymbol(sf, val);
    }
    else
    {
      if(!isAlphabetic(val) && sf == L"")
      {
        if(iswspace(val))
        {
          printSpace(val, output);
        }
        else
        {
          if(isEscaped(val))
          {
            fputwc_unlocked(L'\\', output);
          }
          fputwc_unlocked(val, output);
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
        fputwc_unlocked(L' ', output);
        input_buffer.setPos(last);
        input_buffer.back(1);
      }
      else if(isAlphabetic(val) &&
              ((sf.size()-input_buffer.diffPrevPos(last)) > lastBlank(sf) ||
               lf == L""))
      {
        do
        {
          alphabet.getSymbol(sf, val);
        }
        while((val = readSAO(input)) && isAlphabetic(val));

        unsigned int limit = firstNotAlpha(sf);
        unsigned int size = sf.size();
        limit = (limit == static_cast<unsigned int>(wstring::npos)?size:limit);
        input_buffer.back(1+(size-limit));
        fputws_unlocked(L"<d>", output);
        fputws_unlocked(sf.c_str(), output);
        fputws_unlocked(L"</d>", output);
      }
      else if(lf == L"")
      {
        unsigned int limit = firstNotAlpha(sf);
        unsigned int size = sf.size();
        limit = (limit == static_cast<unsigned int>(wstring::npos)?size:limit);
        input_buffer.back(1+(size-limit));
        fputws_unlocked(L"<d>", output);
        fputws_unlocked(sf.c_str(), output);
        fputws_unlocked(L"</d>", output);
      }
      else
      {
        printSAOWord(lf, output);
        input_buffer.setPos(last);
        input_buffer.back(1);
      }

      current_state = initial_state;
      lf = L"";
      sf = L"";
      last_incond = false;
      last_postblank = false;
    }
  }

  // print remaining blanks
  flushBlanks(output);
}

wstring
FSTProcessor::removeTags(wstring const &str)
{
  for(unsigned int i = 0; i < str.size(); i++)
  {
    if(str[i] == L'<' && i >=1 && str[i-1] != L'\\')
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
FSTProcessor::firstNotAlpha(wstring const &sf)
{
  for(size_t i = 0, limit = sf.size(); i < limit; i++)
  {
    if(!isAlphabetic(sf[i]))
    {
      return i;
    }
  }

  return wstring::npos;
}
