# Architecture Decision Records

Nygard format, one file per decision, `ADR-NNNN-<slug>.md`. A decision is changed only by a
superseding ADR. Every ADR records the options studied and the weighted scoring that led to the
choice, so the reasoning can be re-examined when the context changes.

| ADR | Title | Status |
|-----|-------|--------|
| [0001](ADR-0001-concept-solo-companion-with-proximity-social-layer.md) | Concept: solo companion with a proximity social layer | Accepted |
| [0002](ADR-0002-power-roadmap-usb-then-lipo-then-qi.md) | Power roadmap: USB-C, then LiPo, then Qi | Accepted |
| [0003](ADR-0003-board-xiao-esp32-c3.md) | Board: Seeed XIAO ESP32-C3 (Super Mini for prototypes) | Accepted |
| [0004](ADR-0004-stack-platformio-pioarduino-arduino-core-3.md) | Stack: PlatformIO with the pioarduino platform, Arduino core 3.3.x | Accepted |
| [0005](ADR-0005-radio-esp-now-broadcast.md) | Radio: ESP-NOW broadcast for the social layer | Accepted, amended by 0012 |
| [0006](ADR-0006-display-ssd1331-oled.md) | Display: 0.95" SSD1331 colour OLED, ST7735 LCD compared on prototypes | Accepted |
| [0007](ADR-0007-sensor-tiers-and-eleven-gpio-budget.md) | Sensor tiers under an 11-GPIO budget | Accepted |
| [0008](ADR-0008-identity-mac-default-and-adoption-ritual.md) | Identity: MAC-derived default, adoption ritual stored in NVS | Accepted, amended by 0013 |
| [0009](ADR-0009-licensing-mit-code-cc-by-sa-enclosure.md) | Licensing: MIT code, CC BY-SA 4.0 enclosure, private first | Accepted |
| [0010](ADR-0010-enclosure-freecad-scripted-python.md) | Enclosure: FreeCAD driven by a versioned Python script, MCP for inspection | Accepted |
| [0011](ADR-0011-design-dossier-and-verification-plan.md) | Design dossier structure, verification plan and gates | Accepted |
| [0012](ADR-0012-esp-now-unauthenticated-broadcast-accepted-residual-risk.md) | Security: unauthenticated ESP-NOW broadcast accepted as a bounded residual risk in V1 (amends 0005) | Accepted |
| [0013](ADR-0013-cube-core-pdr-decisions.md) | cube-core PDR: lib-owned PRNG, invitation window and ritual volume step, sleep timings, deferrals (amends 0008) | Accepted |
| [0014](ADR-0014-firmware-graphics-library-and-test-discipline.md) | Firmware: LovyanGFX for the display adapter, TDD on lib + golden images, brick plan F1–F5 | Accepted |
| [0015](ADR-0015-hardware-validation-fixes.md) | Hardware validation fixes before the CDR: buzzer/SDA swap off strapping GPIO2, display reset RC 1 µF + diode, buzzer 82 Ω | Accepted |
| [0016](ADR-0016-expansion-port.md) | Expansion port J1: module power budget 50/75 mA, internal bay behind a clipped rear cover, plug-and-play by allowlisted I2C probe | Accepted |
| [0017](ADR-0017-voice-group-sound-cues-first.md) | Voice: group sound cues on the V1 hardware first, no words, no cloud | Proposed |
