// IStorage adapter over Preferences (NVS), namespace "cubbies". Typed: a key holding anything
// other than one uint8 reads as absent, never as a default that looks valid (02a).
#pragma once

#include <Preferences.h>

#include "ports/IStorage.h"

namespace cubbies {

class NvsStorage final : public IStorage {
 public:
  bool begin() { return prefs_.begin("cubbies", /*readOnly=*/false); }

  bool readU8(const char* key, uint8_t& out) const override {
    if (prefs_.getType(key) != PT_U8) return false;
    out = prefs_.getUChar(key);
    return true;
  }

  bool writeU8(const char* key, uint8_t value) override {
    return prefs_.putUChar(key, value) == sizeof(uint8_t);
  }

 private:
  mutable Preferences prefs_;  // its getters are not const; reading changes no stored state
};

}  // namespace cubbies
