#include "core/JealousyRepertoire.h"

namespace cubbies {

namespace {

using R = JealousReaction;
constexpr uint8_t kWeary = kJealousContextCount - 1;
// "Weary sets Sulk to 0": a modifier no base weight can survive.
constexpr int8_t kZero = -100;

// 05e Contexts and weights. Modifier columns: Bored, Crowd, Newcomer, Friend, Neglected, Weary.
constexpr JealousRow kStep1[] = {
    {R::SideGlance, 3, {0, 0, 0, 2, 0, 2}},
    {R::Stare, 2, {0, 2, 0, 0, 0, 0}},
    {R::TurnAway, 2, {1, 0, 2, 0, 0, 0}},
};
constexpr JealousRow kStep2[] = {
    {R::Pout, 3, {0, 0, 0, 2, 0, 0}},
    {R::PuppyEyes, 2, {3, 0, 0, 0, 2, 0}},
    {R::MeToo, 2, {0, 3, 0, 0, 0, 0}},
    {R::FakeSleep, 1, {0, 0, 0, 0, 0, 4}},
    {R::Sulk, 2, {0, 0, 2, -2, 0, kZero}},
};

int clampZero(int w) { return w < 0 ? 0 : w; }

}  // namespace

uint32_t JealousWeights::total() const {
  uint32_t t = 0;
  for (uint8_t i = 0; i < count; ++i) t += weight[i];
  return t;
}

JealousTable jealousTable(JealousStep step) {
  if (step == JealousStep::Notice) return {kStep1, sizeof(kStep1) / sizeof(kStep1[0])};
  return {kStep2, sizeof(kStep2) / sizeof(kStep2[0])};
}

JealousWeights jealousWeights(const JealousTable& table, const JealousContexts& c, JealousReaction last) {
  const bool holds[kJealousContextCount] = {c.bored, c.crowd, c.newcomer, c.friend_, c.neglected, c.weary};
  JealousWeights w;
  w.count = table.count < kMaxStepReactions ? table.count : kMaxStepReactions;
  for (uint8_t i = 0; i < w.count; ++i) {
    const JealousRow& row = table.rows[i];
    int sum = row.base;
    for (uint8_t k = 0; k < kWeary; ++k)
      if (holds[k]) sum += row.mod[k];
    sum = clampZero(sum);
    if (c.weary) sum = clampZero(sum + row.mod[kWeary]);
    w.reaction[i] = row.reaction;
    w.weight[i] = row.reaction == last ? 0 : static_cast<uint8_t>(sum);
  }
  return w;
}

JealousWeights jealousWeights(JealousStep step, const JealousContexts& c, JealousReaction last) {
  return jealousWeights(jealousTable(step), c, last);
}

JealousReaction drawWeighted(const JealousWeights& w, Prng& prng) {
  const uint32_t total = w.total();
  if (total == 0) return JealousReaction::None;
  uint32_t r = prng.uniform(total);
  for (uint8_t i = 0; i < w.count; ++i) {
    if (r < w.weight[i]) return w.reaction[i];
    r -= w.weight[i];
  }
  return JealousReaction::None;  // unreachable: r < total
}

JealousReaction JealousyRepertoire::draw(JealousStep step, const JealousContexts& c) {
  JealousReaction& last = last_[static_cast<uint8_t>(step)];
  const JealousReaction r = drawWeighted(jealousWeights(step, c, last), prng_);
  if (r != JealousReaction::None) last = r;
  return r;
}

}  // namespace cubbies
