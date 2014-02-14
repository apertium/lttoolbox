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
trim(FILE *file_mono, FILE *file_bi)
{
  std::pair<std::pair<Alphabet, wstring>, std::map<wstring, Transducer> > alph_trans_mono = read_fst(file_mono);
  Alphabet alph_mono = alph_trans_mono.first.first;
  std::map<wstring, Transducer> trans_mono = alph_trans_mono.second;
  std::pair<std::pair<Alphabet, wstring>, std::map<wstring, Transducer> > alph_trans_bi = read_fst(file_bi);
  Alphabet alph_bi = alph_trans_bi.first.first;
  std::map<wstring, Transducer> trans_bi = alph_trans_bi.second;

  // The prefix transducer is the union of all transducers from bidix,
  // with a ".*" appended
  Transducer prefix_transducer;
  // The "." in ".*" is a set of equal pairs of the output symbols
  // from the monodix alphabet (<n>:<n> etc.)
  Alphabet alph_prefix = alph_bi;
  set<int> loopback_symbols;    // ints refer to alph_prefix
  alph_prefix.createLoopbackSymbols(loopback_symbols, alph_mono, Alphabet::right);

  for(std::map<wstring, Transducer>::iterator it = trans_bi.begin(); it != trans_bi.end(); it++)
  {
    Transducer prefix_tmp = it->second.appendDotStar(loopback_symbols);
    if(prefix_transducer.isEmpty()) 
    {
      prefix_transducer = prefix_tmp;
    }
    else 
    {
      prefix_transducer.unionWith(prefix_tmp);
    }
    // wcerr << it->first<<endl;
    // prefix_tmp.show(alph_prefix);
    // wcerr << L"current union:"<<endl;
    // prefix_transducer.show(alph_prefix);
  }
  // prefix_transducer.minimize();
  // wcerr << L"minimized:"<<endl;
  // prefix_transducer.show(alph_prefix);

  for(std::map<wstring, Transducer>::iterator it = trans_mono.begin(); it != trans_mono.end(); it++)
  {
    // Transducer trimmed_tmp = it->second.intersect(prefix_transducer,
    //                                               alph_mono,
    //                                               alph_prefix);

    // wcerr << it->first<<endl;
    // trimmed_tmp.show(alph_mono);
    // trans_mono[it->first] = trimmed_tmp;
  }
  alph_trans_mono.second = trans_mono;
  return alph_trans_mono;
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
