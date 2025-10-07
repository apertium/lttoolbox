/*
 * Copyright (C) 2013 Dávid Márk Nemeskey
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

#include <lttoolbox/att_compiler.h>
#include <lttoolbox/alphabet.h>
#include <lttoolbox/transducer.h>
#include <lttoolbox/compression.h>
#include <lttoolbox/file_utils.h>
#include <algorithm>
#include <limits>
#include <stack>
#include <unicode/uchar.h>
#include <unicode/ustring.h>
#include <utf8.h>
#include <unicode/utf16.h>

using namespace icu;

AttCompiler::AttCompiler()
{}

AttCompiler::~AttCompiler()
{}

void
AttCompiler::clear()
{
  for (auto& it : graph)
  {
    delete it.second;
  }
  graph.clear();
  alphabet = Alphabet();
}

/**
 * Converts symbols like @0@ to epsilon, @_SPACE_@ to space, etc.
 * @todo Are there other special symbols? If so, add them, and maybe use a map
 *       for conversion?
 */
void
AttCompiler::convert_hfst(UString& symbol)
{
  if (symbol == Transducer::HFST_EPSILON_SYMBOL_SHORT ||
      symbol == Transducer::HFST_EPSILON_SYMBOL_LONG ||
      (!hfstSymbols && symbol == Transducer::LTTB_EPSILON_SYMBOL)) {
    symbol.clear();
  } else if (symbol == Transducer::HFST_SPACE_SYMBOL) {
    symbol = " "_u;
  } else if (symbol == Transducer::HFST_TAB_SYMBOL) {
    symbol = "\t"_u;
  }
}

bool
AttCompiler::is_word_punct(UChar32 symbol)
{
  // this version isn't quite right, but something like it should be possible
  //return u_charType(symbol) & (U_NON_SPACING_MARK | U_ENCLOSING_MARK | U_COMBINING_SPACING_MARK);
  // https://en.wikipedia.org/wiki/Combining_character#Unicode_ranges
  if((symbol >= 0x0300 && symbol <= 0x036F) // Combining Diacritics
  || (symbol >= 0x1AB0 && symbol <= 0x1AFF) // ... Extended
  || (symbol >= 0x1DC0 && symbol <= 0x1DFF) // ... Supplement
  || (symbol >= 0x20D0 && symbol <= 0x20FF) // ... for Symbols
  || (symbol >= 0xFE20 && symbol <= 0xFE2F)) // Combining Half Marks
  {
    return true;
  }

  return false;
}

void
AttCompiler::update_alphabet(UChar32 c)
{
  if (is_word_punct(c) || !(u_ispunct(c) || u_isspace(c))) {
    letters.insert(c);
    if(u_islower(c)) {
      letters.insert(u_toupper(c));
    } else if(u_isupper(c)) {
      letters.insert(u_tolower(c));
    }
  }
}

void
AttCompiler::symbol_code(UStringView symbol, std::vector<int32_t>& split)
{
  if (symbol.empty()) {
    split.push_back(0);
  } else if (symbol.size() >= 2 && symbol[0] == '<' && symbol.back() == '>') {
    alphabet.includeSymbol(symbol);
    split.push_back(alphabet(symbol));
  } else {
    size_t i = 0;
    size_t end = symbol.size();
    UChar32 c;
    while (i < end) {
      U16_NEXT(symbol.data(), i, end, c);
      update_alphabet(c);
      split.push_back(c);
    }
  }
}

void
AttCompiler::add_transition(int from, int to,
                            UStringView upper, UStringView lower,
                            double weight)
{
  AttNode* src = get_node(from);
  std::vector<int32_t> lsplit, rsplit;
  symbol_code(upper, lsplit);
  symbol_code(lower, rsplit);
  for (size_t i = 0; i < lsplit.size() || i < rsplit.size(); i++) {
    int32_t l = (lsplit.size() > i ? lsplit[i] : 0);
    int32_t r = (rsplit.size() > i ? rsplit[i] : 0);
    bool last = (i+1 >= lsplit.size() && i+1 >= rsplit.size());
    int dest = (last ? to : -(++phantom_count));
    UString ls, rs;
    alphabet.getSymbol(ls, l);
    alphabet.getSymbol(rs, r);
    src->transductions.push_back(Transduction(dest, ls, rs, alphabet(l, r),
                                              (last ? weight : default_weight)));
    classify_single_transition(src->transductions.back());
    src = get_node(dest);
  }
}

/*
 * ICU number parsing does a lot of locale handling and this tends to
 * dominate the runtime of lt-comp. Since we always force the locale
 * to be C.UTF-8, we can make stronger assumptions leading to a
 * ~2000x speedup for stoi and ~500x for stod relative to StringUtils.
 * - DGS 2025-08-29
 */

int fast_stoi(UString s) {
  int ret = 0;
  for (size_t i = 0; i < s.size(); i++) {
    if (s[i] < '0' || s[i] > '9') throw std::invalid_argument("bad int");
    ret *= 10;
    ret += (s[i] - '0');
  }
  return ret;
}

double fast_stod(UString s) {
  if (s.size() == 0) throw std::invalid_argument("empty string");
  double sign = 1;
  size_t i = 0;
  if (s[i] == '-') {
    i++;
    sign = -1;
  }
  if (i == s.size()) throw std::invalid_argument("no number");
  if (i + 3 == s.size() && s[i] == 'i' && s[i+1] == 'n' && s[i+2] == 'f') {
    return sign * std::numeric_limits<double>::infinity();
  }
  double ret = 0;
  for (; i < s.size(); i++) {
    if (s[i] == '.') break;
    if (s[i] < '0' || s[i] > '9') throw std::invalid_argument("bad digit");
    ret *= 10;
    ret += (s[i] - '0');
  }
  i++;
  double mul = 0.1;
  for (; i < s.size(); i++) {
    if (s[i] < '0' || s[i] > '9') throw std::invalid_argument("bad digit");
    ret += (s[i] - '0') * mul;
    mul *= 0.1;
  }
  return sign * ret;
}

void
AttCompiler::parse(std::string const &file_name, bool read_rl)
{
  clear();

  UFILE* infile = u_fopen(file_name.c_str(), "r", NULL, NULL);
  if (infile == NULL) {
    std::cerr << "Error: unable to open '" << file_name << "' for reading." << std::endl;
  }
  std::vector<UString> tokens;
  bool first_line_in_fst = true;       // First line -- see below
  bool multiple_transducers = false;
  int state_id_offset = 1;
  int largest_seen_state_id = 0;
  int line_number = 0;

  while (!u_feof(infile))
  {
    line_number++;
    tokens.clear();
    tokens.push_back(""_u);
    do {
      UChar c = u_fgetc(infile);
      if (c == '\n' || c == U_EOF) {
        break;
      } else if (c == '\t') {
        tokens.push_back(""_u);
      } else {
        tokens.back() += c;
      }
    } while (!u_feof(infile));

    int from, to;
    UString upper, lower;
    double weight;

    /* Empty line. */
    if (tokens.size() == 1 && tokens[0].length() == 0)
    {
      continue;
    }

    if (first_line_in_fst && tokens.size() == 1)
    {
      std::cerr << "Error: invalid format in file '" << file_name << "' on line " << line_number << "." << std::endl;
      exit(EXIT_FAILURE);
    }

    if (tokens[0].find('-') == 0)
    {
      if (state_id_offset == 1) {
        // this is the first split we've seen
        std::cerr << "Warning: Multiple fsts in '" << file_name << "' will be disjuncted." << std::endl;
        multiple_transducers = true;
      }
      // Update the offset for the new FST
      state_id_offset = largest_seen_state_id + 1;
      first_line_in_fst = true;
      continue;
    }

    if (tokens.size() == 3 || tokens.size() > 5) {
      std::cerr << "Error: wrong number of columns in file '" << file_name << "' on line " << line_number << "." << std::endl;
      exit(EXIT_FAILURE);
    }

    try {
      from = fast_stoi(tokens[0]) + state_id_offset;
    } catch (const std::invalid_argument& e) {
      std::cerr << "Error: invalid source state in file '" << file_name << "' on line " << line_number << "." << std::endl;
      exit(EXIT_FAILURE);
    }
    largest_seen_state_id = std::max(largest_seen_state_id, from);

    get_node(from);
    /* First line: the initial state is of both types. */
    if (first_line_in_fst)
    {
      AttNode * starting_node = get_node(starting_state);

      // Add an Epsilon transition from the new starting state
      starting_node->transductions.push_back(
                     Transduction(from, ""_u, ""_u, 0, default_weight));
      first_line_in_fst = false;
    }

    /* Final state. */
    if (tokens.size() <= 2)
    {
      if (tokens.size() > 1)
      {
	try {
    weight = fast_stod(tokens[1]);
	} catch (const std::invalid_argument& e) {
	  std::cerr << "Error: invalid weight in file '" << file_name << "' on line " << line_number << "." << std::endl;
	  exit(EXIT_FAILURE);
	}
      }
      else
      {
        weight = default_weight;
      }
      finals.insert(std::pair <int, double>(from, weight));
    }
    else
    {
      try {
        to = fast_stoi(tokens[1]) + state_id_offset;
      } catch (const std::invalid_argument& e) {
	std::cerr << "Error: invalid target state in file '" << file_name << "' on line " << line_number << "." << std::endl;
	exit(EXIT_FAILURE);
      }
      largest_seen_state_id = std::max(largest_seen_state_id, to);
      if(read_rl)
      {
        upper = tokens[3];
        lower = tokens[2];
      }
      else
      {
        upper = tokens[2];
        lower = tokens[3];
      }
      convert_hfst(upper);
      convert_hfst(lower);
      if(tokens.size() > 4)
      {
	try {
    weight = fast_stod(tokens[4]);
	} catch (const std::invalid_argument& e) {
	  std::cerr << "Error: invalid weight in file '" << file_name << "' on line " << line_number << "." << std::endl;
	  exit(EXIT_FAILURE);
	}
      }
      else
      {
        weight = default_weight;
      }
      add_transition(from, to, upper, lower, weight);
    }
  }

  if (!multiple_transducers) {
    starting_state = 1;
    // if we aren't disjuncting multiple transducers
    // then we have an extra epsilon transduction at the beginning
    // so skip it
  }

  /* Classify the nodes of the graph. */
  if (splitting) {
    classify_forwards();
    std::set<int> path;
    classify_backwards(starting_state, path);
  }

  u_fclose(infile);
}

/** Extracts the sub-transducer made of states of type @p type. */
Transducer
AttCompiler::extract_transducer(TransducerType type)
{
  Transducer transducer;
  /* Correlation between the graph's state ids and those in the transducer. */
  std::map<int, int> corr;
  std::set<int> visited;

  corr[starting_state] = transducer.getInitial();
  _extract_transducer(type, starting_state, transducer, corr, visited);

  /* The final states. */
  //bool noFinals = true;
  for (auto& f : finals)
  {
    if (corr.find(f.first) != corr.end())
    {
      transducer.setFinal(corr[f.first], f.second);
      //noFinals = false;
    }
  }

/*
  if(noFinals)
  {
    std::cerr << "No final states (" << type << ")" << std::endl;
    std::cerr << "  were:" << std::endl;
    std::cerr << "\t" ;
    for (auto& f : finals)
    {
      std::cerr << f.first << " ";
    }
    std::cerr << std::endl;
  }
*/
  return transducer;
}

/**
 * Recursively fills @p transducer (and @p corr) -- helper method called by
 * extract_transducer().
 */
void
AttCompiler::_extract_transducer(TransducerType type, int from,
                                 Transducer& transducer, std::map<int, int>& corr,
                                 std::set<int>& visited)
{
  if (visited.find(from) != visited.end())
  {
    return;
  }
  else
  {
    visited.insert(from);
  }

  AttNode* source = get_node(from);

  /* Is the source state new? */
  bool new_from = corr.find(from) == corr.end();
  int from_t, to_t;

  for (auto& it : source->transductions)
  {
    if ((it.type & type) != type)
    {
      continue;  // Not the right type
    }
    /* Is the target state new? */
    bool new_to = corr.find(it.to) == corr.end();

    if (new_from)
    {
      corr[from] = transducer.size() + (new_to ? 1 : 0);
    }
    from_t = corr[from];

    /* Now with the target state: */
    if (!new_to)
    {
      /* We already know it, possibly by a different name: link them! */
      to_t = corr[it.to];
      transducer.linkStates(from_t, to_t, it.tag, it.weight);
    }
    else
    {
      /* We haven't seen it yet: add a new state! */
      to_t = transducer.insertNewSingleTransduction(it.tag, from_t, it.weight);
      corr[it.to] = to_t;
    }
    _extract_transducer(type, it.to, transducer, corr, visited);
  }  // for
}

void
AttCompiler::classify_single_transition(Transduction& t)
{
  int32_t sym = alphabet.decode(t.tag).first;
  if (sym > 0) {
    if (letters.find(sym) != letters.end()) {
      t.type |= WORD;
    }
    if (u_ispunct(sym)) {
      t.type |= PUNCT;
    }
  }
}

/**
 * Propagate edge types forwards.
 */
void
AttCompiler::classify_forwards()
{
  std::stack<int> todo;
  std::set<int> done;
  todo.push(starting_state);
  while(!todo.empty()) {
    int next = todo.top();
    todo.pop();
    if(done.find(next) != done.end()) continue;
    AttNode* n1 = get_node(next);
    for(auto& t1 : n1->transductions) {
      AttNode* n2 = get_node(t1.to);
      for(auto& t2 : n2->transductions) {
        t2.type |= t1.type;
      }
      if(done.find(t1.to) == done.end()) {
        todo.push(t1.to);
      }
    }
    done.insert(next);
  }
}

/**
 * Recursively determine edge types of initial epsilon transitions
 * Also check for epsilon loops or epsilon transitions to final states
 * @param state the state to examine
 * @param path the path we took to get here
 */
TransducerType
AttCompiler::classify_backwards(int state, std::set<int>& path)
{
  if(finals.find(state) != finals.end()) {
    std::cerr << "ERROR: Transducer contains epsilon transition to a final state. Aborting." << std::endl;
    exit(EXIT_FAILURE);
  }
  AttNode* node = get_node(state);
  TransducerType type = UNDECIDED;
  for(auto& t1 : node->transductions) {
    if(t1.type != UNDECIDED) {
      type |= t1.type;
    } else if(path.find(t1.to) != path.end()) {
      std::cerr << "ERROR: Transducer contains initial epsilon loop. Aborting." << std::endl;
      exit(EXIT_FAILURE);
    } else {
      path.insert(t1.to);
      t1.type = classify_backwards(t1.to, path);
      type |= t1.type;
      path.erase(t1.to);
    }
  }
  // Note: if type is still UNDECIDED at this point, then we have a dead-end
  // path, which is fine since it will be discarded by _extract_transducer()
  return type;
}


/** Writes the transducer to @p file_name in lt binary format. */
void
AttCompiler::write(FILE *output)
{
  std::map<UString, Transducer> temp;
  if (splitting) {
    temp["main@standard"_u] = extract_transducer(WORD);
    Transducer punct_fst = extract_transducer(PUNCT);
    if (punct_fst.numberOfTransitions() > 0) {
      temp["final@inconditional"_u] = punct_fst;
    }
  } else {
    temp["main@standard"_u] = extract_transducer(UNDECIDED);
  }
  writeTransducerSet(output, UString(letters.begin(), letters.end()),
                     alphabet, temp);
}

void
AttCompiler::setHfstSymbols(bool b)
{
  hfstSymbols = b;
}

void
AttCompiler::setSplitting(bool b)
{
  splitting = b;
}
