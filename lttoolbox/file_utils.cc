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
#include <lttoolbox/compression.h>

#include <cstring>

UFILE*
openOutTextFile(const std::string& fname)
{
  if (fname.empty() || fname == "-") {
    return u_finit(stdout, NULL, NULL);
  } else {
    UFILE* ret = u_fopen(fname.c_str(), "wb", NULL, NULL);
    if (!ret) {
      std::cerr << "Error: Cannot open file '" << fname << "' for writing." << std::endl;
      exit(EXIT_FAILURE);
    }
    return ret;
  }
}

FILE*
openOutBinFile(const std::string& fname)
{
  if (fname.empty() || fname == "-") {
    return stdout;
  } else {
    FILE* ret = fopen(fname.c_str(), "wb");
    if (!ret) {
      std::cerr << "Error: Cannot open file '" << fname << "' for writing." << std::endl;
      exit(EXIT_FAILURE);
    }
    return ret;
  }
}

FILE*
openInBinFile(const std::string& fname)
{
  if (fname.empty() || fname == "-") {
    return stdin;
  } else {
    FILE* ret = fopen(fname.c_str(), "rb");
    if (!ret) {
      std::cerr << "Error: Cannot open file '" << fname << "' for reading." << std::endl;
      exit(EXIT_FAILURE);
    }
    return ret;
  }
}

void
writeTransducerSet(FILE* output, UStringView letters,
                   Alphabet& alpha,
                   std::map<UString, Transducer>& trans)
{
  fwrite_unlocked(HEADER_LTTOOLBOX, 1, 4, output);
  uint64_t features = 0;
  write_le(output, features);

  Compression::string_write(letters, output);
  alpha.write(output);
  Compression::multibyte_write(trans.size(), output);
  for (auto& it : trans) {
    Compression::string_write(it.first, output);
    it.second.write(output);
    std::cout << it.first << " " << it.second.size();
    std::cout << " " << it.second.numberOfTransitions() << std::endl;
  }
}

void
writeTransducerSet(FILE* output, const std::set<UChar32>& letters,
                   Alphabet& alpha,
                   std::map<UString, Transducer>& trans)
{
  writeTransducerSet(output, UString(letters.begin(), letters.end()), alpha, trans);
}

void
readShared(FILE* input, std::set<UChar32>& letters, Alphabet& alpha)
{
  fpos_t pos;
  if (fgetpos(input, &pos) == 0) {
    char header[4]{};
    fread_unlocked(header, 1, 4, input);
    if (strncmp(header, HEADER_LTTOOLBOX, 4) == 0) {
      auto features = read_le<uint64_t>(input);
      if (features >= LTF_UNKNOWN) {
        throw std::runtime_error("FST has features that are unknown to this version of lttoolbox - upgrade!");
      }
    } else {
      // Old binary format
      fsetpos(input, &pos);
    }
  }

  for (int len = Compression::multibyte_read(input); len > 0; len--) {
    letters.insert(static_cast<UChar32>(Compression::multibyte_read(input)));
  }

  alpha.read(input);
}

void
readTransducerSet(FILE* input, std::set<UChar32>& letters,
                  Alphabet& alpha,
                  std::map<UString, Transducer>& trans)
{
  readShared(input, letters, alpha);

  for (int len = Compression::multibyte_read(input); len > 0; len--) {
    UString name = Compression::string_read(input);
    trans[name].read(input);
  }
}

void
readTransducerSet(FILE* input, std::set<UChar32>& letters,
                  Alphabet& alpha,
                  std::map<UString, TransExe>& trans)
{
  readShared(input, letters, alpha);

  for (int len = Compression::multibyte_read(input); len > 0; len--) {
    UString name = Compression::string_read(input);
    trans[name].read(input, alpha);
  }
}
