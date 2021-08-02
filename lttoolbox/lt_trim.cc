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
#include <lttoolbox/transducer.h>
#include <lttoolbox/compression.h>
#include <lttoolbox/endian_util.h>
#include <lttoolbox/string_writer.h>

#include <lttoolbox/my_stdio.h>
#include <lttoolbox/lt_locale.h>

#include <cstdlib>
#include <iostream>
#include <libgen.h>
#include <string>
#include <cstring>

void endProgram(char *name)
{
  if(name != NULL)
  {
    cout << basename(name) << " v" << PACKAGE_VERSION <<": trim a transducer to another transducer" << endl;
    cout << "USAGE: " << basename(name) << " analyser_bin_file bidix_bin_file trimmed_bin_file " << endl;
  }
  exit(EXIT_FAILURE);
}

std::pair<std::pair<Alphabet, UString>, std::map<UString, Transducer> >
trim(FILE *file_mono, FILE *file_bi)
{
  UString letters_mono;
  Alphabet alph_mono;
  std::map<UString, Transducer> trans_mono;
  read_transducer_set(file_mono, letters_mono, alph_mono, trans_mono);

  UString letters_bi;
  Alphabet alph_bi;
  std::map<UString, Transducer> trans_bi;
  read_transducer_set(file_bi, letters_bi, alph_bi, trans_bi);

  // The prefix transducer is the union of all transducers from bidix,
  // with a ".*" appended
  Transducer union_transducer;
  // The "." in ".*" is a set of equal pairs of the output symbols
  // from the monodix alphabet (<n>:<n> etc.)
  Alphabet alph_prefix = alph_bi;
  set<int> loopback_symbols;    // ints refer to alph_prefix
  alph_prefix.createLoopbackSymbols(loopback_symbols, alph_mono, Alphabet::right);

  for(std::map<UString, Transducer>::iterator it = trans_bi.begin(); it != trans_bi.end(); it++)
  {
    Transducer union_tmp = it->second;
    if(union_transducer.isEmpty())
    {
      union_transducer = union_tmp;
    }
    else
    {
      union_transducer.unionWith(alph_bi, union_tmp);
    }
  }
  union_transducer.minimize();

  Transducer prefix_transducer = union_transducer.appendDotStar(loopback_symbols);
  // prefix_transducer should _not_ be minimized (both useless and takes forever)
  Transducer moved_transducer = prefix_transducer.moveLemqsLast(alph_prefix);


  for(std::map<UString, Transducer>::iterator it = trans_mono.begin(); it != trans_mono.end(); it++)
  {
    Transducer trimmed = it->second.intersect(moved_transducer,
                                              alph_mono,
                                              alph_prefix);

    if(it->second.numberOfTransitions() == 0)
    {
      cerr << "Warning: section " << it->first << " is empty! Skipping it ..."<<endl;
      trans_mono[it->first].clear();
    }
    else if(trimmed.hasNoFinals()) {
      cerr << "Warning: section " << it->first << " had no final state after trimming! Skipping it ..."<<endl;
      trans_mono[it->first].clear();
    }
    else {
      trimmed.minimize();
      trans_mono[it->first] = trimmed;
    }
  }

  return make_pair(make_pair(alph_mono, letters_mono), trans_mono);
}


int main(int argc, char *argv[])
{
  if(argc != 4)
  {
    endProgram(argv[0]);
  }

  LtLocale::tryToSetLocale();

  FILE *analyser = fopen(argv[1], "rb");
  if(!analyser)
  {
    cerr << "Error: Cannot open file '" << argv[1] << "'." << endl << endl;
    exit(EXIT_FAILURE);
  }
  FILE *bidix = fopen(argv[2], "rb");
  if(!bidix)
  {
    cerr << "Error: Cannot open file '" << argv[2] << "'." << endl << endl;
    exit(EXIT_FAILURE);
  }

  std::pair<std::pair<Alphabet, UString>, std::map<UString, Transducer> > trimmed = trim(analyser, bidix);
  Alphabet alph_t = trimmed.first.first;
  UString letters = trimmed.first.second;
  std::map<UString, Transducer> trans_t = trimmed.second;

  // Write the file:
  FILE *output = fopen(argv[3], "wb");
  if(!output)
  {
    cerr << "Error: Cannot open file '" << argv[3] << "'." << endl << endl;
    exit(EXIT_FAILURE);
  }

  int n_trans = write_transducer_set(output, letters, alph_t, trans_t, true);

  if (n_trans == 0) {
    cerr << "Error: Trimming gave empty transducer!" << endl;
    cerr << "Hint: There are no words in bilingual dictionary that match "
      "words in both monolingual dictionaries?" << endl;
    exit(EXIT_FAILURE);
  }

  fclose(analyser);
  fclose(bidix);
  fclose(output);

  return 0;
}
