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

#include <lttoolbox/Expander.H>
#include <lttoolbox/LttoolsConfig.H>

#include <cstdlib>
#include <iostream>
#include <libgen.h>
#include <string>

using namespace std;

void endProgram(char *name)
{
  if(name != NULL)
  {
    cout << basename(name) << " v" << PACKAGE_VERSION <<": expand the contents of a dictionary file" << endl;
    cout << "USAGE: " << basename(name) << " dictionary_file [output_file]" << endl;
  }
  exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
  FILE *input = NULL, *output = NULL;

  switch(argc)
  {
    case 2:
      input = fopen(argv[1], "r");
      if(input == NULL)
      {
        cerr << "Error: Cannot open file '" << argv[1] << "'." << endl;
        exit(EXIT_FAILURE);
      }      
      fclose(input);
      output = stdout;
      break;
    
    case 3:
      input = fopen(argv[1], "r");
      if(input == NULL)
      {
        cerr << "Error: Cannot open file '" << argv[1] << "'." << endl;
        exit(EXIT_FAILURE);
      }
      fclose(input);

      output = fopen(argv[2], "w");
      if(output == NULL)
      {
        cerr << "Error: Cannot open file '" << argv[2] << "'." << endl;
        exit(EXIT_FAILURE);
      }
      break;

    default:
      endProgram(argv[0]);
      break;
  }

  Expander e;
  e.expand(argv[1], output);
  fclose(output);
  
  return EXIT_SUCCESS;
}
