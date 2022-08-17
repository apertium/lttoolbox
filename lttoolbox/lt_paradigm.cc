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
#include <lttoolbox/alphabet.h>
#include <lttoolbox/file_utils.h>
#include <lttoolbox/input_file.h>
#include <lttoolbox/lt_locale.h>
#include <lttoolbox/state.h>
#include <lttoolbox/trans_exe.h>
#include <lttoolbox/cli.h>

#include <queue>

void expand(Transducer& inter, int state, const std::set<int>& past_states,
            const std::vector<int32_t>& syms, const Alphabet& alpha, UFILE* out,
            std::set<std::pair<UString, UString>>& outset)
{
  if (inter.isFinal(state)) {
    UString l, r;
    for (auto& it : syms) {
      auto pr = alpha.decode(it);
      alpha.getSymbol(l, pr.first);
      alpha.getSymbol(r, pr.second);
    }
    if (!l.empty() && !r.empty()) {
      if (out != nullptr) {
        u_fprintf(out, "%S:%S\n", r.c_str(), l.c_str());
      } else {
        outset.insert(std::make_pair(r, l));
      }
    }
  }
  std::set<int> new_states = past_states;
  new_states.insert(state);
  for (auto& it : inter.getTransitions()[state]) {
    if (past_states.find(it.second.first) != past_states.end()) {
      continue;
    }
    std::vector<int32_t> new_syms = syms;
    new_syms.push_back(it.first);
    expand(inter, it.second.first, new_states, new_syms, alpha, out, outset);
  }
}

void process(const UString& pattern, std::map<UString, Transducer>& trans,
             Alphabet& alpha,
             const std::set<UChar32>& letters, const std::set<int32_t>& tags,
             UFILE* output, bool sort)
{
  int32_t any_char = static_cast<int32_t>('*');
  int32_t any_tag = alpha("<*>"_u);
  std::vector<int32_t> pat = alpha.tokenize(pattern);
  Transducer other;
  int state = other.getInitial();
  for (auto& it : pat) {
    if (it == any_char) {
      state = other.insertNewSingleTransduction(0, state);
      for (auto& sym : letters) {
        other.linkStates(state, state, alpha(sym, sym));
      }
    } else if (it == any_tag) {
      state = other.insertNewSingleTransduction(0, state);
      for (auto& sym : tags) {
        other.linkStates(state, state, alpha(sym, sym));
      }
    } else {
      state = other.insertNewSingleTransduction(alpha(it, it), state);
    }
  }
  other.setFinal(state);
  std::set<std::pair<UString, UString>> outset;
  for (auto& it : trans) {
    Transducer inter = it.second.intersect(other, alpha, alpha);
    if (!inter.getFinals().empty()) {
      std::set<int> states;
      std::vector<int32_t> syms;
      expand(inter, inter.getInitial(), states, syms, alpha,
             (sort ? nullptr : output), outset);
    }
  }
  if (sort) {
    for (auto& it : outset) {
      u_fprintf(output, "%S:%S\n", it.first.c_str(), it.second.c_str());
    }
  }
}

int main(int argc, char* argv[])
{
  LtLocale::tryToSetLocale();
  CLI cli("generate listings from a compiled transducer", PACKAGE_VERSION);
  cli.add_bool_arg('a', "analyser", "FST is an analyser (tags on the right)");
  cli.add_str_arg('e', "exclude", "disregard paths containing TAG", "TAG");
  cli.add_bool_arg('s', "sort", "alphabetize the paths for each pattern");
  cli.add_bool_arg('z', "null-flush", "flush output on \\0");
  cli.add_bool_arg('h', "help", "show this help and exit");
  cli.add_file_arg("FST", false);
  cli.add_file_arg("input");
  cli.add_file_arg("output");
  cli.parse_args(argc, argv);

  bool should_invert = !cli.get_bools()["analyser"];
  bool sort = cli.get_bools()["sort"];
  std::set<UString> skip_tags;
  for (auto& it : cli.get_strs()["exclude"]) {
    skip_tags.insert(to_ustring(it.c_str()));
  }

  FILE* fst = openInBinFile(cli.get_files()[0]);

  std::set<UChar32> letters;
  Alphabet alpha;
  std::map<UString, Transducer> trans;
  readTransducerSet(fst, letters, alpha, trans);
  fclose(fst);

  alpha.includeSymbol("<*>"_u);
  std::set<int32_t> tags;
  for (int32_t i = 1; i <= alpha.size(); i++) {
    if (!skip_tags.empty()) {
      UString t;
      alpha.getSymbol(t, -i);
      if (skip_tags.find(t) != skip_tags.end()) continue;
    }
    tags.insert(-i);
  }

  if (should_invert) {
    for (auto& it : trans) {
      it.second.invert(alpha);
    }
  }

  InputFile input;
  if (!cli.get_files()[1].empty()) {
    input.open_or_exit(cli.get_files()[1].c_str());
  }
  UFILE* output = openOutTextFile(cli.get_files()[2]);

  UString cur;
  do {
    UChar32 c = input.get();
    if (c == '\n' || c == '\0' || c == U_EOF) {
      process(cur, trans, alpha, letters, tags, output, sort);
      if (c != U_EOF) {
        u_fputc(c, output);
        u_fflush(output);
      }
      cur.clear();
    } else {
      cur += c;
    }
  } while (!input.eof());

  u_fclose(output);
  return 0;
}
