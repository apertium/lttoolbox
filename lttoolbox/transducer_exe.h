/*
 * Copyright (C) 2021 Apertium
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

#include <cstdint>
#include <cstdio>

// only needed for reading non-mmap files
#include <lttoolbox/alphabet.h>

struct Transition {
  int32_t isym;
  int32_t osym;
  uint64_t dest;
  double weight;
};

struct Final {
  uint64_t state;
  double weight;
};

class TransducerExe {
private:
  uint64_t initial;
  uint64_t state_count;
  uint64_t final_count;
  uint64_t transition_count;
  Final* finals;
  uint64_t* offsets;
  Transition* transitions;
public:
  TransducerExe();
  ~TransducerExe();
  void read(FILE* input, Alphabet& alphabet);
};
