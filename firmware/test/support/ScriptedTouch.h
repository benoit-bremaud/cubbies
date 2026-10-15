// Test double for ITouchSource: a level the test sets by hand.
#pragma once

#include "ports/ITouchSource.h"

namespace cubbies::test {

class ScriptedTouch final : public ITouchSource {
 public:
  bool pressed() const override { return pressed_; }
  void set(bool pressed) { pressed_ = pressed; }

 private:
  bool pressed_ = false;
};

}  // namespace cubbies::test
