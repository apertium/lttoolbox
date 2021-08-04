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

#include <lttoolbox/match_state2.h>

#include <climits>

MatchState2::MatchState2(TransducerExe* t) :
  trans(t)
{
  buffer[0] = trans->initial;
  last = 1;
}

MatchState2::~MatchState2()
{}

void
MatchState2::copy(const MatchState2& o)
{
  trans = o.trans;
  first = o.first;
  last = o.last;
  for (uint16_t i = first; i != last; i = (i + 1) % BUF_LIMIT) {
    buffer[i] = o.buffer[i];
  }
}

MatchState2::MatchState2(const MatchState2& o)
{
  copy(o);
}

MatchState2&
MatchState2::operator=(const MatchState2& o)
{
  if (this != &o) {
    copy(o);
  }
  return *this;
}

uint16_t
MatchState2::size() const
{
  return (last + BUF_LIMIT - first) % BUF_LIMIT;
}

bool
MatchState2::empty() const
{
  return last == first;
}

void
MatchState2::applySymbol(const uint64_t state, const int32_t symbol)
{
  uint64_t start = 0;
  uint64_t end = 0;
  trans->get_range(state, symbol, start, end);
  for (uint64_t i = start; i < end; i++) {
    buffer[last] = trans->transitions[i].dest;
    last = (last + 1) % BUF_LIMIT;
  }
}

void
MatchState2::step(const int32_t input)
{
  uint16_t temp_last = last;
  for (uint16_t i = first; i != temp_last; i = (i+1)%BUF_LIMIT) {
    applySymbol(buffer[i], input);
  }
  first = temp_last;
}

void
MatchState2::step(const int32_t input, const int32_t alt)
{
  uint16_t temp_last = last;
  for (uint16_t i = first; i != temp_last; i = (i+1)%BUF_LIMIT) {
    applySymbol(buffer[i], input);
    applySymbol(buffer[i], alt);
  }
  first = temp_last;
}

void
MatchState2::step(const int32_t input, const int32_t alt1, int32_t alt2)
{
  uint16_t temp_last = last;
  for (uint16_t i = first; i != temp_last; i = (i+1)%BUF_LIMIT) {
    applySymbol(buffer[i], input);
    applySymbol(buffer[i], alt1);
    applySymbol(buffer[i], alt2);
  }
  first = temp_last;
}

void
MatchState2::step(UString_view input, const AlphabetExe& alpha, bool foldcase)
{
  int32_t any_char = alpha("<ANY_CHAR>"_u);
  int32_t any_tag = alpha("<ANY_TAG>"_u);
  for (uint64_t i = 0; i < input.size(); i++) {
    if (input[i] == '<') {
      for (uint64_t j = i+1; j < input.size(); j++) {
        if (input[j] == '\\') {
          j++;
        } else if (input[j] == '>') {
          int32_t sym = alpha(input.substr(i, j-i+1));
          if (sym) {
            step(sym, any_tag);
          } else {
            step(any_tag);
          }
          i = j;
          break;
        }
      }
    } else {
      if (input[i] == '\\') {
        i++;
      }
      if (foldcase && u_isupper(input[i])) {
        step(input[i], u_tolower(input[i]), any_char);
      } else {
        step(input[i], any_char);
      }
    }
  }
}

int
MatchState2::classifyFinals(const std::map<uint64_t, int>& finals,
                            const std::set<int>& banned_rules) const
{
  int ret = INT_MAX;
  for (uint16_t i = first; i != last; i = (i+1)%BUF_LIMIT) {
    auto it = finals.find(buffer[i]);
    if (it != finals.end()) {
      if (it->second < ret &&
          banned_rules.find(it->second) == banned_rules.end()) {
        ret = it->second;
      }
    }
  }
  return (ret < INT_MAX) ? ret : -1;
}

int
MatchState2::classifyFinals(const std::map<uint64_t, int>& finals) const
{
  set<int> empty;
  return classifyFinals(finals, empty);
}

void
MatchState2::clear()
{
  first = 0;
  last = 1;
  buffer[0] = trans->initial;
}
