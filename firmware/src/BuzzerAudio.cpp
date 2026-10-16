#include "BuzzerAudio.h"

#include <Arduino.h>

namespace cubbies {

namespace {

constexpr uint8_t kResolutionBits = 10;
constexpr uint32_t kIdleHz = 2000;  // any value in the 1–5 kHz range of the spec; output is 0 %

// Duty per volume level, 10-bit: 2 %, 8 %, 20 %, 50 % (docs/hardware/calculations/log.md C-0003; the
// spec caps the duty at 50 %, the loudest a square wave gets).
constexpr uint32_t kDuty[kMaxVolume + 1] = {20, 82, 205, 512};

class Lock {
 public:
  explicit Lock(SemaphoreHandle_t m) : m_(m) { xSemaphoreTake(m_, portMAX_DELAY); }
  ~Lock() { xSemaphoreGive(m_); }

 private:
  SemaphoreHandle_t m_;
};

}  // namespace

bool BuzzerAudio::begin() {
  lock_ = xSemaphoreCreateMutex();
  if (lock_ == nullptr || !ledcAttach(pin_, kIdleHz, kResolutionBits)) return false;
  ledcWrite(pin_, 0);
  const esp_timer_create_args_t args = {
      .callback = &BuzzerAudio::onTimer,
      .arg = this,
      .dispatch_method = ESP_TIMER_TASK,
      .name = "buzzer",
      .skip_unhandled_events = true,
  };
  ready_ = esp_timer_create(&args, &timer_) == ESP_OK;
  return ready_;
}

void BuzzerAudio::play(const Note* notes, uint8_t count, VolumeLevel level) {
  if (!ready_) return;
  Lock l(lock_);
  // Never waits for a callback blocked on the lock; noteEndUs_ makes that one a no-op.
  esp_timer_stop(timer_);  // ESP_ERR_INVALID_STATE when idle: harmless
  notes_ = notes;
  count_ = count;
  index_ = 0;
  duty_ = kDuty[level <= kMaxVolume ? level : kMaxVolume];
  startNoteLocked();
}

void BuzzerAudio::stop() {
  if (!ready_) return;
  Lock l(lock_);
  esp_timer_stop(timer_);
  count_ = 0;
  ledcWrite(pin_, 0);
}

void BuzzerAudio::onTimer(void* self) {
  auto* a = static_cast<BuzzerAudio*>(self);
  Lock l(a->lock_);
  // Stale: fired for a sequence that stop() or play() replaced while it waited for the lock.
  if (a->count_ == 0 || esp_timer_get_time() < a->noteEndUs_) return;
  ++a->index_;
  a->startNoteLocked();
}

void BuzzerAudio::startNoteLocked() {
  if (index_ >= count_) {
    count_ = 0;
    ledcWrite(pin_, 0);
    return;
  }
  const Note& n = notes_[index_];
  if (n.hz == 0) {
    ledcWrite(pin_, 0);
  } else {
    ledcChangeFrequency(pin_, n.hz, kResolutionBits);
    ledcWrite(pin_, duty_);
  }
  const uint64_t us = static_cast<uint64_t>(n.ms) * 1000u;
  noteEndUs_ = esp_timer_get_time() + static_cast<int64_t>(us);
  esp_timer_start_once(timer_, us);
}

}  // namespace cubbies
