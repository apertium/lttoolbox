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
#include <lttoolbox/tmx_compiler.h>
#include <lttoolbox/lt_locale.h>
#include <cstdlib>
#include <iostream>
#include <libgen.h>
#include <string>
#include <getopt.h>
#include <lttoolbox/i18n.h>
#include <unicode/ustream.h>


void endProgram(char *name)
{
  I18n i18n {ALT_I18N_DATA, "lttoolbox"};
  if(name != NULL)
  {
    std::cout << basename(name) << " v" << PACKAGE_VERSION << ": " << i18n.format("lt_tmxcomp_desc") << std::endl;
    std::cout << i18n.format("usage") << basename(name) << " [OPTIONS] lang1-lang2 tmx_file output_file" << std::endl;
    std::cout << i18n.format("modes") << std::endl;
    std::cout << "  lang1:     " << i18n.format("input_language") << std::endl;
    std::cout << "  lang2:     " << i18n.format("output_language") << std::endl;
    std::cout << i18n.format("options") << std::endl;
#if HAVE_GETOPT_LONG
    std::cout << "  -o, --origin-code code   " << i18n.format("origin_code_desc") << std::endl;
    std::cout << "  -m, --meta-code code     " << i18n.format("meta_code_desc") << std::endl;
#else
    std::cout << "  -o code   " << i18n.format("origin_code_desc") << std::endl;
    std::cout << "  -m code   " << i18n.format("meta_code_desc") << std::endl;
#endif
  }
  exit(EXIT_FAILURE);
}


int main(int argc, char *argv[])
{
  LtLocale::tryToSetLocale();

  if(argc != 4)
  {
    endProgram(argv[0]);
  }

  TMXCompiler c;

#if HAVE_GETOPT_LONG
  int option_index = 0;
#endif
  while(true)
  {
#if HAVE_GETOPT_LONG
    static struct option long_options[] =
    {
      {"origin-code", required_argument, 0, 'o'},
      {"meta-code", required_argument, 0, 'm'},
      {0, 0, 0, 0}
    };

    int c_t = getopt_long(argc, argv, "o:m:", long_options, &option_index);
#else
    int c_t = getopt(argc, argv, "o:m:");
#endif
    if(c_t == -1)
    {
      break;
    }

    switch(c_t)
    {
      case 'o':
        c.setOriginLanguageCode(to_ustring(optarg));
        break;

      case 'm':
        c.setMetaLanguageCode(to_ustring(optarg));
        break;

      default:
        endProgram(argv[0]);
        break;
    }
  }

  UString opc = to_ustring(argv[argc-3]);
  UString lo = opc.substr(0, opc.find('-'));
  UString lm = opc.substr(opc.find('-')+1);

  if(lo.empty() || lm.empty()) {
    endProgram(argv[0]);
  }

  c.parse(argv[argc-2], lo, lm);

  FILE *output = fopen(argv[argc-1], "wb");
  if(!output)
  {
    I18n(ALT_I18N_DATA, "lttoolbox").error("ALT80050", {"file_name"}, {argv[2]}, true);
  }
  c.write(output);
  fclose(output);
}
