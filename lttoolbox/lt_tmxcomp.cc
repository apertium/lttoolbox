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


void endProgram(char *name)
{
  if(name != NULL)
  {
    std::cout << basename(name) << " v" << PACKAGE_VERSION <<": build a letter transducer from a TMX translation memory" << std::endl;
    std::cout << "USAGE: " << basename(name) << " [OPTIONS] lang1-lang2 tmx_file output_file" << std::endl;
    std::cout << "Modes:" << std::endl;
    std::cout << "  lang1:     input language" << std::endl;
    std::cout << "  lang2:     output language" << std::endl;
    std::cout << "Options:" << std::endl;
#if HAVE_GETOPT_LONG
    std::cout << "  -o, --origin-code code   the language code to be taken as lang1" << std::endl;
    std::cout << "  -m, --meta-code code     the language code to be taken as lang2" << std::endl;
#else
    std::cout << "  -o code   the language code to be taken as lang1" << std::endl;
    std::cout << "  -m code   the language code to be taken as lang2" << std::endl;
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
    std::cerr << "Error: Cannot open file '" << argv[2] << "'." << std::endl;
    exit(EXIT_FAILURE);
  }
  c.write(output);
  fclose(output);
}
