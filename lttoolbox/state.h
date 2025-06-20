/*
 * Copyright (C) 2005 Universitat d'Alacant / Universidad de Alicante
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
#ifndef _STATE_
#define _STATE_

#include <map>
#include <set>
#include <string>
#include <vector>
#include <queue>
#include <climits>

#include <lttoolbox/alphabet.h>
#include <lttoolbox/node.h>
#include <lttoolbox/match_exe.h>
#include <lttoolbox/match_state.h>
#include <lttoolbox/transducer.h>

#include <lttoolbox/ustring.h>


/**
 * Class to represent the current state of transducer processing
 */
class State
{
private:
  /**
   * The current state of transducer processing
   */
  struct TNodeState
  {
    Node *where;
    std::vector<std::pair<int, double>> *sequence;
    // a state is "dirty" if it was introduced at runtime (case variants, etc.)
    bool dirty;

    TNodeState(Node * const &w, std::vector<std::pair<int, double>> * const &s, bool const &d): where(w), sequence(s), dirty(d){}

    TNodeState(const TNodeState& other)
      : where(other.where)
      , sequence(other.sequence)
      , dirty(other.dirty)
    {}

    TNodeState & operator=(TNodeState const &other)
    {
      where = other.where;
      sequence = other.sequence;
      dirty = other.dirty;
      return *this;
    }
  };

  std::vector<TNodeState> state;

  std::vector<std::vector<std::pair<int, double>>*> sequence_pool;
  // get or create a sequence vector (may be non-empty)
  std::vector<std::pair<int, double>>* new_sequence();
  // return a sequence vector to the pool
  void free_sequence(std::vector<std::pair<int, double>>*);

  /**
   * Destroy function
   */
  void destroy();

  /**
   * Helper functions for the various apply()s to reduce code duplication
   * @return whether any transitions were made
   */
  bool apply_into(std::vector<TNodeState>* new_state, int const input, int index, bool dirty);

  bool apply_into_override(std::vector<TNodeState>* new_state, int const input, int const old_sym, int const new_sym, int index, bool dirty);

  /**
   * Make a transition, version for lowercase letters and symbols
   * @param input the input symbol
   */
  void apply(int const input);

  /**
   * Make a transition, version for lowercase and uppercase letters
   * @param input the input symbol
   * @param alt the alternative input symbol
   */
  void apply(int const input, int const alt);

  void apply(int const input, int const alt1, int const alt2);

  /**
   * Make a transition, with multiple possibilities
   * @param input the input symbol
   * @param alts set of alternative input symbols
   */
  void apply(int const input, std::set<int> const alts);

  /**
   * Make a transition, only applying lowercase version if
   * uppercase version is absent
   * @param input the input symbol
   * @param alt the alternative input symbol
   */
  void apply_careful(int const input, int const alt);

  /**
   * Make a transition, but overriding the output symbol
   * @param input symbol read from infile
   * @param output symbol from the FST
   * @param output symbol we want to appear in outfile
   */
  void apply_override(int const input, int const old_sym, int const new_sym);

  void apply_override(int const input, int const alt, int const old_sym, int const new_sym);

  /**
   * Calculate the epsilon closure over the current state, replacing
   * its content.
   */
  void epsilonClosure();

  bool lastPartHasRequiredSymbol(const std::vector<std::pair<int, double>> &seq, int requiredSymbol, int separationSymbol);

public:

  /**
   * Copy function
   * @param s the state to be copied
   */
  void copy(State const &s);


  /**
   * Constructor
   */
  State();

  /**
   * Destructor
   */
  ~State();

  /**
   * Copy constructor
   * @param s the state to be copied
   */
  State(State const &s);

  /**
   * Assignment operator
   * @param s the state to be assigned
   * @return the object that results from the assignation
   */
  State & operator =(State const &s);

  /**
   * Number of alive transductions
   * @return the size
   */
  size_t size() const;

  /**
   * step = apply + epsilonClosure
   * @param input the input symbol
   */
  void step(int const input);

  /**
   * step = apply + epsilonClosure
   * @param input the input symbol
   * @param alt the alternative input symbol
   */
  void step(int const input, int const alt);

  void step(int const input, int const alt1, int const alt2);

  /**
   * step = apply + epsilonClosure
   * @param input the input symbol
   * @param alt the alternative input symbols
   */
  void step(int const input, std::set<int> const alts);

  void step_case(UChar32 val, bool caseSensitive);

  void step_case(UChar32 val, UChar32 val2, bool caseSensitive);

  void step_careful(int const input, int const alt);

  void step_override(int const input, int const old_sym, int const new_sym);

  void step_override(int const input, int const alt, int const old_sym, int const new_sym);

  void step_case_override(const int val, const bool caseSensitive);

  void step_optional(UChar32 val);

  void closure(const sorted_vector<int32_t>& symbols);

  /**
   * Init the state with the initial node and empty output
   * @param initial the initial node of the transducer
   */
  void init(Node *initial);

  /**
    * Remove states not containing a specific symbol in their last 'part', and states
    * with more than a number of 'parts'
    * @param requieredSymbol the symbol requiered in the last part
    * @param separationSymbol the symbol that represent the separation between two parts
    * @param compound_max_elements the maximum part number allowed
    */
  void pruneCompounds(int requiredSymbol, int separationSymbol, int compound_max_elements);

  /**
    * Remove states containing a forbidden symbol
    * @param forbiddenSymbol the symbol forbidden
    */
  void pruneStatesWithForbiddenSymbol(int forbiddenSymbol);

  /**
   * Remove states not containing a particular symbol
   * @param symbol the symbol that is required
   */
  void requireSymbol(int32_t symbol);

  /**
    * Whether any of the analyses contains a certain symbol
    * @param requiredSymbol the symbol we're looking for
    */
  bool hasSymbol(int requiredSymbol);

  /**
   * Returns the lexical form in a sorted by weights manner
   * and restricts to N analyses or N weight classes if those options are provided.
   * @param the original lexical form map
   * @param the max number of printable analyses
   * @param the max number of printable weight classes
   * @return the sorted lexical form
   */

  template <typename T1, typename T2>
  struct sort_weights {
      typedef std::pair<T1, T2> type;
      bool operator ()(type const& a, type const& b) const {
          return a.second < b.second;
      }
  };

  std::vector<std::pair< UString, double >> NFinals(std::vector<std::pair<UString, double>> lf,
                                          int maxAnalyses,
                                          int maxWeightClasses) const;

  /**
   * Print all outputs of current parsing, preceded by a bar '/',
   * from the final nodes of the state
   * @param finals the set of final nodes
   * @param a the alphabet to decode strings
   * @param escaped_chars the set of chars to be preceded with one
   *                      backslash
   * @param uppercase true if the word is uppercase
   * @param firstupper true if the first letter of a word is uppercase
   * @param firstchar first character of the word
   * @return the result of the transduction
   */
  UString filterFinals(std::map<Node *, double> const &finals,
                       Alphabet const &a,
                       std::set<UChar32> const &escaped_chars,
                       bool display_weights = false,
                       int max_analyses = INT_MAX,
                       int max_weight_classes = INT_MAX,
                       bool uppercase = false,
                       bool firstupper = false,
                       int firstchar = 0) const;

  /**
   * filterFinals(), but write the results into `result`
   */
  void filterFinalsArray(std::vector<UString>& result,
                         std::map<Node *, double> const &finals,
                         Alphabet const &a,
                         std::set<UChar32> const &escaped_chars,
                         bool display_weights = false,
                         int max_analyses = INT_MAX,
                         int max_weight_classes = INT_MAX,
                         bool uppercase = false,
                         bool firstupper = false,
                         int firstchar = 0) const;

  /**
   * Same as previous one, but  the output is adapted to the SAO system
   * @param finals the set of final nodes
   * @param a the alphabet to decode strings
   * @param escaped_chars the set of chars to be preceded with one
   *                      backslash
   * @param uppercase true if the word is uppercase
   * @param firstupper true if the first letter of a word is uppercase
   * @param firstchar first character of the word
   * @return the result of the transduction
   */
  UString filterFinalsSAO(std::map<Node *, double> const &finals,
                          Alphabet const &a,
                          std::set<UChar32> const &escaped_chars,
                          bool uppercase = false,
                          bool firstupper = false,
                          int firstchar = 0) const;


  /**
   * Same as previous one, but  the output is adapted to the LRX system
   * @param finals the set of final nodes
   * @param a the alphabet to decode strings
   * @param escaped_chars the set of chars to be preceded with one
   *                      backslash
   * @param uppercase true if the word is uppercase
   * @param firstupper true if the first letter of a word is uppercase
   * @param firstchar first character of the word
   * @return the result of the transduction
   */

  std::set<std::pair<UString, std::vector<UString> > > filterFinalsLRX(std::map<Node *, double> const &finals,
                                                        Alphabet const &a,
                                                        std::set<UChar32> const &escaped_chars,
                                                        bool uppercase = false,
                                                        bool firstupper = false,
                                                        int firstchar = 0) const;





  /**
   * Find final states, remove those that not has a requiredSymbol and 'restart' each of them as the
   * set of initial states, but remembering the sequence and adding a separationSymbol
   * @param finals
   * @param requiredSymbol
   * @param restart_state
   * @param separationSymbol
   */
    void restartFinals(const std::map<Node *, double> &finals, int requiredSymbol, State *restart_state, int separationSymbol);


  /**
   * Returns true if at least one record of the state references a
   * final node of the set
   * @param finals set of final nodes @return
   * @true if the state is final
   */
  bool isFinal(std::map<Node *, double> const &finals) const;

  /**
   * Return the full states string (to allow debuging...) using a Java ArrayList.toString style
   */
  UString getReadableString(const Alphabet &a);

  UString filterFinalsTM(std::map<Node *, double> const &finals,
                         Alphabet const &alphabet,
                         std::set<UChar32> const &escaped_chars,
                         std::queue<UString> &blanks,
                         std::vector<UString> &numbers) const;

  /**
   * Add all paths in other to self
   */
  void merge(const State& other);

};

#endif
