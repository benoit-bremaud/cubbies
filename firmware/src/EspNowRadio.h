// IRadio adapter: ESP-NOW broadcast on a fixed channel (ADR-0005, social-presence/06).
// The receive callback runs on the Wi-Fi task: it drops any frame whose length is not
// kFrameSize (own counter), stamps the time and the 802.11 source MAC, and pushes into an
// 8-slot FreeRTOS queue that fails when full, so the newest frame is dropped (owner decision
// 2026-10-15, MCDA 480/500). No parsing here, and no MAC is ever logged.
#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

#include <atomic>

#include "ports/IClock.h"
#include "ports/IRadio.h"

namespace cubbies {

// Fixed ESP-NOW channel, set explicitly so the fleet never depends on the driver's default
// (owner decision 2026-10-15, MCDA 380/500). Reopened by the V1.5 mother-cube ADR.
constexpr uint8_t kEspNowChannel = 1;

class EspNowRadio final : public IRadio {
 public:
  explicit EspNowRadio(const IClock& clock) : clock_(clock) {}
  // Call once Wi-Fi is started in station mode, before the first broadcast.
  bool begin();

  void broadcast(const uint8_t* bytes, size_t len) override;
  bool receive(RxFrame& out) override;

  uint32_t dropLen() const { return dropLen_.load(std::memory_order_relaxed); }
  uint32_t dropFull() const { return dropFull_.load(std::memory_order_relaxed); }

 private:
  static constexpr UBaseType_t kQueueSlots = 8;

  void onReceive(const uint8_t* mac, const uint8_t* data, int len);

  const IClock& clock_;
  QueueHandle_t queue_ = nullptr;
  std::atomic<uint32_t> dropLen_{0};
  std::atomic<uint32_t> dropFull_{0};
};

}  // namespace cubbies
