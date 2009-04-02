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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */
#include <lttoolbox/tmx_compiler.h>
#include <lttoolbox/lttoolbox_config.h>
#include <lttoolbox/lt_locale.h>
#include <cstdlib>
#include <iostream>
#include <libgen.h>
#include <string>

using namespace std;

void endProgram(char *name)
{
  if(name != NULL)
  {
    cout << basename(name) << " v" << PACKAGE_VERSION <<": build a letter transducer from a TMX translation memory" << endl;
    cout << "USAGE: " << basename(name) << " lang1-lang2 tmx_file output_file" << endl;
    cout << "Modes:" << endl;
    cout << "  lang1:     input language" << endl;
    cout << "  lang2:     output language" << endl;
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

  string opc = argv[1];
  wchar_t lo[opc.size()+1];
  wchar_t lm[opc.size()+1];

  
  
  if(((size_t) -1 == mbstowcs(lo, opc.substr(0, opc.find('-')).c_str(), opc.size()))|| 
     ((size_t) -1 == mbstowcs(lm, opc.substr(opc.find('-')+1).c_str(), opc.size())))
  {
    endProgram(argv[0]);
  }

  TMXCompiler c;

  c.parse(argv[2], lo, lm);

  FILE *output = fopen(argv[3], "w");
  if(!output)
  {
    cerr << "Error: Cannot open file '" << argv[2] << "'." << endl;
    exit(EXIT_FAILURE);
  }
  c.write(output);
  fclose(output);
}
