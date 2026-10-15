// Test double for IRadio: a shared "air" that delivers each broadcast to every other attached
// radio, stamped with the sender's MAC and the receiver clock's time. Unbounded queue: the
// 8-slot drop-newest policy belongs to the EspNowRadio adapter, not to the lib under test.
#pragma once

#include <cstring>
#include <deque>
#include <vector>

#include "ports/IClock.h"
#include "ports/IRadio.h"

namespace cubbies::test {

class LoopbackRadio;

struct Air {
  std::vector<LoopbackRadio*> radios;
};

class LoopbackRadio final : public IRadio {
 public:
  LoopbackRadio(Air& air, const Mac& mac, const IClock& clock) : air_(air), mac_(mac), clock_(clock) {
    air_.radios.push_back(this);
  }

  void broadcast(const uint8_t* bytes, size_t len) override {
    sent.emplace_back(bytes, bytes + len);
    for (LoopbackRadio* r : air_.radios)
      if (r != this) r->inject(bytes, len, mac_);
  }

  bool receive(RxFrame& out) override {
    if (queue_.empty()) return false;
    out = queue_.front();
    queue_.pop_front();
    return true;
  }

  // Puts raw bytes in this radio's queue as if received from `from` (fuzz, forged frames).
  void inject(const uint8_t* bytes, size_t len, const Mac& from) {
    RxFrame f{};
    f.len = len < kFrameSize ? len : kFrameSize;
    std::memcpy(f.bytes, bytes, f.len);
    f.len = len;  // the real length, so the lib sees oversize frames too
    f.mac = from;
    f.nowMs = clock_.nowMs();
    queue_.push_back(f);
  }

  size_t pending() const { return queue_.size(); }
  std::vector<std::vector<uint8_t>> sent;

 private:
  Air& air_;
  Mac mac_;
  const IClock& clock_;
  std::deque<RxFrame> queue_;
};

}  // namespace cubbies::test
