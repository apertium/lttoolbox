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

#ifndef __FILE_UTILS_H__

#include <lttoolbox/alphabet.h>
#include <lttoolbox/alphabet_exe.h>
#include <lttoolbox/string_writer.h>
#include <lttoolbox/transducer.h>
#include <lttoolbox/trans_exe.h>
#include <lttoolbox/transducer_exe.h>

#include <cstdio>

UFILE* openOutTextFile(const std::string& fname);
FILE* openOutBinFile(const std::string& fname);
FILE* openInBinFile(const std::string& fname);

void writeTransducerSet(FILE* output, const UString& letters,
                        Alphabet& alpha,
                        std::map<UString, Transducer>& trans);
void readTransducerSet(FILE* input, std::set<UChar32>& letters,
                       Alphabet& alpha,
                       std::map<UString, Transducer>& trans);
void readTransducerSet(FILE* input, std::set<UChar32>& letters,
                       Alphabet& alpha,
                       std::map<UString, TransExe>& trans);

// if letters == nullptr, then skip it (e.g. in lrx)
void readTransducerSet(FILE* input,
                       bool& mmapping, void* mmap_ptr, int& mmap_len,
                       StringWriter& str_write,
                       std::set<UChar32>* letters, AlphabetExe& alpha,
                       std::map<UString, TransducerExe>& trans);

#endif // __FILE_UTILS_H__
