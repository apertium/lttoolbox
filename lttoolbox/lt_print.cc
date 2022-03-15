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
#include <lttoolbox/transducer.h>
#include <lttoolbox/compression.h>
#include <lttoolbox/endian_util.h>
#include <lttoolbox/string_writer.h>
#include <lttoolbox/file_utils.h>

#include <lttoolbox/my_stdio.h>
#include <lttoolbox/lt_locale.h>

#include <cstdlib>
#include <iostream>
#include <libgen.h>
#include <string>
#include <cstring>
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
    cout << basename(name) << " v" << PACKAGE_VERSION <<": dump a transducer to text in ATT format" << endl;
    cout << "USAGE: " << basename(name) << " [-aHh] bin_file [output_file] " << endl;
    cout << "    -a, --alpha:    print transducer alphabet" << endl;
    cout << "    -H, --hfst:     use HFST-compatible character escapes" << endl;
    cout << "    -h, --help:     print this message and exit" << endl;
  }
  exit(EXIT_FAILURE);
}


int main(int argc, char *argv[])
{
  LtLocale::tryToSetLocale();

  bool alpha = false;
  bool hfst = false;

#ifdef _MSC_VER
  _setmode(_fileno(output), _O_U8TEXT);
#endif

#if HAVE_GETOPT_LONG
  int option_index=0;
#endif

  while (true) {
#if HAVE_GETOPT_LONG
    static struct option long_options[] =
    {
      {"alpha",     no_argument, 0, 'a'},
      {"hfst",      no_argument, 0, 'H'},
      {"help",      no_argument, 0, 'h'},
      {0, 0, 0, 0}
    };

    int cnt=getopt_long(argc, argv, "aHh", long_options, &option_index);
#else
    int cnt=getopt(argc, argv, "aHh");
#endif
    if (cnt==-1)
      break;

    switch (cnt)
    {
      case 'a':
        alpha = true;
        break;

      case 'H':
        hfst = true;
        break;

      case 'h':
      default:
        endProgram(argv[0]);
        break;
    }
  }

  string infile;
  string outfile;
  switch(argc - optind)
  {
    case 1:
      infile = argv[argc-1];
      break;

    case 2:
      infile = argv[argc-2];
      outfile = argv[argc-1];
      break;

    default:
      endProgram(argv[0]);
      break;
  }

  FILE* input = openInBinFile(infile);
  UFILE* output = openOutTextFile(outfile);

  Alphabet alphabet;
  set<UChar32> alphabetic_chars;
  map<UString, Transducer> transducers;

  readTransducerSet(input, alphabetic_chars, alphabet, transducers);

  /////////////////////

  if (alpha) {
    for (auto& it : alphabetic_chars) {
      u_fprintf(output, "%C\n", it);
    }
    for (int i = 1; i <= alphabet.size(); i++) {
      alphabet.writeSymbol(-i, output);
      u_fprintf(output, "\n");
    }
  } else {
    bool first = true;
    for (auto& it : transducers) {
      if (!first) {
        u_fprintf(output, "--\n");
        first = false;
      }
      it.second.joinFinals();
      it.second.show(alphabet, output, 0, hfst);
    }
  }

  fclose(input);
  u_fclose(output);

  return 0;
}
