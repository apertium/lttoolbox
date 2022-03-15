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

#include <lttoolbox/file_utils.h>
#include <lttoolbox/binary_headers.h>
#include <lttoolbox/old_binary.h>
#include <lttoolbox/endian_util.h>

#include <cstring>
#include <iostream>

UFILE*
openOutTextFile(const string& fname)
{
  if (fname.empty() || fname == "-") {
    return u_finit(stdout, NULL, NULL);
  } else {
    UFILE* ret = u_fopen(fname.c_str(), "wb", NULL, NULL);
    if (!ret) {
      cerr << "Error: Cannot open file '" << fname << "' for writing." << endl;
      exit(EXIT_FAILURE);
    }
    return ret;
  }
}

FILE*
openOutBinFile(const string& fname)
{
  if (fname.empty() || fname == "-") {
    return stdout;
  } else {
    FILE* ret = fopen(fname.c_str(), "wb");
    if (!ret) {
      cerr << "Error: Cannot open file '" << fname << "' for writing." << endl;
      exit(EXIT_FAILURE);
    }
    return ret;
  }
}

FILE*
openInBinFile(const string& fname)
{
  if (fname.empty() || fname == "-") {
    return stdin;
  } else {
    FILE* ret = fopen(fname.c_str(), "rb");
    if (!ret) {
      cerr << "Error: Cannot open file '" << fname << "' for reading." << endl;
      exit(EXIT_FAILURE);
    }
    return ret;
  }
}

void
writeTransducerSet(FILE* output, const UString& letters,
                   Alphabet& alpha,
                   map<UString, Transducer>& trans)
{
  fwrite_unlocked(HEADER_LTTOOLBOX, 1, 4, output);
  uint64_t features = 0;
  features |= LTF_MMAP;
  write_le_64(output, features);

  uint64_t transducer_count = trans.size();

  StringWriter sw;
  StringRef letter_loc = sw.add(letters);
  for (auto& it : alpha.getTags()) {
    sw.add(it);
  }
  for (auto& it : trans) {
    sw.add(it.first);
  }
  sw.write(output);

  // letters
  write_le_32(output, letter_loc.start);
  write_le_32(output, letter_loc.count);

  // symbols
  alpha.write_mmap(output, sw);

  // transducers
  write_le_64(output, transducer_count);
  for (auto& it : trans) {
    cout << it.first << " " << it.second.size();
    cout << " " << it.second.numberOfTransitions() << endl;
    StringRef loc = sw.add(it.first);
    write_le_32(output, loc.start);
    write_le_32(output, loc.count);
    it.second.write_mmap(output, alpha);
  }
}

void
readTransducerSet(FILE* input, set<UChar32>& letters,
                  Alphabet& alpha,
                  map<UString, Transducer>& trans)
{
  fpos_t pos;
  bool mmap = false;
  if (fgetpos(input, &pos) == 0) {
    char header[4]{};
    auto r = fread_unlocked(header, 1, 4, input);
    if (r == 4 && strncmp(header, HEADER_LTTOOLBOX, 4) == 0) {
      auto features = read_le_64(input);
      if (features >= LTF_UNKNOWN) {
        throw std::runtime_error("FST has features that are unknown to this version of lttoolbox - upgrade!");
      }
      mmap = features & LTF_MMAP;
    }
    else {
      // Old binary format
      fsetpos(input, &pos);
    }
  }

  UString letters_str;

  if (mmap) {
    // make copies of all the strings we get from StringWriter
    // because it gets deallocated when the function returns
    StringWriter sw;
    sw.read(input);

    // letters
    uint32_t s = read_le_32(input);
    uint32_t c = read_le_32(input);
    letters_str = UString{sw.get(s, c)};

    // symbols
    alpha.read_mmap(input, sw);

    uint64_t tr_count = read_le_64(input);
    for (uint64_t i = 0; i < tr_count; i++) {
      uint32_t s = read_le_32(input);
      uint32_t c = read_le_32(input);
      UString name = UString{sw.get(s, c)};
      trans[name].read_mmap(input, alpha);
    }
  } else {
    // letters
    OldBinary::read_ustr(input, letters_str, true);

    // symbols
    alpha.read(input);

    int len = OldBinary::read_int(input, true);

    while(len > 0) {
      UString name;
      OldBinary::read_ustr(input, name, true);
      trans[name].read(input);

      len--;
    }
  }
  letters = set<int32_t>(letters_str.begin(), letters_str.end());
}
