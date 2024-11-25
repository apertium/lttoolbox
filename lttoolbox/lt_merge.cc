/*
 * Copyright (C) 2024 Universitat d'Alacant / Universidad de Alicante
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
#include <lttoolbox/fst_processor.h>
#include <lttoolbox/file_utils.h>
#include <lttoolbox/cli.h>
#include <lttoolbox/lt_locale.h>
#include <iostream>


int main(int argc, char *argv[])
{
  LtLocale::tryToSetLocale();
  CLI cli("merge lexical units from the one tagged BEG until END", PACKAGE_VERSION);
  cli.add_file_arg("input_file");
  cli.add_file_arg("output_file");
  cli.add_bool_arg('u', "unmerge", "Undo the merge");
  cli.parse_args(argc, argv);

  auto strs = cli.get_strs();
  bool unmerge = cli.get_bools()["unmerge"];
  InputFile input;
  if (!cli.get_files()[1].empty()) {
    input.open_or_exit(cli.get_files()[0].c_str());
  }
  UFILE* output = openOutTextFile(cli.get_files()[1]);

  FSTProcessor fstp;
  fstp.initBiltrans();
  fstp.quoteMerge(input, output);

  return 0;
}
