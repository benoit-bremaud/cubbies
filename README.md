# cubbies

Small cube-shaped desk companions with colour OLED eyes, a microphone and a buzzer, built on
the ESP32-C3. Each cube lives on its own on a desk (moods, sleep, reactions to touch and sound)
and gains group behaviours when other cubes are in radio range: greetings, contagious moods,
synchronised choirs, chain reactions. Designed as a gift for a team: one cube per colleague,
one colour per cube, handed out on an "adoption day".

**Status:** conception. Hardware, stack and scope are decided (see
[docs/decisions/](docs/decisions/README.md)); the UML study comes next, then the firmware.

## What a cube is (V1)

| Tier | Part | Role |
|------|------|------|
| P0 | Seeed XIAO ESP32-C3 | MCU, Wi-Fi/BLE radio, USB-C, LiPo charger for V2 |
| P0 | 0.95" colour OLED, SSD1331, 96×64, SPI | the face: procedurally animated eyes in the cube's colour |
| P0 | Passive buzzer | the voice: expressive beeps, melodies, adjustable volume |
| P0 | TTP223 capacitive touch pad | petting: tap, double tap, long stroke |
| P1 | INMP441 I2S microphone | hears noise, voice, claps, other cubes |
| P2 | MPU6050 IMU (I2C) | tilt-following eyes, shake, bump |
| P3 | BH1750 light sensor and other I2C extras | sleeps when the office lights go out |

Power: USB-C on the desk (V1), LiPo battery (V2), wireless charging pad (V3).
Radio between cubes: ESP-NOW broadcast, no pairing, no router, no app.

Full bill of materials: [docs/bom.md](docs/bom.md). Roadmap and prioritised backlog:
[docs/roadmap.md](docs/roadmap.md).

## Repository layout

```
docs/bom.md               bill of materials per cube and for the prototypes
docs/roadmap.md           V1 → V3 and the prioritised behaviour backlog
docs/decisions/           ADRs (Nygard format), one per structuring decision
docs/architecture/        design dossier and UML study (Mermaid), see its README
docs/electronics/         electronics spec, calculations, measurements (to come)
docs/mechanics/           mechanical spec, assembly guide
firmware/                 PlatformIO project (pioarduino platform, Arduino core 3.3.x)
electronics/              KiCad schematic and ngspice simulations, to come
mechanics/                3D-printable enclosure (CC BY-SA 4.0), to come
web/                      browser simulator and configurator (WebAssembly build of the firmware lib), to come
okh.yml                   Open Know-How manifest (what this project is, under which licences)
PROJECT_LOG.md            operational logbook
```

## Licence

Firmware and documentation: [MIT](LICENSE). Enclosure files under `mechanics/`:
[CC BY-SA 4.0](mechanics/LICENSE.md).
