// Jealousy repertoire (social-group/05e, G6a): the weight tables of the two crescendo steps, the
// draw and the no-repeat memory. The Mood engine computes the contexts and decides the step; this
// class only weighs and draws, with its own lib-owned Prng (stream 3, ADR-0013).
#pragma once

#include <cstdint>

#include "core/Prng.h"

namespace cubbies {

// None, then the repertoire table of 05e in order: step 1 (silent), then step 2.
enum class JealousReaction : uint8_t { None, SideGlance, Stare, TurnAway, Pout, PuppyEyes, MeToo, FakeSleep, Sulk };
enum class JealousStep : uint8_t { Notice, Jealous };

// The contexts of 05e that hold for one draw, computed by the Mood engine.
struct JealousContexts {
  bool bored = false;
  bool crowd = false;
  bool newcomer = false;
  bool friend_ = false;
  bool neglected = false;
  bool weary = false;  // applied last
};

constexpr uint8_t kJealousContextCount = 6;  // the order of JealousContexts and JealousRow::mod
constexpr uint8_t kMaxStepReactions = 5;

// One reaction of a step: its base weight and one modifier per context.
struct JealousRow {
  JealousReaction reaction;
  uint8_t base;
  int8_t mod[kJealousContextCount];
};

struct JealousTable {
  const JealousRow* rows;
  uint8_t count;  // ≤ kMaxStepReactions
};

struct JealousWeights {
  uint8_t count = 0;
  JealousReaction reaction[kMaxStepReactions] = {};
  uint8_t weight[kMaxStepReactions] = {};
  uint32_t total() const;
};

JealousTable jealousTable(JealousStep step);
// Base weights plus the modifiers of every context that holds, clamped at 0, Weary last; `last`
// (the previous reaction of the step) gets weight 0.
JealousWeights jealousWeights(const JealousTable& table, const JealousContexts& c, JealousReaction last);
JealousWeights jealousWeights(JealousStep step, const JealousContexts& c, JealousReaction last);
// r = uniform(total), then the first reaction whose cumulative weight exceeds r; None if total is 0.
JealousReaction drawWeighted(const JealousWeights& w, Prng& prng);

class JealousyRepertoire {
 public:
  explicit JealousyRepertoire(uint64_t seed) : prng_(seed, kStream) {}
  JealousReaction draw(JealousStep step, const JealousContexts& c);

  static constexpr uint64_t kStream = 3;  // after the eyes (1) and the social protocol (2)

 private:
  Prng prng_;
  JealousReaction last_[2] = {JealousReaction::None, JealousReaction::None};  // per step
};

}  // namespace cubbies
