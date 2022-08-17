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
#include <lttoolbox/file_utils.h>
#include <lttoolbox/cli.h>
#include <lttoolbox/lt_locale.h>
#include <iostream>

void
trim(FILE* file_mono, FILE* file_bi, FILE* file_out)
{
  Alphabet alph_mono;
  std::set<UChar32> letters_mono;
  std::map<UString, Transducer> trans_mono;
  readTransducerSet(file_mono, letters_mono, alph_mono, trans_mono);
  Alphabet alph_bi;
  std::set<UChar32> letters_bi;
  std::map<UString, Transducer> trans_bi;
  readTransducerSet(file_bi, letters_bi, alph_bi, trans_bi);

  // The prefix transducer is the union of all transducers from bidix,
  // with a ".*" appended
  Transducer union_transducer;
  // The "." in ".*" is a set of equal pairs of the output symbols
  // from the monodix alphabet (<n>:<n> etc.)
  Alphabet alph_prefix = alph_bi;
  std::set<int> loopback_symbols;    // ints refer to alph_prefix
  alph_prefix.createLoopbackSymbols(loopback_symbols, alph_mono, Alphabet::right);

  for (auto& it : trans_bi) {
    if (union_transducer.isEmpty()) {
      union_transducer = it.second;
    } else {
      union_transducer.unionWith(alph_bi, it.second);
    }
  }
  union_transducer.minimize();

  Transducer prefix_transducer = union_transducer.appendDotStar(loopback_symbols);
  // prefix_transducer should _not_ be minimized (both useless and takes forever)
  Transducer moved_transducer = prefix_transducer.moveLemqsLast(alph_prefix);

  std::map<UString, Transducer> trans_trim;

  for (auto& it : trans_mono) {
    if (it.second.numberOfTransitions() == 0) {
      std::cerr << "Warning: section " << it.first << " is empty! Skipping it..." << std::endl;
      continue;
    }
    Transducer trimmed = it.second.intersect(moved_transducer,
                                             alph_mono,
                                             alph_prefix);
    if (trimmed.hasNoFinals()) {
      std::cerr << "Warning: section " << it.first << " had no final state after trimming! Skipping it..." << std::endl;
      continue;
    }
    trimmed.minimize();
    trans_trim[it.first] = trimmed;
  }

  if (trans_trim.empty()) {
    std::cerr << "Error: Trimming gave empty transducer!" << std::endl;
    std::cerr << "Hint: There are no words in bilingual dictionary that match "
      "words in both monolingual dictionaries?" << std::endl;
    exit(EXIT_FAILURE);
  }

  writeTransducerSet(file_out, letters_mono, alph_mono, trans_trim);
}


int main(int argc, char *argv[])
{
  LtLocale::tryToSetLocale();
  CLI cli("trim a transducer to another transducer", PACKAGE_VERSION);
  cli.add_file_arg("analyser_bin_file", false);
  cli.add_file_arg("bidix_bin_file");
  cli.add_file_arg("trimmed_bin_file");
  cli.parse_args(argc, argv);

  FILE* analyser = openInBinFile(cli.get_files()[0]);
  FILE* bidix = openInBinFile(cli.get_files()[1]);
  FILE* output = openOutBinFile(cli.get_files()[2]);

  trim(analyser, bidix, output);

  fclose(analyser);
  fclose(bidix);
  fclose(output);

  return 0;
}
