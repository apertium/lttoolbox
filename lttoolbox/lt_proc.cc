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
#include <lttoolbox/file_utils.h>
#include <lttoolbox/cli.h>
#include <lttoolbox/lt_locale.h>
#include <lttoolbox/i18n.h>

void checkValidity(FSTProcessor const &fstp)
{
  if(!fstp.valid())
  {
    exit(EXIT_FAILURE);
  }
}

int main(int argc, char *argv[])
{
  LtLocale::tryToSetLocale();
  I18n i18n {ALT_I18N_DATA, "lttoolbox"};

  CLI cli(i18n.format("lt_proc_desc"), PACKAGE_VERSION);
  cli.add_file_arg("fst_file", false);
  cli.add_file_arg("input_file");
  cli.add_file_arg("output_file");
  cli.add_bool_arg('a', "analysis", i18n.format("analysis_desc"));
  cli.add_bool_arg('b', "bilingual", i18n.format("bilingual_desc"));
  cli.add_bool_arg('c', "case-sensitive", i18n.format("case_sensitive_desc"));
  cli.add_bool_arg('d', "debugged-gen", i18n.format("debugged_gen_desc"));
  cli.add_bool_arg('e', "decompose-nouns", i18n.format("decompose_nouns_desc"));
  cli.add_bool_arg('g', "generation", i18n.format("generation_desc"));
  cli.add_str_arg('i', "ignored-chars", i18n.format("ignored_chars_desc"), "icx_file");
  cli.add_str_arg('r', "restore-chars", i18n.format("restore_chars_desc"), "rcx_file");
  cli.add_bool_arg('l', "tagged-gen", i18n.format("tagged_gen_desc"));
  cli.add_bool_arg('m', "tagged-nm-gen", i18n.format("tagged_nm_gen_desc"));
  cli.add_bool_arg('n', "non-marked-gen", i18n.format("non_marked_gen_desc"));
  cli.add_bool_arg('o', "surf-bilingual", i18n.format("surf_bilingual_desc"));
  cli.add_bool_arg('p', "post-generation", i18n.format("post_generation_desc"));
  cli.add_bool_arg('x', "inter-generation", i18n.format("inter_generation_desc"));
  cli.add_bool_arg('s', "sao", i18n.format("sao_desc"));
  cli.add_bool_arg('t', "transliteration", i18n.format("transliteration_desc"));
  cli.add_bool_arg('v', "version", i18n.format("version_desc"));
  cli.add_bool_arg('z', "null-flush", i18n.format("null_flush_desc"));
  cli.add_bool_arg('w', "dictionary-case", i18n.format("dictionary_case_desc"));
  cli.add_bool_arg('C', "careful-case", i18n.format("careful_case_desc"));
  cli.add_bool_arg('I', "no-default-ignore", i18n.format("no_default_ignore_desc"));
  cli.add_bool_arg('W', "show-weights", i18n.format("show_weights_desc"));
  cli.add_str_arg('N', "analyses", i18n.format("analyses_desc"), "N");
  cli.add_str_arg('L', "weight-classes", i18n.format("weight_classes_desc"), "N");
  cli.add_str_arg('M', "compound-max-elements", i18n.format("compound_max_elements_desc"), "N");
  cli.add_bool_arg('h', "help", i18n.format("help_desc"));
  cli.parse_args(argc, argv);

  FSTProcessor fstp;
  GenerationMode bilmode = gm_unknown;
  char cmd = 0;

  auto args = cli.get_bools();
  if (args["analysis"]) {
    cmd = 'a';
  }
  if (args["bilingual"]) {
    if (cmd) cli.print_usage();
    cmd = 'b';
  }
  if (args["surf-bilingual"]) {
    if (cmd && cmd != 'b') cli.print_usage();
    if (!cmd) cmd = 'b';
    fstp.setBiltransSurfaceForms(true);
  }
  if (args["generation"]) {
    if (cmd && cmd != 'b') cli.print_usage();
    if (!cmd) cmd = 'g';
  }
  if (args["decompose-nouns"]) {
    if (cmd) cli.print_usage();
    cmd = 'e';
  }
  if (args["post-generation"]) {
    if (cmd) cli.print_usage();
    cmd = 'p';
  }
  if (args["inter-generation"] || args["transliteration"]) {
    if (cmd) cli.print_usage();
    cmd = 't';
  }
  if (args["sao"]) {
    if (cmd) cli.print_usage();
    cmd = 's';
  }

  if (args["debugged-gen"]) {
    if (!cmd) cmd = 'g';
    bilmode = gm_all;
  }
  if (args["tagged-gen"]) {
    if (!cmd) cmd = 'g';
    bilmode = gm_tagged;
  }
  if (args["tagged-nm-gen"]) {
    if (!cmd) cmd = 'g';
    bilmode = gm_tagged_nm;
  }
  if (args["non-marked-gen"]) {
    if (!cmd) cmd = 'g';
    bilmode = gm_clean;
  }
  if (args["careful-case"]) {
    if (!cmd) cmd = 'g';
    bilmode = gm_carefulcase;
  }

  fstp.setCaseSensitiveMode(cli.get_bools()["case-sensitive"]);
  fstp.setUseDefaultIgnoredChars(!cli.get_bools()["no-default-ignore"]);
  fstp.setDisplayWeightsMode(cli.get_bools()["show-weights"]);
  fstp.setNullFlush(cli.get_bools()["null-flush"]);
  fstp.setDictionaryCaseMode(cli.get_bools()["dictionary-case"]);

  auto strs = cli.get_strs();
  if (strs.find("ignored-chars") != strs.end()) {
    fstp.setIgnoredChars(true);
    for (auto& it : strs["ignored-chars"]) {
      fstp.parseICX(it);
    }
  }
  if (strs.find("restore-chars") != strs.end()) {
    fstp.setRestoreChars(true);
    fstp.setUseDefaultIgnoredChars(false);
    for (auto& it : strs["restore-chars"]) {
      fstp.parseRCX(it);
    }
  }
  if (strs.find("analyses") != strs.end()) {
    int n = atoi(strs["analyses"].back().c_str());
    if (n < 1) {
      i18n.error("ALT80000", {"option"}, {"analyses"}, true);
    }
    fstp.setMaxAnalysesValue(n);
  }
  if (strs.find("weight-classes") != strs.end()) {
    int n = atoi(strs["weight-classes"].back().c_str());
    if (n < 1) {
      i18n.error("ALT80000", {"option"}, {"weight-classes"}, true);
    }
    fstp.setMaxWeightClassesValue(n);
  }
  if (strs.find("compound-max-elements") != strs.end()) { // Test
    int n = atoi(strs["compound-max-elements"].back().c_str());
    if (n < 1) {
      i18n.error("ALT80000", {"option"}, {"compound-max-elements"}, true);
    }
    fstp.setCompoundMaxElements(n);
  }

  FILE* in = openInBinFile(cli.get_files()[0]);
  fstp.load(in);
  fclose(in);

  InputFile input;
  if (!cli.get_files()[1].empty()) {
    input.open_or_exit(cli.get_files()[1].c_str());
  }
  UFILE* output = openOutTextFile(cli.get_files()[2]);

  try
  {
    switch(cmd)
    {
      case 'g':
        fstp.initGeneration();
        checkValidity(fstp);
        fstp.generation(input, output, bilmode);
        break;

      case 'p':
        fstp.initPostgeneration();
        checkValidity(fstp);
        fstp.postgeneration(input, output);
        break;

      case 's':
        fstp.initAnalysis();
        checkValidity(fstp);
        fstp.SAO(input, output);
        break;

      case 't':
        fstp.initPostgeneration();
        checkValidity(fstp);
        fstp.transliteration(input, output);
        break;

      case 'b':
        fstp.initBiltrans();
        checkValidity(fstp);
        fstp.bilingual(input, output, bilmode);
        break;

      case 'e':
        fstp.initDecomposition();
        checkValidity(fstp);
        fstp.analysis(input, output);
        break;

      case 'a':
      default:
        fstp.initAnalysis();
        checkValidity(fstp);
        fstp.analysis(input, output);
        break;
    }
  }
  catch (std::exception& e)
  {
    std::cerr << e.what();
    if (fstp.getNullFlush()) {
      u_fputc('\0', output);
    }

    exit(1);
  }

  u_fclose(output);
  return EXIT_SUCCESS;
}
