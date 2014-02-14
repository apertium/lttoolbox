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

void endProgram(char *name)
{
  if(name != NULL)
  {
    cout << basename(name) << " v" << PACKAGE_VERSION <<": trim a transducer to another transducer" << endl;
    cout << "USAGE: " << basename(name) << " analyser_bin_file bidix_bin_file output_bin_file " << endl;
  }
  exit(EXIT_FAILURE);
}

std::pair<std::pair<Alphabet, wstring>, std::map<wstring, Transducer> >
read_fst(FILE *bin_file)
{
  Alphabet new_alphabet;
  wstring letters = L"";

  std::map<wstring, Transducer> transducers;

  // letters
  int len = Compression::multibyte_read(bin_file);
  while(len > 0)
  {
    letters.push_back(static_cast<wchar_t>(Compression::multibyte_read(bin_file)));
    len--;
  }

  // symbols
  new_alphabet.read(bin_file);

  len = Compression::multibyte_read(bin_file);

  while(len > 0)
  {
    int len2 = Compression::multibyte_read(bin_file);
    wstring name = L"";
    while(len2 > 0)
    {
      name += static_cast<wchar_t>(Compression::multibyte_read(bin_file));
      len2--;
    }
    transducers[name].read(bin_file);

    len--;
  }

  std::pair<Alphabet, wstring> alph_letters;
  alph_letters.first = new_alphabet;
  alph_letters.second = letters;
  return std::pair<std::pair<Alphabet, wstring>, std::map<wstring, Transducer> > (alph_letters, transducers);
}

std::pair<std::pair<Alphabet, wstring>, std::map<wstring, Transducer> >
trim(FILE *file_a, FILE *file_b)
{
  std::pair<std::pair<Alphabet, wstring>, std::map<wstring, Transducer> > alph_trans_a = read_fst(file_a);
  Alphabet alph_a = alph_trans_a.first.first;
  std::map<wstring, Transducer> trans_a = alph_trans_a.second;
  std::pair<std::pair<Alphabet, wstring>, std::map<wstring, Transducer> > alph_trans_b = read_fst(file_b);
  Alphabet alph_b = alph_trans_b.first.first;
  std::map<wstring, Transducer> trans_b = alph_trans_b.second;

  std::map<wstring, Transducer> prefix_transducers;
  // Do we need the second type of this map? See line 115.
  for(std::map<wstring, Transducer>::iterator it = trans_b.begin(); it != trans_b.end(); it++)
  {
  /* a set of symbols of the alphabet of the monolingual transducer, all of the
   * input and output tags of which are set equal
   */
  set<int> loopback_symbols;
  /* TODO: define this!
   * the alphabet of the prefix transducer
   */
  Alphabet prefix_alphabet;
  /* I have no idea which transducers alph_a and alph_b are for. Therefore, I
   * have written this conditional code to cover both foreseeable possiblilies.
   */
//#define _ALPH_A_IS_MONOLINGUAL_TRASDUCER_
#ifdef _ALPH_A_IS_MONOLINGUAL_TRANSDUCER_
  alph_a.insertSymbolsIntoSet(loopback_symbols, prefix_alphabet);
#endif
//#define _ALPH_B_IS_MONOLINGUAL_TRANSDUCER_
#ifdef _ALPH_B_IS_MONOLINGUAL_TRANSDUCER_
  alph_b.insertSymbolsIntoSet(loopback_symbols, prefix_alphabet);
#endif
  prefix_transducers[it->first]=it->second.appendDotStar(loopback_symbols, prefix_alphabet);
  }
  for(std::map<wstring, Transducer>::iterator it = trans_a.begin(); it != trans_a.end(); it++)
  {
    // it->second.intersect(alph_a, alph_b, trans_b);
  }
  alph_trans_b.second = prefix_transducers;
  return alph_trans_b;
}


int main(int argc, char *argv[])
{
  if(argc != 4)
  {
    endProgram(argv[0]);
  }

  LtLocale::tryToSetLocale();

  FILE *analyser = fopen(argv[1], "rb");
  FILE *bidix = fopen(argv[2], "rb");
  FILE *output = fopen(argv[3], "wb");

  std::pair<std::pair<Alphabet, wstring>, std::map<wstring, Transducer> > trimmed = trim(analyser, bidix);
  Alphabet alph_t = trimmed.first.first;
  wstring letters = trimmed.first.second;
  std::map<wstring, Transducer> trans_t = trimmed.second;

  // letters
  Compression::wstring_write(letters, output);

  // symbols
  alph_t.write(output);

  // transducers
  Compression::multibyte_write(trans_t.size(), output);

  int conta=0;
  for(std::map<wstring, Transducer>::iterator it = trans_t.begin(); it != trans_t.end(); it++)
  {
    conta++;
    wcout << it->first << " " << it->second.size();
    wcout << " " << it->second.numberOfTransitions() << endl;
    Compression::wstring_write(it->first, output);
    it->second.write(output);
  }

  fclose(analyser);
  fclose(bidix);
  fclose(output);

  return 0;
}
