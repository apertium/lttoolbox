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
#ifndef _TRANSDUCTOR_
#define _TRANSDUCTOR_

#include <cstdio>
#include <map>
#include <set>

#include <lttoolbox/alphabet.h>
#include <lttoolbox/sorted_vector.hpp>


/**
  * Default value of weight.
  * Used as the identity element by various operations.
  */
constexpr double default_weight = 0;

class MatchExe;

/**
 * Class to represent a letter transducer during the dictionary compilation
 */
class Transducer
{
private:
  friend class MatchExe;

  /**
   * Initial state
   */
  int initial;

  /**
   * Final state set mapped to its weight walues
   * Schema: (state, weight)
   */
  std::map<int, double> finals;

  /**
   * Transitions of the transducer
   * Schema: (source state, tag, target state, weight)
   */
  std::map<int, std::multimap<int, std::pair<int, double> > > transitions;

  /**
   * New state creator
   * @return the new state number
   */
  int newState();

  /**
   * Copy function
   * @param t the transducer to be copied
   */
  void copy(Transducer const &t);

  /**
   * Empty transducer
   */
  void destroy();

  /**
   * Helper function for show()
   * @param symbol the string to be escaped
   * @param hfst if true, use HFST-compatible escape sequences
   */
  void escapeSymbol(UString& symbol, bool hfst) const;

public:

  /**
   * String constants
   */
  static constexpr UStringView HFST_EPSILON_SYMBOL_SHORT   = u"@0@";
  static constexpr UStringView HFST_EPSILON_SYMBOL_LONG    = u"@_EPSILON_SYMBOL_@";
  static constexpr UStringView LTTB_EPSILON_SYMBOL         = u"ε";
  static constexpr UStringView HFST_SPACE_SYMBOL           = u"@_SPACE_@";
  static constexpr UStringView HFST_TAB_SYMBOL             = u"@_TAB_@";
  static constexpr UStringView GROUP_SYMBOL                = u"#";
  static constexpr UStringView JOIN_SYMBOL                 = u"+";
  static constexpr UStringView ANY_TAG_SYMBOL              = u"<ANY_TAG>";
  static constexpr UStringView ANY_CHAR_SYMBOL             = u"<ANY_CHAR>";
  static constexpr UStringView LSX_BOUNDARY_SYMBOL         = u"<$>";
  static constexpr UStringView LSX_BOUNDARY_SPACE_SYMBOL   = u"<$_>";
  static constexpr UStringView LSX_BOUNDARY_NO_SPACE_SYMBOL= u"<$->";
  static constexpr UStringView READING_SEPARATOR_SYMBOL    = u"</>";
  static constexpr UStringView COMPOUND_ONLY_L_SYMBOL      = u"<compound-only-L>";
  static constexpr UStringView COMPOUND_R_SYMBOL           = u"<compound-R>";

  /**
   * Constructor
   */
  Transducer();

  /**
   * Destructor
   */
  ~Transducer();

  /**
   * Copy constructor
   * @param t transducer to be copied
   */
  Transducer(Transducer const &t);

  /**
   * Assignment operator
   * @param t transducer to be assigned
   * @return the object result of the assignment
   */
  Transducer & operator =(Transducer const &t);

  /**
   * Determine whether any weight is non-default
   * @return bool true or false
   */
  bool weighted() const;

  /**
   * Insertion of a single transduction, creating a new target state
   * if needed
   * @param tag the tag of the transduction being inserted
   * @param source the source state of the new transduction
   * @param weight the weight value for the new transduction
   * @return the target state
   */
  int insertSingleTransduction(int tag, int source, double weight = 0.0000);

  /**
   * Insertion of a single transduction, forcing create a new target
   * state
   * @param tag the tag of the transduction being inserted
   * @param source the source state of the new transduction
   * @param weight the weight value for the new transduction
   * @return the target state
   */
  int insertNewSingleTransduction(int tag, int source, double weight = 0.0000);

  /**
   * Insertion of a transducer in a given source state, unifying their
   * final states using a optionally given epsilon tag
   * @param source the source state
   * @param t the transducer being inserted
   * @param epsilon_tag the epsilon tag
   * @return the new target state
   */
  int insertTransducer(int source, Transducer &t,
                       int epsilon_tag = 0);

  /**
   * Link two existing states by a transduction
   * @param source the source state
   * @param target the target state
   * @param tag the tag of the transduction
   * @param weight the weight value for the new transduction
   */
  void linkStates(int source, int target, int tag, double weight = 0.0000);

  /**
   * Test if the state is a final state
   * @param state the state
   * @return true if is a final state
   */
  bool isFinal(int state) const;

  /**
   * Test if a pattern is recognised by the FST
   * @param a widestring of the pattern to be recognised
   * @return true if the pattern is recognised by the transducer
   */
  bool recognise(UStringView pattern, Alphabet &a, FILE *err = stderr) const;

  /**
   * Set the state as a final or not, yes by default
   * @param state the state
   * @param weight the weight value for the final state
   * @param value if true, the state is set as final state
   */
  void setFinal(int state, double weight = 0.0000, bool value = true);

  /**
   * Returns the initial state of a transducer
   * @return the initial state identifier
   */
  int getInitial() const;

  /**
   * Returns the epsilon closure of a given state
   * @param state the state
   * @param epsilon_tag the tag to take as epsilon
   * @return the epsilon-connected states
   */
  std::set<int> closure(int state, int epsilon_tag = 0) const;

  /**
   * Returns the epsilon closure of a given state
   * @param state the state
   * @param epsilon_tags the tags to treat as epsilon
   * @return the epsilon-connected states
   */
  std::set<int> closure(int state, std::set<int> const &epsilon_tags) const;

  std::vector<sorted_vector<int>> closure_all(int epsilon_tag) const;

  /**
   * Join all finals in one using epsilon transductions
   * @param epsilon_tag the tag to take as epsilon
   */
  void joinFinals(int epsilon_tag = 0);


  /**
   * Return a copy of the final states
   */
  std::map<int, double> getFinals() const;

  /**
   * Return reference to the transitions
   */
  std::map<int, std::multimap<int, std::pair<int, double> > >& getTransitions();

  /**
   * Reverse all the transductions of a transducer
   * @param epsilon_tag the tag to take as epsilon
   */
  void reverse(int epsilon_tag = 0);

  /**
   * Print all the transductions of a transducer in ATT format
   * @param epsilon_tag the tag to take as epsilon
   * @param hfst if true, use HFST-compatible escape characters
   */
  void show(Alphabet const &a, UFILE *output, int epsilon_tag = 0, bool hfst = false) const;

  /**
   * Determinize the transducer
   * @param epsilon_tag the tag to take as epsilon
   */
  void determinize(int epsilon_tag = 0);

  /**
   * Minimize = reverse + determinize + reverse + determinize
   * @param epsilon_tag the tag to take as epsilon
   */
  void minimize(int epsilon_tag = 0);


  /**
   * Make a transducer optional (link initial state with final states with
   * empty transductions)
   * @param epsilon_tag the tag to take as epsilon
   */
  void optional(int epsilon_tag = 0);

  /**
   * Make a transducer cyclic (link final states with initial state with
   * empty transductions)
   * @param epsilon_tag the tag to take as epsilon
   */
  void oneOrMore(int epsilon_tag = 0);

  /**
   * zeroOrMore = oneOrMore + optional
   * @param epsilon_tag the tag to take as epsilon
   */
  void zeroOrMore(int epsilon_tag = 0);

  /**
   * Clear transducer
   */
  void clear();

  /**
   * Check if the transducer is empty
   * @return true if the transducer is empty
   */
  bool isEmpty() const;

  /**
   * Check if the transducer has no final state(s)
   * @return true if the set of final states is empty
   */
  bool hasNoFinals() const;

  /**
   * Returns the number of states of the transducer
   * @return the number of states
   */
  int size() const;

  /**
   * Returns the number of transitions of the transducer
   * @return the number of transitions
   */
  int numberOfTransitions() const;

  /**
   * Checks if a state is empty (not outgoing transductions)
   * @param state the state to check
   * @return true if the state is empty
   */
  bool isEmpty(int state) const;

  /**
   * Returns the number of transitions from a given state
   * @return the number of transitions
   */
  int getStateSize(int state);

  /**
   * Write method
   * @param output the stream to write to
   * @param decalage offset to sum to the tags
   */
  void write(FILE *output, int decalage = 0);

  /**
   * Read method
   * @param input the stream to read from
   * @param decalage offset to sum to the tags
   */
  void read(FILE *input, int decalage = 0);

  void serialise(std::ostream &serialised) const;
  void deserialise(std::istream &serialised);

  /**
   * Insert another transducer into this, unifying source and targets.
   * Does not minimize.
   *
   * @param t the transducer being inserted
   * @param epsilon_tag the epsilon tag
   */
  void unionWith(Alphabet &my_a,
                 Transducer &t,
                 int epsilon_tag = 0);

  /**
   * Converts this class into a "prefix transducer", ie. for any final
   * state, appends a transition to itself for all of the loopback
   * symbols (typically all tags). So if the original transducer
   * accepts "foo<tag1>" and loopback_symbols includes <tag2>, the
   * prefix transducer will accept "foo<tag1><tag2>" as well as
   * "foo<tag1>".
   *
   * @param loopback_symbols a set of symbols of the alphabet for this class,
   *   all of the input and output tags of which are set equal
   * @param epsilon_tag the tag to take as epsilon
   * @return the prefix transducer
   */
  Transducer appendDotStar(std::set<int> const &loopback_symbols,
                           int epsilon_tag = 0);


  /**
   * Turn "foo# bar<tag1><tag2>" into "foo<tag1><tag2># bar". Only
   * regards the input (left) side, output side will be in wrong
   * order. Used on bidixes when trimming.
   *
   * @param alphabet the alphabet of this transducer, also used for returned Transducer
   * @param epsilon_tag the tag to take as epsilon
   * @return the prefix transducer
   */
  Transducer moveLemqsLast(Alphabet const &alphabet,
                           int epsilon_tag = 0);
  /**
   * Helper for moveLemqsLast. Starting from a certain state, make all
   * the tags go before the non-tags, so if " bar<tag1><tag2>" is a
   * posible path, we get "<tag1><tag2> bar" in the returned
   * transducer.
   *
   * @param start the state (in this Transducer) to start from
   * @param group_label the label of the "#" symbol we saw
   * @param alphabet the alphabet of this transducer
   * @param epsilon_tag the tag to take as epsilon
   * @return a transducer of all paths going from start, but with tags first.
   */
  Transducer copyWithTagsFirst(int start,
                               int group_label,
                               Alphabet const &alphabet,
                               int epsilon_tag = 0);

  /**
   * Intersect with a bidix transducer, retaining only those analyses
   * which would pass through the bidix, respecting multiwords.
   *
   * This is "intersection-and-then-some" – we assume this transducer
   * is a monolingual analyser, and &t is a bidix, where outputs from
   * the analyser would go through apertium-pretransfer before being
   * sent to the bidix. So it has to handle the same transformations
   * that apertium-pretransfer applies, e.g. "^a<n>+b<n>#c$" from the
   * monodix analyser has to match "^a#c<n>$" and "^b<n>$" in the
   * bidix.
   *
   * The returned transducer is not minimized! Minimization will exit
   * with failure if there are no finals, but we might want to
   * continue with intersecting the other sections.
   *
   * @param bi the Transducer with which this class is intersected
   * @param my_a the alphabet of this transducer
   * @param bi_a the alphabet of the transducer bi
   * @return the trimmed transducer
   */
  Transducer trim(Transducer &bi,
                       Alphabet const &my_a,
                       Alphabet const &bi_a,
                       int epsilon_tag = 0);

  /**
   * Composes two finite-state transducers
   *
   * The returned transducer is not minimized! Minimization will exit
   * with failure if there are no finals, but we might want to
   * continue with intersecting the other sections.
   *
   * Weights are added. The alphabet of this transducer is re-used –
   * and may be altered if g adds new symbols!
   *
   * @param g the Transducer with which this is composed
   * @param my_a the alphabet of this transducer
   * @param g_a the alphabet of the transducer g
   * @param f_inverted run composition right-to-left on this transducer
   * @param g_anywhere don't require anchored matches, let g optionally compose at any sub-path
   * @return the composition gf
   */
  Transducer compose(Transducer const &g,
                     Alphabet &my_a, // not const since we may add symbols from g_a
                     Alphabet const &g_a,
                     bool f_inverted = false,
                     bool g_anywhere = false,
                     int epsilon_tag = 0);

  /**
   * Helper for creating the arc label when composing g ∘ f, used by compose.
   *
   * @return symbol-pair going from input of f to output of g (left/right-inverted if f_inverted)
   */
  inline
  int32_t composeLabel(Alphabet &f_a, Alphabet const &g_a,
                       int32_t f_input, int32_t g_right,
                       bool f_inverted) const
  {
    int32_t gf_label;
    if (g_right >= 0) { // optimisation: non-symbols are equal across alphabets
      gf_label = f_inverted ? f_a(g_right, f_input)
                            : f_a(f_input, g_right);
    }
    else {
      UString g_rightstr;
      g_a.getSymbol(g_rightstr, g_right);
      gf_label = f_inverted ? f_a(f_a(g_rightstr), f_input)
                            : f_a(f_input,         f_a(g_rightstr));
    }
    return gf_label;
  }

  /**
   * Ensure that new_alpha contains all the symbols in old_alpha
   * and update all transitions to the symbol numbers of new_alpha.
   * If has_pairs is false, transition labels will be looked up as
   * single symbols rather than pairs.
   */
  void updateAlphabet(Alphabet& old_alpha, Alphabet& new_alpha, bool has_pairs = true);

  /**
   * Invert all transitions so x:y becomes y:x (this will update alpha).
   */
  void invert(Alphabet& alpha);

  /**
   * Deletes all transitions with a symbol pair in syms
   */
  void deleteSymbols(const sorted_vector<int32_t>& syms);

  /**
   * For every transition with a symbol pair in syms,
   * change the symbol pair to epsilon
   */
  void epsilonizeSymbols(const sorted_vector<int32_t>& syms);

  /**
   * Given a map of symbols, x:[a,b,c],
   * expand all x:y transitions to x:y, a:y, b:y, c:y
   */
  void applyACX(Alphabet& alpha, const std::map<int32_t, sorted_vector<int32_t>>& acx);
};

#endif
