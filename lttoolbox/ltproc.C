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
#include <lttoolbox/FSTProcessor.H>
#include <lttoolbox/LttoolsConfig.H>
#include <lttoolbox/MyStdio.H>
#include <lttoolbox/LtLocale.H>

#include <cstdlib>
#include <getopt.h>
#include <iostream>
#include <libgen.h>

using namespace std;

void endProgram(char *name)
{
  cout << basename(name) << ": process a stream with a letter transducer" << endl;
  cout << "USAGE: " << basename(name) << " [-a|-g|-n|-d|-p|-s] fst_file [input_file [output_file]]" << endl;
  cout << "Options:" << endl;
#if HAVE_GETOPT_LONG
  cout << "  -a, --analysis:         morphological analysis (default behavior)" << endl;
  cout << "  -g, --generation:       morphological generation" << endl;
  cout << "  -n, --non-marked-gen    morph. generation without unknown word marks" << endl;
  cout << "  -d, --debugged-gen      morph. generation with all the stuff" <<endl;
  cout << "  -p, --post-generation:  post-generation" << endl;
  cout << "  -s, --sao:              SAO annotation system input processing" << endl;
  cout << "  -v, --version:          version" << endl;
  cout << "  -h, --help:             show this help" << endl;
#else
  cout << "  -a:   morphological analysis (default behavior)" << endl;
  cout << "  -g:   morphological generation" << endl;
  cout << "  -n:   morph. generation without unknown word marks" << endl;
  cout << "  -p:   post-generation" << endl;
  cout << "  -s:   SAO annotation system input processing" << endl;
  cout << "  -v:   version" << endl;
  cout << "  -h:   show this help" << endl;
#endif
  exit(EXIT_FAILURE);
}

void checkValidity(FSTProcessor const &fstp)
{
  if(!fstp.valid())
  {
    cerr << "ERROR: Invalid dictionary" << endl;
    exit(EXIT_FAILURE);
  }
}

int main(int argc, char *argv[])
{
  int cmd = 0;

  static struct option long_options[]=
    {
      {"analysis",        0, 0, 'a'},
      {"generation",      0, 0, 'g'},
      {"non-marked-gen",  0, 0, 'n'},
      {"debugged-gen",    0, 0, 'd'},
      {"post-generation", 0, 0, 'p'},
      {"sao",             0, 0, 's'},
      {"version",	  0, 0, 'v'},
      {"help",            0, 0, 'h'}
    };

  while(true)
  {
#if HAVE_GETOPT_LONG
    int option_index;
    int c = getopt_long(argc, argv, "agndpsvh", long_options, &option_index);
#else
    int c = getopt(argc, argv, "agndpsvh");
#endif    

    if(c == -1)
    {
      break;
    }
      
    switch(c)
    {
    case 'a':
    case 'g':
    case 'n':
    case 'd':
    case 'p':
    case 's':
      if(cmd == 0)
      {
	cmd = c;
      }
      else
      {
	endProgram(argv[0]);
      }
      break;
    case 'v':
      cout << basename(argv[0]) << " version " << PACKAGE_VERSION << endl;
      exit(EXIT_SUCCESS);
      break;
    case 'h':
    default:
      endProgram(argv[0]);
      break;
    }
  }

  FILE *input = stdin, *output = stdout;
  LtLocale::tryToSetLocale();
  FSTProcessor fstp;
  
  if(optind == (argc - 3))
  {
    FILE *in = fopen(argv[optind], "r");
    if(in == NULL || ferror(in))
    {
      endProgram(argv[0]);
    }
    
    input = fopen(argv[optind+1], "r");
    if(input == NULL || ferror(input))
    {
      endProgram(argv[0]);
    }
    
    output= fopen(argv[optind+2], "w");
    if(output == NULL || ferror(output))
    {
      endProgram(argv[0]);
    }
    
    fstp.load(in);
    fclose(in);
  }
  else if(optind == (argc -2))
  {
    FILE *in = fopen(argv[optind], "r");
    if(in == NULL || ferror(in))
    {
      endProgram(argv[0]);
    }
    
    input = fopen(argv[optind+1], "r");
    if(input == NULL || ferror(input))
    {
      endProgram(argv[0]);
    }
    
    fstp.load(in);
    fclose(in);
  }   
  else if(optind == (argc - 1))
  {
    FILE *in = fopen(argv[optind], "r");
    if(in == NULL || ferror(in))
    {
      endProgram(argv[0]);
    }
    fstp.load(in);    
    fclose(in);
  }
  else
  {
    endProgram(argv[0]);
  }

  switch(cmd)
  {
    case 'n':
      fstp.initGeneration();
      checkValidity(fstp);
      fstp.generation(stdin, stdout, gm_clean);
      break;

    case 'g':
      fstp.initGeneration();
      checkValidity(fstp);
      fstp.generation(stdin, stdout); 
      break;
      
    case 'd':
      fstp.initGeneration();
      checkValidity(fstp);
      fstp.generation(stdin, stdout, gm_all);
      
    case 'p':
      fstp.initPostgeneration();
      checkValidity(fstp);
      fstp.postgeneration(stdin, stdout);
      break;

    case 's':
      fstp.initAnalysis();
      checkValidity(fstp);
      fstp.SAO(stdin, stdout);
      break;
      
    case 'a':
    default:
      fstp.initAnalysis(); 
      checkValidity(fstp);
      fstp.analysis(stdin, stdout);
      break;
  }      

  fclose(input);
  fclose(output); 
  return EXIT_SUCCESS;
}
