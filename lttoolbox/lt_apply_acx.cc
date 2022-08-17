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

#include <lttoolbox/file_utils.h>
#include <lttoolbox/cli.h>
#include <lttoolbox/lt_locale.h>
#include <lttoolbox/string_utils.h>
#include <lttoolbox/acx.h>

int main(int argc, char* argv[])
{
  LtLocale::tryToSetLocale();
  CLI cli("apply an ACX file to a compiled transducer", PACKAGE_VERSION);
  cli.add_file_arg("input_file", false);
  cli.add_file_arg("acx_file");
  cli.add_file_arg("output_file");
  cli.parse_args(argc, argv);

  FILE* input = openInBinFile(cli.get_files()[0]);
  auto acx = readACX(cli.get_files()[1].c_str());
  FILE* output = openOutBinFile(cli.get_files()[2]);

  Alphabet alpha;
  std::set<UChar32> letters;
  std::map<UString, Transducer> trans;
  readTransducerSet(input, letters, alpha, trans);

  for (auto& it : trans) {
    it.second.applyACX(alpha, acx);
  }

  writeTransducerSet(output, letters, alpha, trans);

  fclose(input);
  fclose(output);
  return 0;
}
