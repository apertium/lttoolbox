/*
 * Copyright (C) 2022 Apertium
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

int main(int argc, char *argv[])
{
  LtLocale::tryToSetLocale();
  CLI cli("add sections to a compiled transducer", PACKAGE_VERSION);
  cli.add_bool_arg('k', "keep", "in case of section name conflicts, keep the one from the first transducer");
  cli.add_bool_arg('s', "single", "treat input transducers as one-sided");
  cli.add_bool_arg('h', "help", "print this message and exit");
  cli.add_file_arg("bin_file1", false);
  cli.add_file_arg("bin_file2");
  cli.add_file_arg("output_file");
  cli.parse_args(argc, argv);

  bool pairs = !cli.get_bools()["single"];
  bool keep = cli.get_bools()["keep"];

  FILE* input1 = openInBinFile(cli.get_files()[0]);
  FILE* input2 = openInBinFile(cli.get_files()[1]);
  FILE* output = openOutBinFile(cli.get_files()[2]);

  Alphabet alpha1, alpha2;
  std::set<UChar32> chars1, chars2;
  std::map<UString, Transducer> trans1, trans2;

  readTransducerSet(input1, chars1, alpha1, trans1);
  readTransducerSet(input2, chars2, alpha2, trans2);

  for (auto& it : chars2) {
    chars1.insert(it);
  }
  UString chars(chars1.begin(), chars1.end());

  for (auto& it : trans2) {
    if (trans1.find(it.first) != trans1.end()) {
      if (keep) {
        continue;
      } else {
        std::cerr << "WARNING: section '" << it.first << "' appears in both transducers and will be overwritten!" << std::endl;
      }
    }
    it.second.updateAlphabet(alpha2, alpha1, pairs);
    trans1[it.first] = it.second;
  }

  writeTransducerSet(output, chars, alpha1, trans1);

  fclose(input1);
  fclose(input2);
  fclose(output);

  return 0;
}
