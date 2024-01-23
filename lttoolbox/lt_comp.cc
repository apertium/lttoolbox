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
#include <lttoolbox/compiler.h>
#include <lttoolbox/att_compiler.h>
#include <lttoolbox/lt_locale.h>
#include <lttoolbox/cli.h>
#include <lttoolbox/file_utils.h>

#include <iostream>
#include <lttoolbox/i18n.h>

/*
 * Error function that does nothing so that when we fallback from
 * XML to AT&T, the user doesn't get a message unless it's really
 * invalid XML.
 */
void errorFunc(void *ctx, const char *msg, ...)
{
  return;
}

int main(int argc, char *argv[])
{
  I18n i18n(ALT_I18N_DATA, "lttoolbox");
  LtLocale::tryToSetLocale();
  CLI cli(i18n.format("lt_comp_desc"), PACKAGE_VERSION);
  cli.add_bool_arg('d', "debug", i18n.format("debug_desc"));
  cli.add_bool_arg('m', "keep-boundaries", i18n.format("keep_boundaries_desc"));
  cli.add_str_arg('v', "var", i18n.format("var_desc"), "VAR");
  cli.add_str_arg('a', "alt", i18n.format("alt_desc"), "ALT");
  cli.add_str_arg('l', "var-left", i18n.format("var_left_desc"), "VAR");
  cli.add_str_arg('r', "var-right", i18n.format("var_right_desc"), "VAR");
  cli.add_bool_arg('H', "hfst", i18n.format("expect_hfst_desc"));
  cli.add_bool_arg('S', "no-split", i18n.format("no_split_desc"));
  cli.add_bool_arg('j', "jobs", i18n.format("jobs_desc"));
  cli.add_bool_arg('V', "verbose", i18n.format("verbose_desc"));
  cli.add_bool_arg('h', "help", i18n.format("help_desc"));
  cli.add_file_arg("lr | rl | u", false);
  cli.add_file_arg("dictionary_file", false);
  cli.add_file_arg("output_file", false);
  cli.add_file_arg("acx_file", true);
  cli.parse_args(argc, argv);

  char ttype = 'x';
  Compiler c;
  AttCompiler a;

  bool have_vl = false;
  bool have_vr = false;
  auto args = cli.get_strs();
  if (args.find("var") != args.end()) {
    c.setVariantValue(to_ustring(args["var"][0].c_str()));
  }
  if (args.find("alt") != args.end()) {
    c.setAltValue(to_ustring(args["alt"][0].c_str()));
  }
  if (args.find("var-left") != args.end()) {
    have_vl = true;
    c.setVariantLeftValue(to_ustring(args["var-left"][0].c_str()));
  }
  if (args.find("var-right") != args.end()) {
    have_vr = true;
    c.setVariantRightValue(to_ustring(args["var-right"][0].c_str()));
  }

  c.setEntryDebugging(cli.get_bools()["debug"]);
  c.setKeepBoundaries(cli.get_bools()["keep-boundaries"]);
  c.setVerbose(cli.get_bools()["verbose"]);

  a.setHfstSymbols(cli.get_bools()["hfst"]);
  a.setSplitting(!cli.get_bools()["no-split"]);

  auto LT_JOBS = std::getenv("LT_JOBS");
  if(cli.get_bools()["jobs"] || (LT_JOBS != NULL && LT_JOBS[0] != 'n')) {
    c.setJobs(true);
    c.setMaxSectionEntries(50000);
  }
  else {
    c.setJobs(false);
    c.setMaxSectionEntries(0);
  }
  if(const char* max_section_entries = std::getenv("LT_MAX_SECTION_ENTRIES")) {
    c.setMaxSectionEntries(std::stol(max_section_entries));
  }

  std::string opc = cli.get_files()[0];
  std::string infile = cli.get_files()[1];
  std::string outfile = cli.get_files()[2];
  std::string acxfile = cli.get_files()[3];

  xmlTextReaderPtr reader;
  reader = xmlReaderForFile(infile.c_str(), NULL, 0);
  xmlGenericErrorFunc handler = (xmlGenericErrorFunc)errorFunc;
  initGenericErrorDefaultFunc(&handler);
  if(reader != NULL)
  {
    int ret = xmlTextReaderRead(reader);
    if(ret != 1)
    {
      ttype = 'a';
    }
    xmlFreeTextReader(reader);
    xmlCleanupParser();
  }
  else
  {
    i18n.error("ALT80050", {"file_name"}, {infile.c_str()}, true);
  }
  initGenericErrorDefaultFunc(NULL);


  if(opc == "lr")
  {
    if (have_vl) {
      i18n.error("ALT80390", false);
      cli.print_usage();
    }
    if(ttype == 'a')
    {
      a.parse(infile, false);
    }
    else
    {
      if(!acxfile.empty())
      {
        c.parseACX(acxfile, Compiler::COMPILER_RESTRICTION_LR_VAL);
      }
      c.parse(infile, Compiler::COMPILER_RESTRICTION_LR_VAL);
    }
  }
  else if(opc == "rl")
  {
    if (have_vr) {
      i18n.error("ALT80391", false);
      cli.print_usage();
    }
    if(ttype == 'a')
    {
      a.parse(infile, true);
    }
    else
    {
      c.parse(infile, Compiler::COMPILER_RESTRICTION_RL_VAL);
    }
  }
  else if (opc == "u") {
    if (ttype == 'a') {
      a.parse(infile, false);
    } else {
      c.parse(infile, Compiler::COMPILER_RESTRICTION_U_VAL);
    }
  }
  else
  {
    cli.print_usage();
  }

  FILE* output = openOutBinFile(outfile);
  if(ttype == 'a')
  {
    a.write(output);
  }
  else
  {
    c.write(output);
  }
  fclose(output);
}
