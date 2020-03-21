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
#include <lttoolbox/transducer.h>
#include <lttoolbox/compression.h>
#include <lttoolbox/lttoolbox_config.h>

#include <lttoolbox/my_stdio.h>
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
    cout << basename(name) << " v" << PACKAGE_VERSION <<": dump a transducer to text in ATT format" << endl;
    cout << "USAGE: " << basename(name) << " [-Hh] bin_file " << endl;
    cout << "    -H, --hfst:     use HFST-compatible character escapes" << endl;
    cout << "    -h, --help:     print this message and exit" << endl;
  }
  exit(EXIT_FAILURE);
}


int main(int argc, char *argv[])
{
  bool hfst = false;
  FILE* input = NULL;
  FILE* output = stdout;

  LtLocale::tryToSetLocale();

#if HAVE_GETOPT_LONG
  int option_index=0;
#endif

  while (true) {
#if HAVE_GETOPT_LONG
    static struct option long_options[] =
    {
      {"hfst",      no_argument, 0, 'H'},
      {"help",      no_argument, 0, 'h'},
      {0, 0, 0, 0}
    };

    int cnt=getopt_long(argc, argv, "Hh", long_options, &option_index);
#else
    int cnt=getopt(argc, argv, "Hh");
#endif
    if (cnt==-1)
      break;

    switch (cnt)
    {
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

  input = fopen(infile.c_str(), "rb");
  if(!input)
  {
    cerr << "Error: Cannot open file '" << infile << "' for reading." << endl;
    exit(EXIT_FAILURE);
  }

  if(outfile != "")
  {
    output = fopen(outfile.c_str(), "w");
    if(!output)
    {
      cerr << "Error: Cannot open file '" << outfile << "' for writing." << endl;
      exit(EXIT_FAILURE);
    }
  }

  Alphabet alphabet;
  set<wchar_t> alphabetic_chars;

  map<wstring, Transducer> transducers;

  fpos_t pos;
  if (fgetpos(input, &pos) == 0) {
      char header[4]{};
      fread(header, 1, 4, input);
      if (strncmp(header, HEADER_LTTOOLBOX, 4) == 0) {
          auto features = read_le<uint64_t>(input);
          if (features >= LTF_UNKNOWN) {
              throw std::runtime_error("FST has features that are unknown to this version of lttoolbox - upgrade!");
          }
      }
      else {
          // Old binary format
          fsetpos(input, &pos);
      }
  }

  // letters
  int len = Compression::multibyte_read(input);
  while(len > 0)
  {
    alphabetic_chars.insert(static_cast<wchar_t>(Compression::multibyte_read(input)));
    len--;
  }

  // symbols
  alphabet.read(input);

  len = Compression::multibyte_read(input);

  while(len > 0)
  {
    int len2 = Compression::multibyte_read(input);
    wstring name = L"";
    while(len2 > 0)
    {
      name += static_cast<wchar_t>(Compression::multibyte_read(input));
      len2--;
    }
    transducers[name].read(input);

    len--;
  }

  /////////////////////

  map<wstring, Transducer>::iterator penum = transducers.end();
  penum--;
  for(map<wstring, Transducer>::iterator it = transducers.begin(); it != transducers.end(); it++)
  {
    it->second.joinFinals();
    it->second.show(alphabet, output, hfst);
    if(it != penum)
    {
      fwprintf(output, L"--\n", it->first.c_str());
    }
  }

  fclose(input);
  if(output != stdout)
  {
    fclose(output);
  }

  return 0;
}
