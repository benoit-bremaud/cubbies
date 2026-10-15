# Bench checklist — brick F5 adapters

Adapters in `firmware/src/` are validated by the board builds and on the bench (ADR-0014). Run
on one XIAO ESP32-C3 cube (series) and one Super Mini (prototype); two cubes for the radio rows.
Record results in `docs/hardware/measurements.md` (create it on first use) with date and board.

| # | Adapter | Check | Pass when |
|---|---------|-------|-----------|
| 1 | Ssd1331Display | Boot shows the eyes in the cube's colour | Colours match the palette (no red/blue swap, no byte swap) |
| 2 | Ssd1331Display | Frame time | Serial timing of `present()` ≤ 20 ms at 6 MHz |
| 3 | Ssd1331Display | Brightness Full / Low / Minimum (awake, dozing, asleep) | Three distinct levels, Minimum still readable in a dim room; tune `kBrightness` |
| 4 | Ttp223Touch | Tap, double tap, stroke, hold at boot | Gestures classified as in 05c; touch-down widens the eyes within one tick |
| 5 | I2sMic | Silence, speech at 1 m, clap, loud music | rms baseline stable; voice, clap, loud classified (05d); no sample printed on serial |
| 6 | BuzzerAudio | Each sound, volume level 1 | ≤ 40 dB(A) at 1 m (REQ-NF-03); note lengths audibly right; silence after the last note |
| 7 | BuzzerAudio | Doze during a melody | Sound stops at once |
| 8 | NvsStorage | Adoption ritual, power cycle | Adopted colour survives the reboot; erase NVS → MAC colour |
| 9 | EspNowRadio | Two cubes in range | Each greets the other once (Hello); presence every 5 s ± 1 s |
| 10 | EspNowRadio | Foreign ESP-NOW sender (wrong length / magic) | Dropped, counters increase, no crash |
| 11 | main.cpp | Tick budget | Loop period 40 ms with no overrun under radio traffic |
| 12 | Issue #16 | `Released` after a wake touch | Wake/Yawn animation not cut |
| 13 | Issue #16 | Own buzzer heard by the mic | No spurious LookUp; if seen, open a lib issue |
| 14 | Strapping pins | Reset with buzzer stage on GPIO2 / GPIO8 | Board boots every time (CDR check, gates.md) |
