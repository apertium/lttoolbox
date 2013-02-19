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

#include <lttoolbox/expander.h>
#include <lttoolbox/lttoolbox_config.h>

#include <cstdlib>
#include <iostream>
#include <libgen.h>
#include <string>
#include <getopt.h>

#ifdef _MSC_VER
#include <io.h>
#include <fcntl.h>
#endif

using namespace std;

void endProgram(char *name)
{
  if(name != NULL)
  {
    cout << basename(name) << " v" << PACKAGE_VERSION <<": expand the contents of a dictionary file" << endl;
    cout << "USAGE: " << basename(name) << " [-avlrh] dictionary_file [output_file]" << endl;
  }
  exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
  FILE *input = NULL, *output = NULL;
  Expander e;

#if HAVE_GETOPT_LONG
  int option_index=0;
#endif

  while (true) {
#if HAVE_GETOPT_LONG
    static struct option long_options[] =
    {
      {"alt",       required_argument, 0, 'a'},
      {"var",       required_argument, 0, 'v'},
      {"var-left",  required_argument, 0, 'l'},
      {"var-right", required_argument, 0, 'r'},
      {"help",      no_argument,       0, 'h'}, 
      {0, 0, 0, 0}
    };

    int cnt=getopt_long(argc, argv, "a:v:l:r:h", long_options, &option_index);
#else
    int cnt=getopt(argc, argv, "a:v:l:r:h");
#endif
    if (cnt==-1)
      break;

    switch (cnt)
    {
      case 'a':
        e.setAltValue(optarg);
        break;

      case 'v':
        e.setVariantValue(optarg);
        break;

      case 'l':
        e.setVariantLeftValue(optarg);
        break;

      case 'r':
        e.setVariantRightValue(optarg);
        break;

      case 'h':
      default:
        endProgram(argv[0]);
        break;
    }
  }

  string infile;
  string outfile;

  switch(argc - optind + 1)
  {
    case 2:
      infile = argv[argc-1];
      input = fopen(infile.c_str(), "rb");
      if(input == NULL)
      {
        cerr << "Error: Cannot open file '" << infile << "'." << endl;
        exit(EXIT_FAILURE);
      }      
      fclose(input);
      output = stdout;
      break;
    
    case 3:
      infile = argv[argc-2];
      input = fopen(infile.c_str(), "rb");
      if(input == NULL)
      {
        cerr << "Error: Cannot open file '" << infile << "'." << endl;
        exit(EXIT_FAILURE);
      }
      fclose(input);

      outfile = argv[argc-1];
      output = fopen(argv[argc-1], "wb");
      if(output == NULL)
      {
        cerr << "Error: Cannot open file '" << outfile << "'." << endl;
        exit(EXIT_FAILURE);
      }
      break;

    default:
      endProgram(argv[0]);
      break;
  }

#ifdef _MSC_VER
  _setmode(_fileno(output), _O_U8TEXT);
#endif

  e.expand(infile, output);
  fclose(output);
  
  return EXIT_SUCCESS;
}
