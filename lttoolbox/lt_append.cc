/*
 * Copyright (C) 2022 Apertium
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
#include <lttoolbox/file_utils.h>

#include <lttoolbox/my_stdio.h>
#include <lttoolbox/lt_locale.h>

#include <cstdlib>
#include <iostream>
#include <libgen.h>
#include <string>
#include <cstring>
#include <getopt.h>

void endProgram(char *name)
{
  if(name != NULL)
  {
    std::cout << basename(name) << " v" << PACKAGE_VERSION <<": add sections to a compiled transducer" << std::endl;
    std::cout << "USAGE: " << basename(name) << " [-ksh] bin_file1 bin_file2 output_file" << std::endl;
    std::cout << "    -k, --keep:     in case of section name conflicts, keep the one from the first transducer" << std::endl;
    std::cout << "    -s, --single:   treat input transducers as one-sided" << std::endl;
    std::cout << "    -h, --help:     print this message and exit" << std::endl;
  }
  exit(EXIT_FAILURE);
}


int main(int argc, char *argv[])
{
  LtLocale::tryToSetLocale();

  bool pairs = true;
  bool keep = false;

#if HAVE_GETOPT_LONG
  int option_index=0;
#endif

  while (true) {
#if HAVE_GETOPT_LONG
    static struct option long_options[] =
    {
      {"keep",      no_argument, 0, 'k'},
      {"single",    no_argument, 0, 's'},
      {"help",      no_argument, 0, 'h'},
      {0, 0, 0, 0}
    };

    int cnt=getopt_long(argc, argv, "ksh", long_options, &option_index);
#else
    int cnt=getopt(argc, argv, "ksh");
#endif
    if (cnt==-1)
      break;

    switch (cnt)
    {
      case 'k':
        keep = true;
        break;

      case 's':
        pairs = false;
        break;

      case 'h':
      default:
        endProgram(argv[0]);
        break;
    }
  }

  std::string infile1;
  std::string infile2;
  std::string outfile;
  switch(argc - optind)
  {
    case 1:
      infile1 = argv[argc-1];
      break;

    case 2:
      infile1 = argv[argc-2];
      infile2 = argv[argc-1];
      break;

    case 3:
      infile1 = argv[argc-3];
      infile2 = argv[argc-2];
      outfile = argv[argc-1];
      break;

    default:
      endProgram(argv[0]);
      break;
  }

  FILE* input1 = openInBinFile(infile1);
  FILE* input2 = openInBinFile(infile2);
  FILE* output = openOutBinFile(outfile);

  Alphabet alpha1, alpha2;
  std::set<UChar32> chars1, chars2;
  std::map<UString, Transducer> trans1, trans2;

  readTransducerSet(input1, chars1, alpha1, trans1);
  readTransducerSet(input2, chars2, alpha2, trans2);

  for (auto& it : chars2) {
    chars1.insert(it);
  }
  UString chars(chars1.begin(), chars1.end());

  for (auto& it : trans2) {
    if (trans1.find(it.first) != trans1.end()) {
      if (keep) {
        continue;
      } else {
        std::cerr << "WARNING: section '" << it.first << "' appears in both transducers and will be overwritten!" << std::endl;
      }
    }
    it.second.updateAlphabet(alpha2, alpha1, pairs);
    trans1[it.first] = it.second;
  }

  writeTransducerSet(output, chars, alpha1, trans1);

  fclose(input1);
  fclose(input2);
  fclose(output);

  return 0;
}
