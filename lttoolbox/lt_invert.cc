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
#include <lttoolbox/lt_locale.h>
#include <lttoolbox/cli.h>

int main(int argc, char* argv[])
{
  LtLocale::tryToSetLocale();

  CLI cli("reverse the direction of a compiled transducer", PACKAGE_VERSION);
  cli.add_bool_arg('h', "help", "print this message and exit");
  cli.add_file_arg("in_bin");
  cli.add_file_arg("out_bin");
  cli.parse_args(argc, argv);

  FILE* input = openInBinFile(cli.get_files()[0]);
  FILE* output = openOutBinFile(cli.get_files()[1]);

  Alphabet alphabet;
  std::set<UChar32> alphabetic_chars;
  std::map<UString, Transducer> transducers;
  readTransducerSet(input, alphabetic_chars, alphabet, transducers);

  for (auto& it : transducers) {
    it.second.invert(alphabet);
  }

  writeTransducerSet(output, alphabetic_chars, alphabet, transducers);

  fclose(input);
  fclose(output);
  return EXIT_SUCCESS;
}
