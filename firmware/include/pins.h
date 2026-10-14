// Pin map — one block per board env (ADR-0003, ADR-0007).
// Hard budget: 11 GPIO. SPI display 4, I2S mic 3, buzzer 1, touch 1, I2C bus 2.
// Any new peripheral goes on the I2C bus or replaces one of these.
#pragma once

#include <cstdint>

namespace cubbies::pins {

#if defined(CUBBIES_BOARD_XIAO_C3)
// Seeed XIAO ESP32-C3. GPIO9 is a strapping pin held high by the BOOT button pull-up; it is
// only ever driven by the MCU (display DC), never by an external device at reset.
constexpr uint8_t kDisplaySck = 8;   // D8
constexpr uint8_t kDisplayMosi = 10; // D10
constexpr uint8_t kDisplayCs = 3;    // D1
constexpr uint8_t kDisplayDc = 9;    // D9
constexpr uint8_t kMicBclk = 20;     // D7
constexpr uint8_t kMicWs = 21;       // D6
constexpr uint8_t kMicSd = 5;        // D3
constexpr uint8_t kBuzzer = 2;       // D0
constexpr uint8_t kTouch = 4;        // D2
constexpr uint8_t kI2cSda = 6;       // D4
constexpr uint8_t kI2cScl = 7;       // D5

#elif defined(CUBBIES_BOARD_C3_SUPERMINI)
// ESP32-C3 Super Mini (prototypes). GPIO8 carries the onboard LED, reused for the buzzer;
// GPIO9 (BOOT) is left unused; GPIO20/21 (UART) stay free for a serial probe.
constexpr uint8_t kDisplaySck = 4;
constexpr uint8_t kDisplayMosi = 6;
constexpr uint8_t kDisplayCs = 7;
constexpr uint8_t kDisplayDc = 5;
constexpr uint8_t kMicBclk = 2;
constexpr uint8_t kMicWs = 3;
constexpr uint8_t kMicSd = 10;
constexpr uint8_t kBuzzer = 8;
constexpr uint8_t kTouch = 1;
constexpr uint8_t kI2cSda = 0;
constexpr uint8_t kI2cScl = 21;

#elif defined(CUBBIES_NATIVE)
// Host build: no pins.

#else
#error "No cubbies board selected: define CUBBIES_BOARD_XIAO_C3 or CUBBIES_BOARD_C3_SUPERMINI"
#endif

}  // namespace cubbies::pins
