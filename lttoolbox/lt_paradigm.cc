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

#include <getopt.h>
#include <iostream>
#include <libgen.h>
#include <queue>

void endProgram(char* name)
{
  std::cout << basename(name) << ": generate listings from a compiled transducer" << std::endl;
  std::cout << "Usage: " << basename(name) << " [ -a ] FST [ input [ output ] ]" << std::endl;
  std::cout << "  -a, --analyser:       FST is an analyser (tags on the right)" << std::endl;
  std::cout << "  -h, --help:           Print this help and exit" << std::endl;
  exit(EXIT_FAILURE);
}

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

  bool should_invert = true;
  bool sort = false;
  std::set<UString> skip_tags;

#if HAVE_GETOPT_LONG
  static struct option long_options[] =
    {
     {"analyser",     0, 0, 'a'},
     {"exclude",      1, 0, 'e'},
     {"sort",         0, 0, 's'},
     {"null-flush",   0, 0, 'z'},
     {"help",         0, 0, 'h'},
     {0,0,0,0}
    };
#endif

  while (true) {
#if HAVE_GETOPT_LONG
    int c = getopt_long(argc, argv, "ae:szh", long_options, &optind);
#else
    int c = getopt(argc, argv, "ae:szh");
#endif
    if (c == -1) break;

    switch (c) {
    case 'a':
      should_invert = false;
      break;

    case 'e':
      skip_tags.insert(to_ustring(optarg));
      break;

    case 's':
      sort = true;
      break;

    case 'z': // no-op
      break;

    case 'h':
    default:
      endProgram(argv[0]);
      break;
    }
  }

  if (optind == argc) {
    std::cerr << "Transducer file is required." << std::endl;
    exit(EXIT_FAILURE);
  }
  FILE* fst = openInBinFile(argv[optind++]);
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
  UFILE* output = u_finit(stdout, NULL, NULL);
  if (optind < argc) {
    input.open_or_exit(argv[optind++]);
  }
  if (optind < argc) {
    output = openOutTextFile(argv[optind++]);
  }

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
