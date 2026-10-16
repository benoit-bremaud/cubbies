# Bill of materials

Prices are indicative (AliExpress / Seeed, October 2026) and must be confirmed at order time.
Tiers follow ADR-0007: P0 is the minimum living cube, each later tier is demonstrable on its own.

## Per cube, V1 (USB-C powered)

| Tier | Part | Interface | Pins | Qty | Unit price |
|------|------|-----------|------|-----|-----------|
| P0 | Seeed XIAO ESP32-C3 | USB-C | — | 1 | ~6 € |
| P0 | 0.95" colour OLED SSD1331 96×64 (4-wire SPI) | SPI | 4 (SCK, MOSI, CS, DC; RESET tied to 3V3 through RC) | 1 | ~6 € |
| P0 | TTP223 capacitive touch module | GPIO | 1 | 1 | ~0.5 € |
| P0 | Passive buzzer 3 V, 16 Ω, rated 40 mA | PWM | 1 | 1 | ~0.3 € |
| P1 | INMP441 I2S MEMS microphone | I2S | 3 (SCK, WS, SD; L/R to GND) | 1 | ~2 € |
| P2 | MPU6050 IMU (or LSM6DS3) | I2C | 2 (shared bus) | 1 | ~1.5 € |
| P3 | BH1750 ambient light sensor | I2C | 0 (shared bus) | 1 | ~1 € |
| P0 | I2C expansion connector JST-SH 4-pin (Qwiic/STEMMA QT pinout, issue #18) | I2C | 0 (shared bus) | 1 | ~0.3 € |
| — | USB-C cable 1 m | — | — | 1 | ~1.5 € |
| P0 | Buzzer stage: AO3400, 1N4148, 82 Ω, 100 Ω, 100 kΩ; display RESET RC 10 kΩ + 1 µF X7R + 1N4148; SDA boot pull-up 47 kΩ (ADR-0015) | — | — | 1 set | ~0.3 € |
| — | Hook-up wire, heat-shrink, foam pads | — | — | — | ~1 € |
| — | PLA filament for the enclosure | — | — | ~40 g | ~1 € |
| | **Total per cube** | | **11 GPIO** | | **~20 €** |

GPIO budget: 4 (SPI) + 3 (I2S) + 1 (buzzer) + 1 (touch) + 2 (I2C) = 11, which is every GPIO the
XIAO exposes on its headers. Any further sensor must be an I2C device (ADR-0007).

## Prototype batch (2–3 cubes)

| Part | Qty | Note |
|------|-----|------|
| Everything in the per-cube list | 3 | one full cube per prototype |
| ESP32-C3 Super Mini | 2 | same chip as the XIAO, cheaper, own env in `platformio.ini`; no charger |
| 0.96" IPS LCD ST7735 160×80 (SPI) | 1 | side-by-side comparison with the OLED (ADR-0006) |
| 10-port USB power strip | 1 | adoption day power |

## Deferred to later versions

| Version | Part | Note |
|---------|------|------|
| V2 | LiPo 3.7 V 800–1000 mAh with protection circuit | charged by the XIAO's onboard charger (~370 mA) |
| V2 | Slide switch | battery cut-off |
| V2 | Vibration motor (optional, 1 GPIO) | purring; needs a pin freed or a GPIO expander |
| V3 | Qi wireless receiver module, 5 V output | fed into the XIAO's 5 V input; office desks have Qi pads |
| V1.5 | Nothing extra | the "mother cube" is a regular cube with Wi-Fi credentials in NVS |
| V4 (if ever) | XIAO ESP32-S3, MAX98357A I2S amplifier, small speaker | real voice; the C3 has a single I2S peripheral shared with the mic |

## Rejected parts and why

- ESP32-C6 boards: Arduino core 3.x only through a community fork, onboard charger limited to
  100 mA, no need for Zigbee/Thread (ADR-0003).
- 1.5" SSD1351 128×128 OLED: twice the price and forces a 50 mm+ cube (ADR-0006).
- Photoresistor on an ADC pin: no pin left; replaced by the I2C BH1750 (ADR-0007).
- Addressable RGB LED: one more pin for a colour the eyes and the enclosure already carry.
