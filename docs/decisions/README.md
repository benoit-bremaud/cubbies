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
| [0008](ADR-0008-identity-mac-default-and-adoption-ritual.md) | Identity: MAC-derived default, adoption ritual stored in NVS | Accepted |
| [0009](ADR-0009-licensing-mit-code-cc-by-sa-enclosure.md) | Licensing: MIT code, CC BY-SA 4.0 enclosure, private first | Accepted |
| [0010](ADR-0010-enclosure-freecad-scripted-python.md) | Enclosure: FreeCAD driven by a versioned Python script, MCP for inspection | Accepted |
| [0011](ADR-0011-design-dossier-and-verification-plan.md) | Design dossier structure, verification plan and gates | Accepted |
| [0012](ADR-0012-esp-now-unauthenticated-broadcast-accepted-residual-risk.md) | Security: unauthenticated ESP-NOW broadcast accepted as a bounded residual risk in V1 (amends 0005) | Accepted |
