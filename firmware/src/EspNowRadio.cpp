#include "EspNowRadio.h"

#include <esp_now.h>
#include <esp_wifi.h>

#include <cstring>

namespace cubbies {

namespace {

constexpr uint8_t kBroadcastMac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// esp_now_register_recv_cb takes a plain function: one radio per cube, so a single instance.
EspNowRadio* instance = nullptr;

}  // namespace

bool EspNowRadio::begin() {
  queue_ = xQueueCreate(kQueueSlots, sizeof(RxFrame));
  if (queue_ == nullptr) return false;
  if (esp_wifi_set_channel(kEspNowChannel, WIFI_SECOND_CHAN_NONE) != ESP_OK) return false;
  if (esp_now_init() != ESP_OK) return false;

  esp_now_peer_info_t peer = {};
  std::memcpy(peer.peer_addr, kBroadcastMac, sizeof(kBroadcastMac));
  peer.channel = kEspNowChannel;
  peer.ifidx = WIFI_IF_STA;
  peer.encrypt = false;  // ADR-0012: unauthenticated broadcast, accepted residual risk
  if (esp_now_add_peer(&peer) != ESP_OK) return false;

  instance = this;
  return esp_now_register_recv_cb([](const esp_now_recv_info_t* info, const uint8_t* data,
                                     int len) {
           if (instance != nullptr && info != nullptr) instance->onReceive(info->src_addr, data, len);
         }) == ESP_OK;
}

void EspNowRadio::broadcast(const uint8_t* bytes, size_t len) {
  esp_now_send(kBroadcastMac, bytes, len);  // fire and forget: presence repeats every 5 s
}

bool EspNowRadio::receive(RxFrame& out) { return xQueueReceive(queue_, &out, 0) == pdTRUE; }

void EspNowRadio::onReceive(const uint8_t* mac, const uint8_t* data, int len) {
  if (len != static_cast<int>(kFrameSize) || data == nullptr || mac == nullptr) {
    dropLen_.fetch_add(1, std::memory_order_relaxed);
    return;
  }
  RxFrame f;
  std::memcpy(f.bytes, data, kFrameSize);
  f.len = kFrameSize;
  std::memcpy(f.mac.data(), mac, f.mac.size());
  f.nowMs = clock_.nowMs();
  if (xQueueSend(queue_, &f, 0) != pdTRUE) dropFull_.fetch_add(1, std::memory_order_relaxed);
}

}  // namespace cubbies
