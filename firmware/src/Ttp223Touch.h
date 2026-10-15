// ITouchSource adapter: the TTP223 module drives its output high while touched (default
// jumpers: momentary, active high). Level only; every gesture timer lives in lib/.
#pragma once

#include <Arduino.h>

#include "ports/ITouchSource.h"

namespace cubbies {

class Ttp223Touch final : public ITouchSource {
 public:
  explicit Ttp223Touch(uint8_t pin) : pin_(pin) {}
  void begin() { pinMode(pin_, INPUT); }
  bool pressed() const override { return digitalRead(pin_) == HIGH; }

 private:
  uint8_t pin_;
};

}  // namespace cubbies
