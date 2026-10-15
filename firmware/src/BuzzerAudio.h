// IAudio adapter: passive buzzer on LEDC PWM. The only place a volume level becomes a duty
// (03-component "one sound egress"). Notes are stepped by a one-shot esp_timer (owner decision
// 2026-10-15, MCDA 470/500): exact note lengths, no task, no policy. play() and stop() run on
// the loop task and the timer callback on the esp_timer task, so a mutex guards the sequence.
#pragma once

#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include "ports/IAudio.h"

namespace cubbies {

class BuzzerAudio final : public IAudio {
 public:
  explicit BuzzerAudio(uint8_t pin) : pin_(pin) {}
  bool begin();  // when it fails, play() and stop() do nothing: the cube stays silent

  void play(const Note* notes, uint8_t count, VolumeLevel level) override;
  void stop() override;

 private:
  static void onTimer(void* self);
  void startNoteLocked();  // plays notes_[index_] and arms the timer, or goes silent at the end

  uint8_t pin_;
  bool ready_ = false;
  esp_timer_handle_t timer_ = nullptr;
  SemaphoreHandle_t lock_ = nullptr;
  const Note* notes_ = nullptr;
  uint8_t count_ = 0;
  uint8_t index_ = 0;
  uint32_t duty_ = 0;
  int64_t noteEndUs_ = 0;  // when the current note's timer is due
};

}  // namespace cubbies
