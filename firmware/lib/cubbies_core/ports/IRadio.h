// Port: ESP-NOW broadcast (ADR-0005), pulled by the Social protocol (social-presence/05a, owner
// decision 2026-10-15). The adapter compares the received length to kFrameSize in its callback,
// queues accepted frames in 8 fixed slots (drop newest when full) and never parses them. The
// adapter counts its own length drops; the lib re-checks `len` as defence in depth (dropLen).
// Real adapter: EspNowRadio (F5); test double: LoopbackRadio.
#pragma once

#include <cstddef>
#include <cstdint>

#include "ports/Mac.h"

namespace cubbies {

// One received frame: its bytes, the sender MAC from the 802.11 header (never from the payload)
// and the receive time on the IClock timeline.
struct RxFrame {
  uint8_t bytes[kFrameSize];
  size_t len;
  Mac mac;
  uint32_t nowMs;
};

class IRadio {
 public:
  virtual ~IRadio() = default;
  virtual void broadcast(const uint8_t* bytes, size_t len) = 0;
  // True and `out` set when a frame was waiting; false when the queue is empty.
  virtual bool receive(RxFrame& out) = 0;
};

}  // namespace cubbies
