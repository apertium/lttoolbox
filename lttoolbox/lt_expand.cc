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

#include <lttoolbox/expander.h>
#include <lttoolbox/lt_locale.h>
#include <lttoolbox/file_utils.h>
#include <lttoolbox/cli.h>
#include <lttoolbox/i18n.h>

int main(int argc, char *argv[])
{
  I18n i18n {ALT_I18N_DATA, "lttoolbox"};
  LtLocale::tryToSetLocale();
  CLI cli(i18n.format("lt_expand"), PACKAGE_VERSION);
  cli.add_bool_arg('m', "keep-boundaries", i18n.format("keep_boundaries_desc"));
  cli.add_str_arg('v', "var", i18n.format("var_desc"), "VAR");
  cli.add_str_arg('a', "alt", i18n.format("alt_desc"), "ALT");
  cli.add_str_arg('l', "var-left", i18n.format("var_left_desc"), "VAR");
  cli.add_str_arg('r', "var-right", i18n.format("var_right_desc"), "VAR");
  cli.add_file_arg("dictionary_file", false);
  cli.add_file_arg("output_file");
  cli.parse_args(argc, argv);

  Expander e;
  e.setKeepBoundaries(cli.get_bools()["keep-boundaries"]);
  auto args = cli.get_strs();
  if (args.find("var") != args.end()) {
    e.setVariantValue(to_ustring(args["var"][0].c_str()));
  }
  if (args.find("alt") != args.end()) {
    e.setAltValue(to_ustring(args["alt"][0].c_str()));
  }
  if (args.find("var-left") != args.end()) {
    e.setVariantLeftValue(to_ustring(args["var-left"][0].c_str()));
  }
  if (args.find("var-right") != args.end()) {
    e.setVariantRightValue(to_ustring(args["var-right"][0].c_str()));
  }

  UFILE* output = openOutTextFile(cli.get_files()[1]);

  e.expand(cli.get_files()[0], output);
  u_fclose(output);

  return EXIT_SUCCESS;
}
