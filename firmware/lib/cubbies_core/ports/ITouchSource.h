// Port: the touch pad level, read once per tick. All gesture timing lives in lib/ (03-component),
// so the adapter only reports pressed or released. Real adapter: Ttp223Touch (F5); test double:
// ScriptedTouch.
#pragma once

namespace cubbies {

class ITouchSource {
 public:
  virtual ~ITouchSource() = default;
  virtual bool pressed() const = 0;
};

}  // namespace cubbies
