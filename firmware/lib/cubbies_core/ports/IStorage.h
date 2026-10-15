// Port: small persistent settings keyed by name (NVS on the cube). Typed accessors, so the
// adapter rejects a stored value of another size or type instead of returning a default that
// looks valid (02a). Real adapter: NvsStorage (F5); test double: FakeStorage.
#pragma once

#include <cstdint>

namespace cubbies {

class IStorage {
 public:
  virtual ~IStorage() = default;
  // True and `out` set only when `key` holds exactly one uint8.
  virtual bool readU8(const char* key, uint8_t& out) const = 0;
  // True when the value was committed.
  virtual bool writeU8(const char* key, uint8_t value) = 0;
};

}  // namespace cubbies
