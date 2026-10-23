# Voice and AI interaction — research note (October 2026)

Issue #61. How far can cubbies go towards "interacting with humans by voice", and possibly with an
AI later? This note gathers the state of the art, the constraints of the frozen V1 hardware and
the options; the decision is drafted in
[ADR-0017](../decisions/ADR-0017-voice-group-sound-cues-first.md). Checked on 2026-10-23; library
facts to re-check before any implementation brick.

## Origin

The owner's idea (2026-10-23): cubbies do not walk, but they interact with each other and, in an
advanced version, with humans by voice, maybe connected to an AI "later". Inspiration:
[MicroDuck](https://huggingface.co/blog/nvidia/microduck-isaac-lab-sim-to-real) (Pollen Robotics /
Hugging Face open-source biped trained in Isaac Lab, sim-to-real) and the GTC Berlin 2026 video of
many ducks together. The blog post has no voice or LLM content: voice is the owner's
extrapolation. Asked what to keep first, the owner chose the **group effect** (MCDA below).

## The V1 constraints

- **Chip:** XIAO ESP32-C3, single RISC-V core at 160 MHz, ~400 KB SRAM, no PSRAM, no vector
  instructions. Frozen at the CDR (ADR-0011).
- **Microphone:** INMP441 I2S MEMS (P1), on the only I2S peripheral; no speaker, a passive buzzer
  (`docs/bom.md`).
- **No time source, no Wi-Fi in V1** (ADR-0002); Wi-Fi only on the V1.5 mother cube, credentials
  in NVS.
- **The cube must never be loud** and goes through one Sound gate.

## State of the art

### On-device keyword spotting

| Option | Chips | Fits the C3? | Notes |
|--------|-------|--------------|-------|
| Espressif ESP-SR (AFE, WakeNet, MultiNet) | ESP32-S3, ESP32-P4 recommended (AI instructions, octal PSRAM); new models dropped for the original ESP32 | **No** — the C3 is not a target | WakeNet9 offers a few fixed or TTS-trained wake words; MultiNet adds offline commands on S3 [1] |
| TensorFlow Lite Micro + ESP-NN (`micro_speech`) | all ESP32 incl. C3 | Possible, tight | ESP-NN makes person detection ~8× faster on the C3 (3355 → 426 ms at 160 MHz), against 54 ms on an S3 [2]; a tiny "yes/no" model fits, a custom vocabulary needs training and leaves little CPU for the eyes |
| Edge Impulse / custom TinyML | C3 to S3 | Possible for 1–3 sounds | dataset + training pipeline to own; per-voice accuracy is the risk |
| Energy and spectral features (RMS, onset, zero-crossings) | any | **Yes** | enough for clap, knock, "someone is talking", loudness and rough prosody; no words, no model |

### Cloud speech and LLM through a mother cube

A Wi-Fi cube (V1.5 mother cube, an S3 for voice) can stream a wake-word-gated utterance to a
speech-to-text service and an LLM, then broadcast a short command over ESP-NOW. It brings real
language understanding, at the price of: an internet dependency, an API key (NVS only, never in
source), latency of seconds, running costs, and speech leaving the office.

### Privacy (GDPR) of an always-on office mic

A microphone listening all day in an open space processes colleagues' voices, which is personal
data as soon as it is recorded or transmitted. Principles (security policy, privacy section):

- **Process locally, keep nothing.** Features computed in RAM per frame and discarded; no audio
  buffer persisted, no audio on the radio.
- **No words is the strongest guarantee.** A cube that reacts to a clap or a loudness level cannot
  reveal what was said.
- **Cloud speech = a different regime:** information to every colleague, explicit activation
  (wake word or button), data-processing terms of the provider, and the company's own policy. Not
  a gift-grade feature.

### Power and BOM impact

| Path | BOM delta per cube | Power | Firmware cost |
|------|--------------------|-------|---------------|
| Sound cues on V1 | 0 € (mic already there) | mic + I2S DMA always on while awake, already budgeted | small DSP in an adapter, pure detector in `lib/` |
| One ear cube (V1.5) | one XIAO S3 (~14 €) for the group | mains-powered mother cube | ESP-SR or cloud client, new frame type |
| S3 in every cube (V4) | +~8 € per cube, plus amp and speaker for real voice | higher, PSRAM | a second firmware family against "one firmware" (ADR-0008) unless all cubes migrate |

### MicroDuck ideas that transfer

- **Fleet simulator:** N virtual cubes over `LoopbackRadio` in the native build, rendered side by
  side, to watch group behaviours (chain, choir, collective sleep, a clap wave) before the bench.
- **Domain randomisation:** randomise radio loss, delays and clock drift in native tests, as
  MicroDuck randomises physics, so group behaviours stay robust.
- **A community arena** of behaviours, later.

## Options and owner direction

Criteria and weights (total 100): fit with the cubbies identity 30, feasibility on V1 25,
privacy 20, wow effect 15, BOM/power 10. Scores 0–5, totals out of 500.

| Option | Identity | Feasibility | Privacy | Wow | BOM | **Total** |
|--------|----------|-------------|---------|-----|-----|-----------|
| A. Group effect: a human sound (clap, call) triggers the group, no words | 5 | 4 | 5 | 4 | 5 | **460** |
| C. Personality: each cube reacts to the tone of voice (prosody), no words | 4 | 4 | 4 | 3 | 5 | 395 |
| B. Voice commands: one S3 "ear" cube (V1.5) broadcasts commands | 4 | 3 | 3 | 5 | 3 | 360 |

Sensitivity: with wow 30 and identity 15, A 445 vs B 375 — A stays first. **The owner chose A on
2026-10-23.** C extends A later on the same hardware; B and any AI link wait for the mother cube.

## Open questions for the implementation brick

- Detector: clap vs knock vs loud call, thresholds on the INMP441, false positives from keyboards
  and doors (bench measurements, verification plan).
- Group reaction: which existing behaviour a sound cue starts (chain G3, choir G4) and how the
  cubes that heard it agree on one wave (each cube hears the clap itself, so no frame may be
  needed).
- Self-hearing: the buzzer must not trigger the detector (issue #16).

## Sources

1. Espressif ESP-SR README — supported chips and WakeNet models:
   <https://github.com/espressif/esp-sr> (mirrors consulted: <https://github.com/esphome/esp-sr>).
2. Espressif esp-tflite-micro README, ESP-NN performance table:
   <https://github.com/espressif/esp-tflite-micro>.
3. MicroDuck sim-to-real blog: <https://huggingface.co/blog/nvidia/microduck-isaac-lab-sim-to-real>.
4. GTC Berlin 2026 video (not viewed by the author): <https://www.youtube.com/watch?v=WcZUkB-dgM8>.
