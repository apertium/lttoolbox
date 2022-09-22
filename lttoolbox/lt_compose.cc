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
#include <thread>
#include <future>

void
compose(FILE* file_f, FILE* file_g, FILE* file_gf, bool f_inverted, bool g_anywhere, bool jobs)
{
  Alphabet alph_f;
  std::set<UChar32> letters_f;
  std::map<UString, Transducer> trans_f;
  readTransducerSet(file_f, letters_f, alph_f, trans_f);
  Alphabet alph_g;
  std::set<UChar32> letters_g;
  std::map<UString, Transducer> trans_g;
  readTransducerSet(file_g, letters_g, alph_g, trans_g);

  std::map<UString, Transducer> trans_gf;

  Transducer union_g;
  for (auto& it : trans_g) {
    if (union_g.isEmpty()) {
      union_g = it.second;
    } else {
      union_g.unionWith(alph_g, it.second);
    }
  }
  union_g.minimize();

  std::vector<std::future<std::pair<UString, Transducer>>> compositions;
  for (auto& it : trans_f) {
    if (it.second.numberOfTransitions() == 0) {
      std::cerr << "Warning: section " << it.first << " is empty! Skipping it..." << std::endl;
      continue;
    }
    if(jobs) {
      compositions.push_back(std::async(
          [](Transducer &f, Transducer &g, Alphabet &alph_f, Alphabet &alph_g,
             bool f_inverted, bool g_anywhere, UString name) {
            Transducer gf = f.compose(g, alph_f, alph_g, f_inverted, g_anywhere);
            if (gf.hasNoFinals()) {
              std::cerr << "Warning: section " << name
                        << " had no final state after composing! Skipping it..."
                        << std::endl;
              ;
            } else {
              gf.minimize();
            }
            return std::make_pair(name, gf);
          },
          std::ref(it.second), std::ref(union_g), std::ref(alph_f),
          std::ref(alph_g), f_inverted, g_anywhere, it.first));
    } else {
      Transducer gf = it.second.compose(union_g, alph_f, alph_g, f_inverted, g_anywhere);
      if (gf.hasNoFinals()) {
        std::cerr << "Warning: section " << it.first
                  << " had no final state after composing! Skipping it..."
                  << std::endl;
        continue;
      }
      gf.minimize();
      trans_gf[it.first] = gf;
    }
  }
  for (auto &thr : compositions) {
    auto it = thr.get();
    if (!it.second.hasNoFinals()) {
      trans_gf[it.first] = it.second;
    }
  }

  if (trans_gf.empty()) {
    std::cerr << "Error: Composition gave empty transducer!" << std::endl;
    exit(EXIT_FAILURE);
  }

  writeTransducerSet(file_gf, letters_f, alph_f, trans_gf);
}


int main(int argc, char *argv[])
{
  LtLocale::tryToSetLocale();
  CLI cli("compose transducer1 with transducer2", PACKAGE_VERSION);
  cli.add_bool_arg('i', "inverted", "run composition right-to-left on transducer1");
  cli.add_bool_arg('a', "anywhere", "don't require anchored matches, let transducer2 optionally compose at any sub-path");
  cli.add_file_arg("transducer1_bin_file", false);
  cli.add_file_arg("transducer2_bin_file");
  cli.add_file_arg("trimmed_bin_file");
  cli.parse_args(argc, argv);

  FILE* transducer1 = openInBinFile(cli.get_files()[0]);
  FILE* transducer2 = openInBinFile(cli.get_files()[1]);
  FILE* composition = openOutBinFile(cli.get_files()[2]);

  bool jobs = false;
  auto LT_JOBS = std::getenv("LT_JOBS");
  if(cli.get_bools()["jobs"] || (LT_JOBS != NULL && LT_JOBS[0] != 'n')) {
    jobs = true;
  }
  compose(transducer1, transducer2, composition,
          cli.get_bools()["inverted"],
          cli.get_bools()["anywhere"],
          jobs);

  fclose(transducer1);
  fclose(transducer2);
  fclose(composition);

  return 0;
}
