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
#include <lttoolbox/fst_processor.h>
#include <lttoolbox/lttoolbox_config.h>
#include <lttoolbox/my_stdio.h>
#include <lttoolbox/lt_locale.h>

#include <cstdlib>
#include <getopt.h>
#include <iostream>
#include <libgen.h>

using namespace std;

void endProgram(char *name)
{
  cout << basename(name) << ": process a stream with a letter transducer" << endl;
  cout << "USAGE: " << basename(name) << " fst_file [input_file [output_file]]" << endl;
  exit(EXIT_FAILURE);
}

void checkValidity(FSTProcessor const &fstp)
{
  if(!fstp.valid())
  {
    exit(EXIT_FAILURE);
  }
}

int main(int argc, char *argv[])
{
  FILE *input = stdin, *output = stdout;
  LtLocale::tryToSetLocale();
  FSTProcessor fstp;
  FILE *aux;
  
  switch(argc)
  {
    case 4:
      output = fopen(argv[3], "w");
      if(!output)
      {
        endProgram(argv[0]);
      }
      // follow
    case 3:
      input = fopen(argv[2], "r");
      if(!input)
      {
        endProgram(argv[0]);
      }
      // follow
    case 2:
      aux = fopen(argv[1], "r");
      if(!aux)
      {
        endProgram(argv[0]);
      }
      fstp.load(aux);
      fclose(aux);
      break;
    default:
      endProgram(argv[0]);
      break;
  }
    
  fstp.initTMAnalysis(); 
  checkValidity(fstp);
  fstp.tm_analysis(input, output);

  fclose(input);
  fclose(output); 
  return EXIT_SUCCESS;
}
