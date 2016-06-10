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
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */
#include <lttoolbox/tmx_compiler.h>
#include <lttoolbox/lttoolbox_config.h>
#include <lttoolbox/lt_locale.h>
#include <cstdlib>
#include <iostream>
#include <libgen.h>
#include <string>
#include <getopt.h>

using namespace std;

void endProgram(char *name)
{
  if(name != NULL)
  {
    cout << basename(name) << " v" << PACKAGE_VERSION <<": build a letter transducer from a TMX translation memory" << endl;
    cout << "USAGE: " << basename(name) << " [OPTIONS] lang1-lang2 tmx_file output_file" << endl;
    cout << "Modes:" << endl;
    cout << "  lang1:     input language" << endl;
    cout << "  lang2:     output language" << endl;
    cout << "Options:" <<endl;
#if HAVE_GETOPT_LONG
    cout << "  -o, --origin-code code   the language code to be taken as lang1" << endl;
    cout << "  -m, --meta-code code     the language code to be taken as lang2" << endl;
#else
    cout << "  -o code   the language code to be taken as lang1" << endl;
    cout << "  -m code   the language code to be taken as lang2" << endl;
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
        {
          wchar_t *param = new wchar_t[strlen(optarg)+1];
          if((size_t) -1 != mbstowcs(param, optarg, strlen(optarg)))
	  {
            c.setOriginLanguageCode(param);
	  }
    	  delete[] param;
        }
        break;
      
      case 'm':
        {
          wchar_t *param = new wchar_t[strlen(optarg)+1];
          if((size_t) -1 != mbstowcs(param, optarg, strlen(optarg)))
	  {
            c.setMetaLanguageCode(param);
	  }
    	  delete[] param;
        }
        break;
        
      default:
        endProgram(argv[0]);
        break;
    }
  }

  string opc = argv[argc-3];
  wchar_t* lo = new wchar_t[opc.size()+1];
  wchar_t* lm = new wchar_t[opc.size()+1];

  if(((size_t) -1 == mbstowcs(lo, opc.substr(0, opc.find('-')).c_str(), opc.size()))|| 
     ((size_t) -1 == mbstowcs(lm, opc.substr(opc.find('-')+1).c_str(), opc.size())))
  {
    delete[] lo;
    delete[] lm;
    endProgram(argv[0]);
  }


  c.parse(argv[argc-2], lo, lm);
  delete[] lo;
  delete[] lm;

  FILE *output = fopen(argv[argc-1], "w");
  if(!output)
  {
    wcerr << "Error: Cannot open file '" << argv[2] << "'." << endl;
    exit(EXIT_FAILURE);
  }
  c.write(output);
  fclose(output);
}
