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

int main(int argc, char *argv[])
{
  LtLocale::tryToSetLocale();
  CLI cli("dump a transducer to text in ATT format", PACKAGE_VERSION);
  cli.add_bool_arg('a', "alpha", "print transducer alphabet");
  cli.add_bool_arg('H', "hfst", "use HFST-compatible character escapes");
  cli.add_bool_arg('h', "help", "print this message and exit");
  cli.add_file_arg("bin_file");
  cli.add_file_arg("output_file");
  cli.parse_args(argc, argv);

  bool alpha = cli.get_bools()["alpha"];
  bool hfst = cli.get_bools()["hfst"];

  FILE* input = openInBinFile(cli.get_files()[0]);
  UFILE* output = openOutTextFile(cli.get_files()[1]);

  Alphabet alphabet;
  std::set<UChar32> alphabetic_chars;
  std::map<UString, Transducer> transducers;

  readTransducerSet(input, alphabetic_chars, alphabet, transducers);

  /////////////////////

  if (alpha) {
    for (auto& it : alphabetic_chars) {
      u_fprintf(output, "%C\n", it);
    }
    for (int i = 1; i <= alphabet.size(); i++) {
      alphabet.writeSymbol(-i, output);
      u_fprintf(output, "\n");
    }
  } else {
    bool first = true;
    for (auto& it : transducers) {
      if (!first) {
        u_fprintf(output, "--\n");
      }
      it.second.joinFinals();
      it.second.show(alphabet, output, 0, hfst);
      first = false;
    }
  }

  fclose(input);
  u_fclose(output);

  return 0;
}
