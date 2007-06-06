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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */
#include <lttoolbox/fst_processor.h>
#include <lttoolbox/compression.h>

#include <iostream>

using namespace std;

	
FSTProcessor::FSTProcessor()
{
  // escaped_chars chars
  escaped_chars.insert(L'[');
  escaped_chars.insert(L']');
  escaped_chars.insert(L'^');
  escaped_chars.insert(L'$');
  escaped_chars.insert(L'/');
  escaped_chars.insert(L'\\');
  escaped_chars.insert(L'@');
  escaped_chars.insert(L'<');
  escaped_chars.insert(L'>');
}

FSTProcessor::~FSTProcessor()
{
}

void
FSTProcessor::streamError()
{
  cerr << "Error: Malformed input stream." << endl;
  exit(EXIT_FAILURE);
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

    if(val == '\\')
    {
      val = fgetwc_unlocked(input);
      if(feof(input))
      {
	return;
      }
      fputwc_unlocked('\\', output);
      fputwc_unlocked(val, output);
    }
    else if(val == character)
    {
      return;
    }
    else
    {
      fputwc_unlocked(val, output);
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

void
FSTProcessor::flushBlanks(FILE *output)
{  
  for(unsigned int i = blankqueue.size(); i > 0; i--)
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
    root.addTransition(0, 0, it->second.getInitial());
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
    else
    {
      wcerr << "Error: Unsupported transducer type for '";
      wcerr << it->first << "'." << endl;
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


void
FSTProcessor::printWord(wstring const &sf, wstring const &lf, FILE *output)
{
  fputwc_unlocked(L'^', output);
  writeEscaped(sf, output);
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
  return alphabetic_chars.find(c) != alphabetic_chars.end();
}

void
FSTProcessor::load(FILE *input)
{
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
FSTProcessor::initAnalysis()
{
  calcInitial();
  classifyFinals();
  all_finals = standard;
  all_finals.insert(inconditional.begin(), inconditional.end());
  all_finals.insert(postblank.begin(), postblank.end());
}

void
FSTProcessor::initGeneration()
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
FSTProcessor::initPostgeneration()
{
  initGeneration();
}

void
FSTProcessor::initBiltrans()
{
  initGeneration();
}

void
FSTProcessor::analysis(FILE *input, FILE *output)
{
  bool last_incond = false;
  bool last_postblank = false;
  State current_state = initial_state;
  wstring lf = L"";
  wstring sf = L"";
  int last = 0;

  while(wchar_t val = readAnalysis(input))
  {
    // test for final states
    if(current_state.isFinal(all_finals))
    {
      if(current_state.isFinal(inconditional))
      {
        bool firstupper = iswupper(sf[0]);
        bool uppercase = firstupper && iswupper(sf[sf.size()-1]);

        lf = current_state.filterFinals(all_finals, alphabet,
                                        escaped_chars,
                                        uppercase, firstupper);
        last_incond = true;
        last = input_buffer.getPos();
      }
      else if(current_state.isFinal(postblank))
      {
        bool firstupper = iswupper(sf[0]);
        bool uppercase = firstupper && iswupper(sf[sf.size()-1]);

        lf = current_state.filterFinals(all_finals, alphabet,
                                        escaped_chars,
                                        uppercase, firstupper);
        last_postblank = true;
        last = input_buffer.getPos();      
      }
      else if(!isAlphabetic(val))
      {
        bool firstupper = iswupper(sf[0]);
        bool uppercase = firstupper && iswupper(sf[sf.size()-1]);

        lf = current_state.filterFinals(all_finals, alphabet, 
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
        printWord(sf.substr(0, sf.size()-input_buffer.diffPrevPos(last)),
		  lf, output);
        input_buffer.setPos(last);
        input_buffer.back(1);
      }
      else if(last_postblank)
      {
        printWord(sf.substr(0, sf.size()-input_buffer.diffPrevPos(last)),
		  lf, output);
	fputwc_unlocked(' ', output);
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

        unsigned int limit = sf.find(L' ');
        unsigned int size = sf.size();
        limit = (limit == wstring::npos?size:limit);
        input_buffer.back(1+(size-limit));
        printUnknownWord(sf.substr(0, limit), output);
      }
      else if(lf == L"")
      {
        unsigned int limit = sf.find(L' ');
        unsigned int size = sf.size();
        limit = (limit == wstring::npos?size:limit);
        input_buffer.back(1+(size-limit));
        printUnknownWord(sf.substr(0, limit), output);
      }
      else
      {
        printWord(sf.substr(0, sf.size()-input_buffer.diffPrevPos(last)), 
                  lf, output);
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

void
FSTProcessor::generation(FILE *input, FILE *output, GenerationMode mode)
{
  State current_state = initial_state;
  wstring sf = L"";
 
  outOfWord = false;
 
  skipUntil(input, output, L'^');
  int val;
  while((val = readGeneration(input, output)) != 0x7fffffff)
  {
    if(val == L'$')
    {
      if(sf[0] == L'*')
      {
	if(mode != gm_clean)
        {
	  writeEscaped(sf, output);
	}
	else
	{
	  writeEscaped(sf.substr(1), output);
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
      }
      else if(current_state.isFinal(all_finals))
      {
        bool uppercase = sf.size() > 1 && iswupper(sf[1]);
        bool firstupper= iswupper(sf[0]);

        fputws_unlocked(current_state.filterFinals(all_finals, alphabet,
                                                  escaped_chars,
                                                  uppercase, firstupper).substr(1).c_str(),
						  output);
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
          fputwc_unlocked(L'#', output);
          writeEscaped(removeTags(sf), output);
        }
      }
  
      current_state = initial_state;
      sf = L"";
    }
    else if(iswspace(val) && sf.size() == 0)
    {
      // do nothing
    }
    else if(sf.size() > 0 && sf[0] == L'*')
    {
      alphabet.getSymbol(sf, val);
    }
    else
    {
      alphabet.getSymbol(sf,val);
      if(current_state.size() > 0)
      {
	if(!alphabet.isTag(val) && iswupper(val))
	{
	  current_state.step(val, towlower(val));
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
  bool skip_mode = true;
  State current_state = initial_state;
  wstring lf = L"";
  wstring sf = L"";
  int last = 0;

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
					escaped_chars,
					uppercase, firstupper, 0);

        // case of the beggining of the next word
        	
        wstring mybuf = L"";				
        for(unsigned int i = sf.size()-1; i >= 0; i--)
        {
          if(!isalpha(sf[i]))
          {
            break;
          }
          else
          {
            mybuf = sf[i] + mybuf;
          }
        }
        
        if(mybuf.size() > 0)
        {
          bool myfirstupper = iswupper(mybuf[0]);
          bool myuppercase = mybuf.size() > 1 && iswupper(mybuf[1]);
        
          for(unsigned int i = lf.size()-1; i >= 0; i--)
          {
            if(!isalpha(lf[i]))
            {
              if(myfirstupper && i != lf.size()-1)
              {
                lf[i+1] = towupper(lf[i+1]);
              }
              else
              {
                lf[i+1] = towlower(lf[i+1]);
              }
              break;
            }
            else
            {
              if(myuppercase)
              {
                lf[i] = towupper(lf[i]);
              }
              else
              {
                lf[i] = towlower(lf[i]);
              }
            }
          }
        }
        
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

wstring
FSTProcessor::biltrans(wstring const &input_word, bool with_delim)
{
  State current_state = initial_state;
  wstring result = L"";
  unsigned int start_point = 1;
  unsigned int end_point = input_word.size()-2;
  wstring queue = L"";
  
  if(with_delim == false)
  {
    start_point = 0;
    end_point = input_word.size()-1;
  }

  if(input_word[start_point] == L'*')
  {
    return input_word;
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
    else if(input_word[i] == '<')
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
      if(!alphabet.isTag(val) && iswupper(val))
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
                                         uppercase, firstupper, 0);
      if(with_delim)
      {      
        result[0] = L'^';
      }
      else
      {
        result = result.substr(1);
      }
    }
    
    if(current_state.size() == 0)
    { 
      if(symbol != L"")
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
    bool multiple_translation = false;
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
	  multiple_translation = true;
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

pair<wstring, int>
FSTProcessor::biltransWithQueue(wstring const &input_word, bool with_delim)
{
  State current_state = initial_state;
  wstring result = L"";
  unsigned int start_point = 1;
  unsigned int end_point = input_word.size()-2;
  wstring queue = L"";
  
  if(with_delim == false)
  {
    start_point = 0;
    end_point = input_word.size()-1;
  }

  if(input_word[start_point] == L'*')
  {
    return pair<wstring, int>(input_word, 0);
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
      val = input_word[i];
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
      val = input_word[i];
    }
    if(current_state.size() != 0)
    {
      if(!alphabet.isTag(val) && iswupper(val))
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
                                         uppercase, firstupper, 0);
      if(with_delim)
      {      
        result[0] = L'^';
      }
      else
      {
        result = result.substr(1);
      }
    }
    
    if(current_state.size() == 0)
    { 
      if(symbol != L"")
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

  // attach unmatched queue automatically

  if(queue != L"")
  {
    wstring result_with_queue = L"";    
    bool multiple_translation = false;
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
	  multiple_translation = true;
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
  
  if(with_delim == false)
  {
    start_point = 0;
    end_point = input_word.size()-1;
  }

  if(input_word[start_point] == L'*')
  {
    return input_word;
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
      val = static_cast<int>(input_word[i]);
    }
    if(current_state.size() != 0)
    {
      if(!alphabet.isTag(val) && iswupper(val))
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
                                         uppercase, firstupper, 0);
      if(with_delim)
      {      
        result[0] = L'^';
      }
      else
      {
        result = result.substr(1);
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
  return !initial_state.isFinal(all_finals);
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
        return static_cast<int>(' ');
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

        unsigned int limit = sf.find(L' ');
        unsigned int size = sf.size();
        limit = (limit == wstring::npos?size:limit);
        input_buffer.back(1+(size-limit));
        fputws_unlocked(L"<d>", output);
        fputws_unlocked(sf.c_str(), output);
        fputws_unlocked(L"</d>", output);
      }
      else if(lf == L"")
      {
        unsigned int limit = sf.find(L' ');
        unsigned int size = sf.size();
        limit = (limit == wstring::npos?size:limit);
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
