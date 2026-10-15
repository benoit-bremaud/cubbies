// Test double for IStorage: an in-memory key/value map whose writes can be made to fail.
#pragma once

#include <cstdint>
#include <map>
#include <string>

#include "ports/IStorage.h"

namespace cubbies::test {

class FakeStorage final : public IStorage {
 public:
  bool readU8(const char* key, uint8_t& out) const override {
    const auto it = values.find(key);
    if (it == values.end()) return false;
    out = it->second;
    return true;
  }
  bool writeU8(const char* key, uint8_t value) override {
    ++writes;
    if (failWrites) return false;
    values[key] = value;
    return true;
  }

  std::map<std::string, uint8_t> values;
  bool failWrites = false;
  int writes = 0;
};

}  // namespace cubbies::test
