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
#include <lttoolbox/fst_processor.h>
#include <lttoolbox/lt_locale.h>
#include <lttoolbox/cli.h>
#include <lttoolbox/file_utils.h>
#include <i18n.h>

int main(int argc, char *argv[])
{
  I18n i18n {LOCALES_DATA};
  LtLocale::tryToSetLocale();
  CLI cli(i18n.format("lt_tmxproc_desc"));
  cli.add_file_arg("fst_file", false);
  cli.add_file_arg("input_file");
  cli.add_file_arg("output_file");
  cli.parse_args(argc, argv);

  FSTProcessor fstp;
  FILE* aux = openInBinFile(cli.get_files()[0]);
  fstp.load(aux);
  fclose(aux);
  fstp.initTMAnalysis();
  if (!fstp.valid()) {
    return EXIT_FAILURE;
  }

  InputFile input;
  if (!cli.get_files()[1].empty()) {
    input.open_or_exit(cli.get_files()[1].c_str());
  }
  UFILE* output = openOutTextFile(cli.get_files()[2].c_str());

  fstp.tm_analysis(input, output);

  u_fclose(output);
  return EXIT_SUCCESS;
}
