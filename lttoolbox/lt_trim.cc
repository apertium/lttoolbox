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
#include <lttoolbox/i18n.h>
#include <unicode/ustream.h>

void
trim(FILE* file_mono, FILE* file_bi, FILE* file_out, std::set<UString> match_sections)
{
  I18n i18n {ALT_I18N_DATA, "lttoolbox"};
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

  UString union_name = u""; // Not a valid section name, used as key for those where we don't care about names matching
  std::map<UString, Transducer> moved_bi_transducers;
  for (auto& it : trans_bi) {
    if(match_sections.count(it.first)) {
      moved_bi_transducers[it.first] = it.second.appendDotStar(loopback_symbols).moveLemqsLast(alph_prefix);
    }
    else {
      if (union_transducer.isEmpty()) {
        union_transducer = it.second;
      }
      else {
        union_transducer.unionWith(alph_bi, it.second);
      }
    }
  }
  union_transducer.minimize();

  // prefix/moved transducer should _not_ be minimized (both useless and takes forever)
  moved_bi_transducers[union_name] = union_transducer.appendDotStar(loopback_symbols).moveLemqsLast(alph_prefix);

  std::map<UString, Transducer> trans_trim;
  std::set<UString> sections_unmatched = match_sections; // just used to warn if user asked for a match that never happened

  for (auto& it : trans_mono) {
    if (it.second.numberOfTransitions() == 0) {
      i18n.error("ALT60410", {"section_name"}, {icu::UnicodeString(it.first.data())}, false);
      continue;
    }
    if (moved_bi_transducers.count(it.first)) {
      sections_unmatched.erase(it.first);
    }
    Transducer& moved_transducer = moved_bi_transducers.count(it.first)
                                 ? moved_bi_transducers[it.first]
                                 : moved_bi_transducers[union_name];
    Transducer trimmed = it.second.trim(moved_transducer,
                                        alph_mono,
                                        alph_prefix);
    if (trimmed.hasNoFinals()) {
      i18n.error("ALT60420", {"section_name"}, {icu::UnicodeString(it.first.data())}, false);
      continue;
    }
    trimmed.minimize();
    trans_trim[it.first] = trimmed;
  }
  for (const auto &name : sections_unmatched) {
    i18n.error("ALT60450", {"section_name"}, {icu::UnicodeString(name.data())}, false);
  }

  if (trans_trim.empty()) {
    i18n.error("ALT80460", true);
  }

  writeTransducerSet(file_out, letters_mono, alph_mono, trans_trim);
}


int main(int argc, char *argv[])
{
  I18n i18n {ALT_I18N_DATA, "lttoolbox"};
  LtLocale::tryToSetLocale();
  CLI cli(i18n.format("lt_trim_desc"), PACKAGE_VERSION);
  cli.add_file_arg("analyser_bin_file", false);
  cli.add_file_arg("bidix_bin_file");
  cli.add_file_arg("trimmed_bin_file");
  cli.add_str_arg('s', "match-section", i18n.format("match_section_desc"), "section_name");
  cli.parse_args(argc, argv);

  auto strs = cli.get_strs();
  std::set<UString> match_sections;
  if (strs.find("match-section") != strs.end()) {
    for (auto &it : strs["match-section"]) {
      match_sections.insert(to_ustring(it.c_str()));
    }
  }
  FILE* analyser = openInBinFile(cli.get_files()[0]);
  FILE* bidix = openInBinFile(cli.get_files()[1]);
  FILE* output = openOutBinFile(cli.get_files()[2]);

  trim(analyser, bidix, output, match_sections);

  fclose(analyser);
  fclose(bidix);
  fclose(output);

  return 0;
}
