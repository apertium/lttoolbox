#include <reusable_state.h>

ReusableState::ReusableState() {}

ReusableState::~ReusableState()
{
  destroy();
}

ReusableState::ReusableState(const ReusableState& s)
{
  copy(s);
}

ReusableState& ReusableState::operator=(const ReusableState& s)
{
  if (this != &s) copy(s);
  return *this;
}

void ReusableState::destroy()
{
  for (size_t i = 0; i < steps.size(); i++) {
    delete steps[i];
  }
  steps.clear();
}

void ReusableState::copy(const ReusableState& s)
{
  destroy();
  size_t N = (s.end >> STATE_STEP_BLOCK_SIZE_EXP) + 1;
  steps.reserve(N);
  for (size_t i = 0; i < N; i++) {
    auto block = new std::array<Step,STATE_STEP_BLOCK_SIZE>;
    *block = *(s.steps[i]);
    steps.push_back(block);
  }
  start = s.start;
  end = s.end;
}

ReusableState::Step& ReusableState::create(size_t index)
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
      Step& next = create(end);
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
  create(0).where = initial;
  epsilonClosure();
}

void ReusableState::step(int32_t input)
{
  size_t new_start = end;
  for (size_t i = start; i < new_start; i++) {
    apply(input, i, 0, 0, false);
  }
  start = new_start;
  epsilonClosure();
}

void ReusableState::step(int32_t input, int32_t alt)
{
  if (alt == 0 || alt == input) {
    step(input);
    return;
  }
  size_t new_start = end;
  for (size_t i = start; i < new_start; i++) {
    apply(input, i, 0, 0, false);
    apply(alt, i, 0, 0, true);
  }
  start = new_start;
  epsilonClosure();
}

void ReusableState::step_override(int32_t input,
                                  int32_t old_sym, int32_t new_sym)
{
  size_t new_start = end;
  for (size_t i = start; i < new_start; i++) {
    apply(input, i, old_sym, new_sym, false);
  }
  start = new_start;
  epsilonClosure();
}

void ReusableState::step_override(int32_t input, int32_t alt,
                                  int32_t old_sym, int32_t new_sym)
{
  if (alt == 0 || alt == input) {
    step_override(input, old_sym, new_sym);
    return;
  }
  size_t new_start = end;
  for (size_t i = start; i < new_start; i++) {
    apply(input, i, old_sym, new_sym, false);
    apply(alt, i, old_sym, new_sym, true);
  }
  start = new_start;
  epsilonClosure();
}

void ReusableState::step_careful(int32_t input, int32_t alt)
{
  if (alt == 0 || alt == input) {
    step(input);
    return;
  }
  size_t new_start = end;
  for (size_t i = start; i < new_start; i++) {
    if (!apply(input, i, 0, 0, false)) {
      apply(alt, i, 0, 0, true);
    }
  }
  start = new_start;
  epsilonClosure();
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
  size_t new_start = end;
  for (size_t i = start; i < new_start; i++) {
    apply(input, i, 0, 0, false);
    apply(alt1, i, 0, 0, true);
    apply(alt2, i, 0, 0, true);
  }
  start = new_start;
  epsilonClosure();
}

void ReusableState::step(int32_t input, std::set<int> alts)
{
  size_t new_start = end;
  for (size_t i = start; i < new_start; i++) {
    apply(input, i, 0, 0, false);
    for (auto& a : alts) {
      if (a == 0 || a == input) continue;
      apply(a, i, 0, 0, true);
    }
  }
  start = new_start;
  epsilonClosure();
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
  size_t i = pos;
  std::vector<int32_t> symbols;
  while (i != 0) {
    auto& it = get(i);
    weight += it.weight;
    if (it.symbol) symbols.push_back(it.symbol);
    i = it.prev;
  }
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
