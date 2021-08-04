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

#ifndef _LT_MATCH_STATE_
#define _LT_MATCH_STATE_

#include <lttoolbox/alphabet_exe.h>
#include <lttoolbox/transducer_exe.h>
#include <lttoolbox/ustring.h>

// rename upon deleting old MatchState
class MatchState2
{
private:
  static int const BUF_LIMIT = 1024;
  TransducerExe* trans;
  uint64_t buffer[1024];
  uint16_t first = 0;
  uint16_t last = 0;

  void copy(const MatchState2& o);
  void applySymbol(const uint64_t state, const int32_t symbol);
public:
  MatchState2(TransducerExe* t);
  ~MatchState2();
  MatchState2(const MatchState2& o);
  MatchState2& operator=(const MatchState2& o);

  uint16_t size() const;
  bool empty() const;
  void step(const int32_t input);
  void step(const int32_t input, const int32_t alt);
  void step(const int32_t input, const int32_t alt1, const int32_t alt2);
  void step(UString_view input, const AlphabetExe& alpha, bool foldcase = true);
  int classifyFinals(const std::map<uint64_t, int>& finals,
                     const std::set<int>& banned_rules) const;
  int classifyFinals(const std::map<uint64_t, int>& finals) const;
  void clear();
};

#endif
