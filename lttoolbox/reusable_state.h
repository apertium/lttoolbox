#ifndef __LT_REUSABLE_STATE__
#define __LT_REUSABLE_STATE__

#include <lttoolbox/alphabet.h>
#include <lttoolbox/node.h>
#include <lttoolbox/ustring.h>

#include <array>
#include <set>
#include <vector>

#define STATE_STEP_BLOCK_SIZE_EXP 8
#define STATE_STEP_BLOCK_SIZE (1<<STATE_STEP_BLOCK_SIZE_EXP)
#define STATE_RESET_SIZE 16

class ReusableState {
private:
  struct Step {
    Node* where = nullptr;
    int32_t symbol = 0;
    double weight = 0.000;
    bool dirty = false;
    size_t prev = 0;
  };

  std::vector<std::array<Step,STATE_STEP_BLOCK_SIZE>*> steps;
  size_t start = 0;
  size_t end = 1;

  Step& get_or_create(size_t index);
  const Step& get(size_t index) const;

  bool apply(int32_t input, size_t pos, int32_t old_sym, int32_t new_sym,
             bool dirty);

  void epsilonClosure();

  void extract(size_t pos, UString& result, double& weight,
               const Alphabet& alphabet,
               const std::set<UChar32>& escaped_chars, bool uppercase) const;

public:
  ReusableState();
  ~ReusableState();

  size_t size() const;
  void init(Node* initial);
  void reinit(Node* initial);

  void step(int32_t input);
  void step(int32_t input, int32_t alt);
  void step_override(int32_t input, int32_t old_sym, int32_t new_sym);
  void step_override(int32_t input, int32_t alt,
                     int32_t old_sym, int32_t new_sym);
  void step_careful(int32_t input, int32_t alt);
  void step(int32_t input, int32_t alt1, int32_t alt2);
  void step(int32_t input, std::set<int> alts);
  void step_case(UChar32 val, UChar32 val2, bool caseSensitive);
  void step_case(UChar32 val, bool caseSensitive);
  void step_case_override(UChar32 val, bool caseSensitive);
  void step_optional(int32_t val);

  bool isFinal(const std::map<Node*, double>& finals) const;

  UString filterFinals(const std::map<Node*, double>& finals,
                       const Alphabet& alphabet,
                       const std::set<UChar32>& escaped_chars,
                       bool display_weights,
                       int max_analyses, int max_weight_classes,
                       bool uppercase, bool firstupper,
                       int firstchar = 0) const;

  bool lastPartHasRequiredSymbol(size_t pos, int32_t symbol, int32_t separator);
  bool hasSymbol(int32_t symbol);
  void pruneCompounds(int32_t requiredSymbol, int32_t separator,
                      int maxElements);
  void restartFinals(const std::map<Node*, double>& finals,
                     int32_t requiredSymbol, Node* restart_state,
                     int32_t separator);
  void pruneStatesWithForbiddenSymbol(int32_t symbol);
};

#endif
