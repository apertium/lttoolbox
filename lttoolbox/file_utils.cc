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
#include <lttoolbox/mmap.h>
#include <lttoolbox/old_binary.h>
#include <lttoolbox/endian_util.h>

#include <cstring>
#include <iostream>

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
writeTransducerSet(FILE* output, const UString& letters,
                   Alphabet& alpha,
                   std::map<UString, Transducer>& trans)
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
    std::cout << it.first << " " << it.second.size();
    std::cout << " " << it.second.numberOfTransitions() << std::endl;
    StringRef loc = sw.add(it.first);
    write_le_32(output, loc.start);
    write_le_32(output, loc.count);
    it.second.write_mmap(output, alpha);
  }
}

void
readTransducerSet(FILE* input, std::set<UChar32>& letters,
                  Alphabet& alpha,
                  std::map<UString, Transducer>& trans)
{
  uint64_t features;
  bool mmap = false;
  if (readHeader(input, HEADER_LTTOOLBOX, features)) {
    if (features >= LTF_UNKNOWN) {
      throw std::runtime_error("FST has features that are unknown to this version of lttoolbox - upgrade!");
    }
    mmap = features & LTF_MMAP;
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
  letters = std::set<int32_t>(letters_str.begin(), letters_str.end());
}

void
readTransducerSet(FILE* input,
                  bool& mmapping, void* mmap_ptr, int& mmap_len,
                  StringWriter& str_write,
                  std::set<UChar32>* letters, AlphabetExe& alpha,
                  std::map<UString, TransducerExe>& trans)
{
  uint64_t features;
  bool mmap = false;
  if (readHeader(input, HEADER_LTTOOLBOX, features)) {
    if (features >= LTF_UNKNOWN) {
      throw std::runtime_error("FST has features that are unknown to this version of lttoolbox - upgrade!");
    }
    mmap = features & LTF_MMAP;
  }

  if (mmap) {
    fpos_t pos;
    fgetpos(input, &pos);
    rewind(input);
    mmapping = mmap_file(input, mmap_ptr, mmap_len);
    if (mmapping) {
      void* ptr = mmap_ptr + 12;
      ptr = str_write.init(ptr);

      if (letters != nullptr) {
        StringRef let_loc = reinterpret_cast<StringRef*>(ptr)[0];
        std::vector<int32_t> vec;
        ustring_to_vec32(str_write.get(let_loc), vec);
        letters->insert(vec.begin(), vec.end());
        ptr += sizeof(StringRef);
      }

      ptr = alpha.init(ptr);

      uint64_t tr_count = reinterpret_cast<uint64_t*>(ptr)[0];
      ptr += sizeof(uint64_t);
      for (uint64_t i = 0; i < tr_count; i++) {
        StringRef tn = reinterpret_cast<StringRef*>(ptr)[0];
        ptr += sizeof(StringRef);
        UString name = UString{str_write.get(tn)};
        ptr = trans[name].init(ptr);
      }
    } else {
      fsetpos(input, &pos);

      str_write.read(input);

      if (letters != nullptr) {
        uint32_t s = read_le_32(input);
        uint32_t c = read_le_32(input);
        std::vector<int32_t> vec;
        ustring_to_vec32(str_write.get(s, c), vec);
        letters->insert(vec.begin(), vec.end());
      }

      alpha.read(input, true);

      uint64_t tr_count = read_le_64(input);
      for (uint64_t i = 0; i < tr_count; i++) {
        uint32_t s = read_le_32(input);
        uint32_t c = read_le_32(input);
        UString name = UString{str_write.get(s, c)};
        trans[name].read(input);
      }
    }
  } else {
    uint64_t len;

    if (letters != nullptr) {
      // letters
      len = OldBinary::read_int(input);
      while(len > 0) {
        letters->insert(static_cast<UChar32>(OldBinary::read_int(input)));
        len--;
      }
    }

    // symbols
    fpos_t pos;
    fgetpos(input, &pos);
    alpha.read(input, false);
    fsetpos(input, &pos);
    Alphabet temp;
    temp.read(input);

    len = OldBinary::read_int(input);

    while(len > 0) {
      UString name;
      OldBinary::read_ustr(input, name);
      trans[name].read_compressed(input, temp);
      len--;
    }
  }
}
