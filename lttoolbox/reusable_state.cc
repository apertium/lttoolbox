#include <reusable_state.h>
#include <climits>

#define WalkBack(var, pos, block) { \
  size_t index = pos;               \
  while (index != 0) {              \
    auto& var = get(index);         \
    block                           \
    index = var.prev;               \
  }                                 \
  }

#define StepLoop(block) {                      \
  size_t new_start = end;                      \
  for (size_t i = start; i < new_start; i++) { \
    block                                      \
  }                                            \
  start = new_start;                           \
  epsilonClosure();                            \
  }

ReusableState::ReusableState() {}

ReusableState::~ReusableState()
{
  for (size_t i = 0; i < steps.size(); i++) {
    delete steps[i];
  }
  steps.clear();
}

ReusableState::Step& ReusableState::get_or_create(size_t index)
{
  size_t a = index >> STATE_STEP_BLOCK_SIZE_EXP;
  size_t b = index & (STATE_STEP_BLOCK_SIZE-1);
  while (a >= steps.size()) {
    auto block = new std::array<Step,STATE_STEP_BLOCK_SIZE>;
    steps.push_back(block);
  }
  return (*(steps[a]))[b];
}

const ReusableState::Step& ReusableState::get(size_t index) const
{
  size_t a = index >> STATE_STEP_BLOCK_SIZE_EXP;
  size_t b = index & (STATE_STEP_BLOCK_SIZE-1);
  return (*(steps[a]))[b];
}

bool ReusableState::apply(int32_t input, size_t pos,
                   int32_t old_sym, int32_t new_sym, bool dirty)
{
  auto& prev = get(pos);
  bool set_dirty = prev.dirty||dirty;
  std::map<int, Dest>::const_iterator it;
  it = prev.where->transitions.find(input);
  if (it != prev.where->transitions.end()) {
    for (int j = 0; j < it->second.size; j++) {
      Step& next = get_or_create(end);
      next.where = it->second.dest[j];
      next.symbol = it->second.out_tag[j];
      if (old_sym && next.symbol == old_sym) next.symbol = new_sym;
      next.weight = it->second.out_weight[j];
      next.dirty = set_dirty;
      next.prev = pos;
      end++;
    }
    return true;
  }
  return false;
}

void ReusableState::epsilonClosure()
{
  for (size_t i = start; i < end; i++) {
    apply(0, i, 0, 0, false);
  }
}

size_t ReusableState::size() const
{
  return end - start;
}

void ReusableState::init(Node* initial)
{
  while (steps.size() > STATE_RESET_SIZE) {
    delete steps[steps.size()-1];
    steps.pop_back();
  }
  start = 0;
  end = 1;
  get_or_create(0).where = initial;
  epsilonClosure();
}

void ReusableState::reinit(Node* initial)
{
  size_t start_was = start;
  get_or_create(end).where = initial;
  start = end;
  end++;
  epsilonClosure();
  start = start_was;
}

void ReusableState::step(int32_t input)
{
  StepLoop({
      apply(input, i, 0, 0, false);
    })
}

void ReusableState::step(int32_t input, int32_t alt)
{
  if (alt == 0 || alt == input) {
    step(input);
    return;
  }
  StepLoop({
      apply(input, i, 0, 0, false);
      apply(alt, i, 0, 0, true);
    })
}

void ReusableState::step_override(int32_t input,
                                  int32_t old_sym, int32_t new_sym)
{
  StepLoop({
      apply(input, i, old_sym, new_sym, false);
    })
}

void ReusableState::step_override(int32_t input, int32_t alt,
                                  int32_t old_sym, int32_t new_sym)
{
  if (alt == 0 || alt == input) {
    step_override(input, old_sym, new_sym);
    return;
  }
  StepLoop({
      apply(input, i, old_sym, new_sym, false);
      apply(alt, i, old_sym, new_sym, true);
    })
}

void ReusableState::step_careful(int32_t input, int32_t alt)
{
  if (alt == 0 || alt == input) {
    step(input);
    return;
  }
  StepLoop({
      if (!apply(input, i, 0, 0, false)) {
        apply(alt, i, 0, 0, true);
      }
    })
}

void ReusableState::step(int32_t input, int32_t alt1, int32_t alt2)
{
  if (alt1 == 0 || alt1 == input || alt1 == alt2) {
    step(input, alt2);
    return;
  } else if (alt2 == 0 || alt2 == input) {
    step(input, alt1);
    return;
  }
  StepLoop({
      apply(input, i, 0, 0, false);
      apply(alt1, i, 0, 0, true);
      apply(alt2, i, 0, 0, true);
    })
}

void ReusableState::step(int32_t input, std::set<int> alts)
{
  StepLoop({
      apply(input, i, 0, 0, false);
      for (auto& a : alts) {
        if (a == 0 || a == input) continue;
        apply(a, i, 0, 0, true);
      }
    })
}

void ReusableState::step_case(UChar32 val, UChar32 val2, bool caseSensitive)
{
  if (u_isupper(val) && !caseSensitive) {
    step(val, u_tolower(val), val2);
  } else {
    step(val, val2);
  }
}

void ReusableState::step_case(UChar32 val, bool caseSensitive)
{
  if (!u_isupper(val) || caseSensitive) {
    step(val);
  } else {
    step(val, u_tolower(val));
  }
}

void ReusableState::step_case_override(UChar32 val, bool caseSensitive)
{
  if (!u_isupper(val) || caseSensitive) {
    step(val);
  } else {
    step_override(val, u_tolower(val), u_tolower(val), val);
  }
}

void ReusableState::step_optional(int32_t val)
{
  size_t old_start = start;
  step(val);
  start = old_start;
}

bool ReusableState::isFinal(const std::map<Node*, double>& finals) const
{
  for (size_t i = start; i < end; i++) {
    if (finals.find(get(i).where) != finals.end()) return true;
  }
  return false;
}

void ReusableState::extract(size_t pos, UString& result, double& weight,
                            const Alphabet& alphabet,
                            const std::set<UChar32>& escaped_chars,
                            bool uppercase) const {
  std::vector<int32_t> symbols;
  WalkBack(it, pos, {
      weight += it.weight;
      if (it.symbol) symbols.push_back(it.symbol);
    })
  for (auto it = symbols.rbegin(); it != symbols.rend(); it++) {
    if (escaped_chars.find(*it) != escaped_chars.end()) result += '\\';
    alphabet.getSymbol(result, *it, uppercase);
  }
}

void NFinals(std::vector<std::pair<double, UString>>& results,
             size_t maxAnalyses, size_t maxWeightClasses)
{
  if (results.empty()) return;
  sort(results.begin(), results.end());
  if (maxAnalyses < results.size()) {
    results.erase(results.begin()+maxAnalyses, results.end());
  }
  if (maxWeightClasses < results.size()) {
    double last_weight = results[0].first + 1;
    for (size_t i = 0; i < results.size(); i++) {
      if (results[i].first != last_weight) {
        last_weight = results[i].first;
        if (maxWeightClasses == 0) {
          results.erase(results.begin()+i, results.end());
          return;
        }
        maxWeightClasses--;
      }
    }
  }
}

UString ReusableState::filterFinals(const std::map<Node*, double>& finals,
                                    const Alphabet& alphabet,
                                    const std::set<UChar32>& escaped_chars,
                                    bool display_weights,
                                    int max_analyses, int max_weight_classes,
                                    bool uppercase, bool firstupper,
                                    int firstchar) const
{
  std::vector<std::pair<double, UString>> results;

  UString temp;
  double weight;
  for (size_t i = start; i < end; i++) {
    auto fin = finals.find(get(i).where);
    if (fin != finals.end()) {
      weight = fin->second;
      temp.clear();
      extract(i, temp, weight, alphabet, escaped_chars, uppercase);
      if (firstupper && get(i).dirty) {
        int idx = (temp[firstchar] == '~' ? firstchar + 1 : firstchar);
        temp[idx] = u_toupper(temp[idx]);
      }
      results.push_back({weight, temp});
    }
  }

  NFinals(results, max_analyses, max_weight_classes);

  temp.clear();
  std::set<UString> seen;
  for (auto& it : results) {
    if (seen.find(it.second) != seen.end()) continue;
    seen.insert(it.second);
    temp += '/';
    temp += it.second;
    if (display_weights) {
      UChar wbuf[16]{};
      // if anyone wants a weight of 10000, this will not be enough
      u_sprintf(wbuf, "<W:%f>", it.first);
      temp += wbuf;
    }
  }
  return temp;
}

bool ReusableState::lastPartHasRequiredSymbol(size_t pos, int32_t symbol,
                                              int32_t separator)
{
  WalkBack(it, pos, {
      if (it.symbol == symbol) return true;
      else if (separator && it.symbol == separator) return false;
    });
  return false;
}

bool ReusableState::hasSymbol(int32_t symbol)
{
  for (size_t i = start; i < end; i++) {
    if (lastPartHasRequiredSymbol(i, symbol, 0)) return true;
  }
  return false;
}

void ReusableState::pruneCompounds(int32_t requiredSymbol, int32_t separator,
                                   int maxElements)
{
  int min = maxElements;
  size_t len = size();
  std::vector<int> count(len, 0);
  for (size_t i = 0; i < len; i++) {
    bool found = false;
    WalkBack(it, i+start, {
        if (it.symbol == requiredSymbol && count[i] == 0) found = true;
        else if (it.symbol == separator) {
          if (found) count[i]++;
          else {
            count[i] = INT_MAX;
            break;
          }
        }
      });
    if (count[i] < min) min = count[i];
  }
  size_t keep = 0;
  for (size_t i = 0; i < len; i++) {
    if (count[i] == min) {
      size_t src = start + i;
      size_t dest = start + keep;
      // move the step that we're keeping, overwriting one that's being
      // discarded, and shrink the state size
      if (src != dest) get_or_create(dest) = get(src);
      keep++;
    }
  }
  end = start + keep;
}

void ReusableState::restartFinals(const std::map<Node*, double>& finals,
                                  int32_t requiredSymbol, Node* restart,
                                  int32_t separator)
{
  if (restart == nullptr) return;
  for (size_t i = start, limit = end; i < limit; i++) {
    auto& step = get(i);
    if (finals.count(step.where) > 0 &&
        lastPartHasRequiredSymbol(i, requiredSymbol, separator)) {
      size_t start_was = start;
      start = end;
      end++;
      auto& newstep = get_or_create(start);
      newstep.where = restart;
      newstep.symbol = separator;
      newstep.prev = i;
      epsilonClosure();
      start = start_was;
    }
  }
}

void ReusableState::pruneStatesWithForbiddenSymbol(int32_t symbol)
{
  size_t keep = 0;
  for (size_t i = start; i < end; i++) {
    if (!lastPartHasRequiredSymbol(i, symbol, 0)) {
      size_t dest = start + keep;
      if (i != dest) get_or_create(dest) = get(i);
      keep++;
    }
  }
  end = start + keep;
}
