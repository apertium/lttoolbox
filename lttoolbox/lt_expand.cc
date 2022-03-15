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

#include <lttoolbox/expander.h>
#include <lttoolbox/lt_locale.h>
#include <lttoolbox/file_utils.h>

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
    cout << "USAGE: " << basename(name) << " [-mavlrh] dictionary_file [output_file]" << endl;
#if HAVE_GETOPT_LONG
    cout << "  -m, --keep-boundaries:     keep morpheme boundaries" << endl;
    cout << "  -v, --var:                 set language variant" << endl;
    cout << "  -a, --alt:                 set alternative (monodix)" << endl;
    cout << "  -l, --var-left:            set left language variant (bidix)" << endl;
    cout << "  -r, --var-right:           set right language variant (bidix)" << endl;
#else
    cout << "  -m:     keep morpheme boundaries" << endl;
    cout << "  -v:     set language variant" << endl;
    cout << "  -a:     set alternative (monodix)" << endl;
    cout << "  -l:     set left language variant (bidix)" << endl;
    cout << "  -r:     set right language variant (bidix)" << endl;
#endif
  }
  exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
  LtLocale::tryToSetLocale();

  FILE* input = NULL;
  UFILE* output = NULL;
  Expander e;
  e.setKeepBoundaries(false);

#if HAVE_GETOPT_LONG
  int option_index=0;
#endif

  while (true) {
#if HAVE_GETOPT_LONG
    static struct option long_options[] =
    {
      {"keep-boundaries", no_argument, 0, 'm'},
      {"alt",       required_argument, 0, 'a'},
      {"var",       required_argument, 0, 'v'},
      {"var-left",  required_argument, 0, 'l'},
      {"var-right", required_argument, 0, 'r'},
      {"help",      no_argument,       0, 'h'},
      {0, 0, 0, 0}
    };

    int cnt=getopt_long(argc, argv, "a:v:l:r:mh", long_options, &option_index);
#else
    int cnt=getopt(argc, argv, "a:v:l:r:mh");
#endif
    if (cnt==-1)
      break;

    switch (cnt)
    {
      case 'a':
        e.setAltValue(to_ustring(optarg));
        break;

      case 'v':
        e.setVariantValue(to_ustring(optarg));
        break;

      case 'l':
        e.setVariantLeftValue(to_ustring(optarg));
        break;

      case 'm':
        e.setKeepBoundaries(true);
        break;

      case 'r':
        e.setVariantRightValue(to_ustring(optarg));
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
      break;

    case 3:
      infile = argv[argc-2];
      outfile = argv[argc-1];
      break;

    default:
      endProgram(argv[0]);
      break;
  }

#ifdef _MSC_VER
  _setmode(_fileno(output), _O_U8TEXT);
#endif

  input = openInBinFile(infile);
  fclose(input);
  output = openOutTextFile(outfile);

  e.expand(infile, output);
  u_fclose(output);

  return EXIT_SUCCESS;
}
