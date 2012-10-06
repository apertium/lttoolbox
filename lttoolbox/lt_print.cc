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
#include <lttoolbox/transducer.h>
#include <lttoolbox/compression.h>
#include <lttoolbox/lttoolbox_config.h>

#include <lttoolbox/my_stdio.h>
#include <lttoolbox/lt_locale.h>

#include <cstdlib>
#include <iostream>
#include <libgen.h>
#include <string>

using namespace std;

void endProgram(char *name)
{
  if(name != NULL)
  {
    cout << basename(name) << " v" << PACKAGE_VERSION <<": dump a transducer to text in ATT format" << endl;
    cout << "USAGE: " << basename(name) << " bin_file " << endl;
  }
  exit(EXIT_FAILURE);
}


int main(int argc, char *argv[])
{
  if(argc != 2) 
  {
    endProgram(argv[0]);
  }

  LtLocale::tryToSetLocale();


  FILE *input = fopen(argv[1], "r");

  Alphabet new_alphabet;
  set<wchar_t> alphabetic_chars;

  map<wstring, Transducer> transducers;

  // letters
  int len = Compression::multibyte_read(input);
  while(len > 0)
  {
    alphabetic_chars.insert(static_cast<wchar_t>(Compression::multibyte_read(input)));
    len--;
  }  

  // symbols  
  new_alphabet.read(input);

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
    transducers[name].read(input);

    len--;
  } 

  /////////////////////
 
  FILE *output = stdout;
  map<wstring, Transducer>::iterator penum = transducers.end();
  penum--;
  for(map<wstring, Transducer>::iterator it = transducers.begin(); it != transducers.end(); it++)
  {
    //it->second.minimize();
    it->second.show(new_alphabet, output);
    if(it != penum) 
    {
      fwprintf(output, L"--\n", it->first.c_str());
    }
  }

  fclose(input);
  
  return 0;
}
