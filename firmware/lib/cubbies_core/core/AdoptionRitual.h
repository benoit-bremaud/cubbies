// Adoption ritual (identity/05b): tap moves the candidate colour forward, a double tap by two, a
// stroke confirms and writes it to storage once; no gesture for T_ritual_timeout abandons.
// Tick-driven: main.cpp feeds the Gesture classifier's output until isDone().
#pragma once

#include <cstdint>

#include "core/GestureClassifier.h"
#include "core/Identity.h"
#include "core/IdentityOutput.h"
#include "ports/IClock.h"
#include "ports/IStorage.h"

namespace cubbies {

class AdoptionRitual {
 public:
  AdoptionRitual(IClock& clock, IStorage& storage);

  IdentityOutput start(const Identity& identity);
  IdentityOutput tick(Gesture gesture);  // once per loop until isDone()
  bool isDone() const { return state_ == State::Done; }
  // The final identity: the confirmed colour, or the one passed to start() when abandoned or
  // when the write failed (fail closed).
  const Identity& identity() const { return identity_; }

  static constexpr uint32_t kTimeoutMs = 60000;  // of inactivity, tuning value (02b)

 private:
  enum class State : uint8_t { Idle, Cycling, Done };

  IClock& clock_;
  IStorage& storage_;
  State state_ = State::Idle;
  Identity identity_{};
  uint8_t candidate_ = 0;
  uint32_t lastMs_ = 0;  // last gesture, or start()
};

}  // namespace cubbies
