# Local review — cubbies (brick F5, adapters and main.cpp)

- Date: 2026-10-15
- Commit: c17fb5e (findings applied in the follow-up commit)
- Scope: branch diff main...feat/f5-adapters
- Checks run: design · docs · tests · security · correctness
- Verdict: ship after the Must Have fixes (applied)  ·  Must 3 · Should 4 · Nice 7

## 🔴 Must Have
- `firmware/src/I2sMic.cpp:31` `[correctness]` · `I2SClass::readBytes` loops until the buffer is full and returns 0 on timeout, so with timeout 0 every tick lost the partial chunk and logged an error · read with `i2s_channel_read(rxChan(), …, 0)` — **applied**
- `firmware/src/EspNowRadio.cpp:43` `[design|docs|correctness]` · a failed `begin()` left `receive()` calling `xQueueReceive(nullptr)` (assert, reboot loop) while the log said "social layer off" · `ready_` guard on `receive`/`broadcast`, log reworded — **applied**
- `firmware/src/BuzzerAudio.cpp:28` `[design|correctness]` · a failed `begin()` left `play()`/`stop()` on a null mutex or timer · `ready_` guard — **applied**

## 🟡 Should Have
- `firmware/src/EspNowRadio.cpp:22` `[correctness]` · STA modem power save makes ESP-NOW miss broadcasts · `esp_wifi_set_ps(WIFI_PS_NONE)` — **applied**
- `firmware/src/main.cpp` `runBoot` `[correctness]` · frames queued during the boot screens carry a stale `nowMs` · `radio.discardPending()` after boot — **applied**
- `firmware/src/Ssd1331Display.cpp:51` `[correctness]` · a failed init still drove the SPI bus every tick · `ready_` guard — **applied**
- `docs/hardware/bench-f5.md` `[docs]` · rows 2, 10, 11 relied on serial output the firmware does not print · stated as temporary bench instrumentation — **applied**

## 🔵 Nice to Have
- `firmware/src/I2sMic.cpp:40` `[correctness]` · left shift of a negative `int32_t` (UB before C++20) · `s * 256` — **applied**
- `firmware/src/Ssd1331Display.cpp:15` `[design]` · brightness table unchecked against the enum · `static_assert` — **applied**
- `firmware/src/main.cpp` `setup` `[correctness]` · `esp_read_mac` result ignored · logged — **applied**
- `firmware/src/BuzzerAudio.cpp:44` `[correctness]` · timer stop under the lock deserves a comment · **applied**
- `firmware/src/BuzzerAudio.cpp` `[correctness]` · zero-length notes arm a 0 µs timer · issue #25
- `firmware/src/main.cpp` newcomer path `[design]` · relies on `routeNewcomer` never setting `toSleep` · issue #25
- `firmware/src/EspNowRadio.h:34` `[design]` · drop counters only read by bench instrumentation · issue #25

## ⚪ Disagree / Intentional
- `firmware/src/main.cpp` loop `[design]` · "mood ticks only while Awake" and "gate learns the sleep state before the effect Route" live in main · the validated F2 wiring contract (StimulusRouter returns Routes that main applies, 03-component); no new rule
- `firmware/src/EspNowRadio.cpp:15` `[design]` · single static instance · ESP-NOW takes a plain C callback, one radio per cube
- `firmware/src/main.cpp` `Seeded` built with `new` `[design]` · Prngs seeded after Wi-Fi (ADR-0013), framebuffer off the loop stack

## Per-dimension summary
- Design: thin adapters, ports respected; init-failure robustness fixed · Docs: comments match the code; bench rows clarified · Tests: no lib change; adapters validated by builds and bench (ADR-0014), 220 native tests green · Security: radio length check in the callback, no MAC or sample logged, no secrets · Correctness: I2S read bug fixed; boot and tick order match the contract
