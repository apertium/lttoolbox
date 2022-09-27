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

void get_symbol(const std::string& s, Alphabet& alpha, const char* prefix,
                sorted_vector<int32_t>& vec)
{
  UString t;
  t += '<';
  t += to_ustring(prefix);
  t += ':';
  t += to_ustring(s.c_str());
  t += '>';
  if (alpha.isSymbolDefined(t)) {
    vec.insert(alpha(alpha(t), alpha(t)));
  }
}

int main(int argc, char* argv[])
{
  LtLocale::tryToSetLocale();
  CLI cli("remove paths from a transducer", PACKAGE_VERSION);
  cli.add_bool_arg('m', "minimise", "minimise transducers after deleting paths");
  cli.add_str_arg('v', "var", "set language variant", "VAR");
  cli.add_str_arg('a', "alt", "set alternative (monodix)", "ALT");
  cli.add_str_arg('l', "var-left", "set left language variant (bidix)", "VAR");
  cli.add_str_arg('r', "var-right", "set right language variant (bidix)", "VAR");
  cli.add_file_arg("lr | rl", false);
  cli.add_file_arg("input_file");
  cli.add_file_arg("output_file");
  cli.parse_args(argc, argv);

  std::string dir = cli.get_files()[0];
  if (dir == "lr") dir = "LR";
  else if (dir == "rl") dir = "RL";
  FILE* input = openInBinFile(cli.get_files()[1]);
  FILE* output = openOutBinFile(cli.get_files()[2]);

  Alphabet alpha;
  std::set<UChar32> letters;
  std::map<UString, Transducer> trans;
  readTransducerSet(input, letters, alpha, trans);

  sorted_vector<int32_t> keep;
  sorted_vector<int32_t> drop;
  bool has_var = false;
  get_symbol(dir, alpha, "r", keep);
  for (auto& it : cli.get_strs()["var"]) {
    get_symbol(it, alpha, "v", keep);
    has_var = true;
  }
  for (auto& it : cli.get_strs()["alt"]) {
    get_symbol(it, alpha, "alt", keep);
  }
  for (auto& it : cli.get_strs()["var-left"]) {
    get_symbol(it, alpha, "vl", keep);
  }
  for (auto& it : cli.get_strs()["var-right"]) {
    get_symbol(it, alpha, "vr", keep);
  }

  for (int32_t i = 1; i <= alpha.size(); i++) {
    UString t;
    alpha.getSymbol(t, -i);
    if (StringUtils::startswith(t, u"<r:") ||
        (has_var && StringUtils::startswith(t, u"<v:")) ||
        StringUtils::startswith(t, u"<alt:") ||
        StringUtils::startswith(t, u"<vl:") ||
        StringUtils::startswith(t, u"<vr:")) {
      int32_t s = alpha(-i, -i);
      if (!keep.count(s)) {
        drop.insert(s);
      }
    } else if (!has_var && StringUtils::startswith(t, u"<v:")) {
      keep.insert(alpha(-i, -i));
    }
  }

  bool min = cli.get_bools()["minimise"];
  if (!min) {
    auto LT_RELEASE = std::getenv("LT_RELEASE");
    min = (LT_RELEASE != NULL && LT_RELEASE[0] != 'n');
  }

  for (auto& it : trans) {
    it.second.deleteSymbols(drop);
    it.second.epsilonizeSymbols(keep);
    if (dir == "RL") {
      it.second.invert(alpha);
    }
    if (min) {
      it.second.minimize();
    }
  }

  writeTransducerSet(output, letters, alpha, trans);

  fclose(input);
  fclose(output);
  return 0;
}
