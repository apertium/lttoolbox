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

#include <lttoolbox/transducer_exe.h>

#include <cstring>
#include <lttoolbox/binary_headers.h>
#include <lttoolbox/endian_util.h>

// includes needed for reading non-mmap files
#include <lttoolbox/old_binary.h>
#include <map>
#include <vector>

TransducerExe::TransducerExe() :
  initial(0), state_count(0), final_count(0), transition_count(0),
  finals(nullptr), offsets(nullptr), transitions(nullptr)
{}

TransducerExe::~TransducerExe()
{
  if (!mmapping) {
    delete[] finals;
    delete[] offsets;
    delete[] transitions;
  }
}

void
TransducerExe::read_compressed(FILE* input, Alphabet& alphabet, bool match)
{
  bool read_weights = false; // only matters for pre-mmap
  fpos_t pos;
  fgetpos(input, &pos);
  char header[4]{};
  fread_unlocked(header, 1, 4, input);
  if (strncmp(header, HEADER_TRANSDUCER, 4) == 0) {
    auto features = OldBinary::read_u64(input);
    if (features >= TDF_UNKNOWN) {
      throw std::runtime_error("Transducer has features that are unknown to this version of lttoolbox - upgrade!");
    }
    read_weights = (features & TDF_WEIGHTS);
  } else {
    // no header
    fsetpos(input, &pos);
  }

  initial = OldBinary::read_int(input, true);
  final_count = OldBinary::read_int(input, true);

  uint64_t base_state = 0;
  double base_weight = 0.0;
  finals = new Final[final_count];
  for (uint64_t i = 0; i < final_count; i++) {
    base_state += OldBinary::read_int(input, true);
    if (read_weights) {
      base_weight += OldBinary::read_double(input, true);
    }
    finals[i].state = base_state;
    finals[i].weight = base_weight;
  }

  state_count = OldBinary::read_int(input, true);
  offsets = new uint64_t[state_count+1];
  transition_count = 0;
  std::vector<int32_t> isyms, osyms;
  std::vector<uint64_t> dests;
  std::vector<double> weights;
  for (uint64_t i = 0; i < state_count; i++) {
    offsets[i] = transition_count;
    std::map<int32_t,
             std::vector<std::pair<int32_t,
                                   std::pair<uint64_t, double>>>> temp;
    uint64_t count = OldBinary::read_int(input, true);
    transition_count += count;
    int32_t tag_base = 0;
    for (uint64_t t = 0; t < count; t++) {
      tag_base += OldBinary::read_int(input, true);
      if (match) {
        tag_base -= alphabet.size();
      }
      uint64_t dest = (i + OldBinary::read_int(input, true)) % state_count;
      if (read_weights) {
        base_weight = OldBinary::read_double(input, true);
      }
      if (match) {
        temp[tag_base].push_back(make_pair(tag_base,
                                           make_pair(dest, base_weight)));
      } else {
        auto sym = alphabet.decode(tag_base);
        temp[sym.first].push_back(make_pair(sym.second,
                                            make_pair(dest, base_weight)));
      }
    }
    for (auto& it : temp) {
      for (auto& it2 : it.second) {
        isyms.push_back(it.first);
        osyms.push_back(it2.first);
        dests.push_back(it2.second.first);
        weights.push_back(it2.second.second);
      }
    }
  }
  offsets[state_count] = transition_count;
  transitions = new Transition[transition_count];
  for (uint64_t i = 0; i < transition_count; i++) {
    transitions[i].isym = isyms[i];
    transitions[i].osym = osyms[i];
    transitions[i].dest = dests[i];
    transitions[i].weight = weights[i];
  }
}

void
TransducerExe::read_serialised(FILE* input, Alphabet& alphabet, bool match)
{
  initial = OldBinary::read_int(input, false);
  final_count = OldBinary::read_int(input, false);

  finals = new Final[final_count];
  for (uint64_t i = 0; i < final_count; i++) {
    finals[i].state = OldBinary::read_int(input, false);
    finals[i].weight = OldBinary::read_double(input, false);
  }

  state_count = OldBinary::read_int(input, false);
  offsets = new uint64_t[state_count+1];
  transition_count = 0;
  std::vector<int32_t> isyms, osyms;
  std::vector<uint64_t> dests;
  std::vector<double> weights;
  for (uint64_t i = 0; i < state_count; i++) {
    offsets[i] = transition_count;
    std::map<int32_t,
             std::vector<std::pair<int32_t,
                                   std::pair<uint64_t, double>>>> temp;
    OldBinary::read_int(input, false); // src state, should == i
    uint64_t count = OldBinary::read_int(input, false);
    transition_count += count;
    for (uint64_t t = 0; t < count; t++) {
      int32_t tag = OldBinary::read_int(input, false);
      uint64_t dest = OldBinary::read_int(input, false);
      double weight = OldBinary::read_double(input, false);
      if (match) {
        temp[tag].push_back(make_pair(tag, make_pair(dest, weight)));
      } else {
        auto sym = alphabet.decode(tag);
        temp[sym.first].push_back(make_pair(sym.second,
                                            make_pair(dest, weight)));
      }
    }
    for (auto& it : temp) {
      for (auto& it2 : it.second) {
        isyms.push_back(it.first);
        osyms.push_back(it2.first);
        dests.push_back(it2.second.first);
        weights.push_back(it2.second.second);
      }
    }
  }
  offsets[state_count] = transition_count;
  transitions = new Transition[transition_count];
  for (uint64_t i = 0; i < transition_count; i++) {
    transitions[i].isym = isyms[i];
    transitions[i].osym = osyms[i];
    transitions[i].dest = dests[i];
    transitions[i].weight = weights[i];
  }
}

void
TransducerExe::read(FILE* input)
{
  fpos_t pos;
  fgetpos(input, &pos);
  char header[4]{};
  auto l = fread_unlocked(header, 1, 4, input);
  if (l == 4 && strncmp(header, HEADER_TRANSDUCER, 4) == 0) {
    auto features = read_le_64(input);
    if (features >= TDF_UNKNOWN) {
      throw std::runtime_error("Transducer has features that are unknown to this version of lttoolbox - upgrade!");
    }
  } else {
    throw std::runtime_error("Unable to read transducer header!");
  }

  read_le_64(input); // total size
  initial          = read_le_64(input);
  state_count      = read_le_64(input);
  final_count      = read_le_64(input);
  transition_count = read_le_64(input);

  finals = new Final[final_count];
  for (uint64_t i = 0; i < final_count; i++) {
    finals[i].state = read_le_64(input);
    finals[i].weight = read_le_double(input);
  }

  offsets = new uint64_t[state_count+1];
  for (uint64_t i = 0; i < state_count+1; i++) {
    offsets[i] = read_le_64(input);
  }

  transitions = new Transition[transition_count];
  for (uint64_t i = 0; i < transition_count; i++) {
    transitions[i].isym = read_le_s32(input);
    transitions[i].osym = read_le_s32(input);
    transitions[i].dest = read_le_64(input);
    transitions[i].weight = read_le_double(input);
  }
}

void*
TransducerExe::init(void* ptr)
{
  mmapping = true;

  ptr += 4 + sizeof(uint64_t); // skip header
  uint64_t* arr = reinterpret_cast<uint64_t*>(ptr);
  uint64_t total_size = arr[0];
  initial = arr[1];
  state_count = arr[2];
  final_count = arr[3];
  transition_count = arr[4];
  ptr += sizeof(uint64_t)*5;

  finals = reinterpret_cast<Final*>(ptr);
  ptr += sizeof(Final)*final_count;

  offsets = reinterpret_cast<uint64_t*>(ptr);
  ptr += sizeof(uint64_t)*(state_count+1);

  transitions = reinterpret_cast<Transition*>(ptr);
  ptr += sizeof(Transition)*transition_count;

  return ptr;
}

void
TransducerExe::get_range(const uint64_t state, const int32_t symbol,
                         uint64_t& start, uint64_t& end)
{
  uint64_t l = offsets[state];
  uint64_t r = offsets[state+1];
  uint64_t m;
  if (l == r) {
    start = end = 0;
    return;
  }
  while (l < r) {
    m = (l + r) / 2;
    if (transitions[m].isym < symbol) {
      l = m + 1;
    } else {
      r = m;
    }
  }
  if (transitions[l].isym != symbol) {
    end = start = 0;
    return;
  } else {
    start = l;
  }
  // there's probably a way to do this with 1 loop
  // but I'd have to be very sure of what I was doing to write that loop -DGS
  l = start;
  r = offsets[state+1];
  while (l < r) {
    m = (l + r) / 2;
    if (transitions[m].isym > symbol) {
      r = m;
    } else {
      l = m + 1;
    }
  }
  end = l;
}

bool
TransducerExe::find_final(const uint64_t state, double& weight)
{
  int64_t l = 0;
  int64_t r = final_count - 1;
  int64_t m;
  while (l <= r) {
    m = (l + r) / 2;
    if (finals[m].state == state) {
      weight = finals[m].weight;
      return true;
    } else if (finals[m].state < state) {
      l = m + 1;
    } else {
      r = m - 1;
    }
  }
  return false;
}

bool
TransducerExe::is_final(const uint64_t state)
{
  double x;
  return find_final(state, x);
}
