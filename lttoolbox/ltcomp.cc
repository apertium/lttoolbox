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
#include <lttoolbox/compiler.h>
#include <lttoolbox/lttools_config.h>

#include <cstdlib>
#include <iostream>
#include <libgen.h>
#include <string>

using namespace std;

void endProgram(char *name)
{
  if(name != NULL)
  {
    cout << basename(name) << " v" << PACKAGE_VERSION <<": build a letter transducer from a dictionary" << endl;
    cout << "USAGE: " << basename(name) << " lr | rl dictionary_file output_file" << endl;
    cout << "Modes:" << endl;
    cout << "  lr:     left-to-right compilation" << endl;
    cout << "  rl:     right-to-left compilation" << endl;
  }
  exit(EXIT_FAILURE);
}


int main(int argc, char *argv[])
{
  if(argc != 4)
  {
    endProgram(argv[0]);
  }
  string opc = argv[1];

  Compiler c;
  if(opc == "lr")
  {
    c.parse(argv[2], Compiler::COMPILER_RESTRICTION_LR_VAL);
  }
  else if(opc == "rl")
  {
    c.parse(argv[2], Compiler::COMPILER_RESTRICTION_RL_VAL);
  }
  else
  {
    endProgram(argv[0]);
  }

  FILE *output = fopen(argv[3], "w");
  if(!output)
  {
    cerr << "Error: Cannot open file '" << argv[2] << "'." << endl;
    exit(EXIT_FAILURE);
  }
  c.write(output);
  fclose(output);
}
