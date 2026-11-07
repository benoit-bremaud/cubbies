# cubbies

Small cube-shaped desk companions with colour OLED eyes, a microphone and a buzzer, built on
the ESP32-C3. Each cube lives on its own on a desk (moods, sleep, reactions to touch and sound)
and gains group behaviours when other cubes are in radio range: greetings, contagious moods,
synchronised choirs, chain reactions. Designed as a gift for a team: one cube per colleague,
one colour per cube, handed out on an "adoption day".

**Status:** conception, heading for the hardware CDR. Hardware, stack and scope are decided
([docs/decisions/](docs/decisions/README.md)); the UML study is done
([docs/architecture/](docs/architecture/README.md)); the firmware logic is tested natively
and its adapters build for both boards; the KiCad schematic and its simulation benches pass. The CDR paper checks are
closed; the bench measurements remain ([PROJECT_LOG.md](PROJECT_LOG.md)).

## What a cube is (V1)

| Tier | Part | Role |
|------|------|------|
| P0 | Seeed XIAO ESP32-C3 | MCU, Wi-Fi/BLE radio, USB-C, 3V3 LDO (Torex XC6210), LiPo charger for V2 |
| P0 | 0.95" colour OLED, SSD1331, 96×64, SPI | the face: procedurally animated eyes in the cube's colour |
| P0 | Passive buzzer | the voice: expressive beeps, melodies, adjustable volume |
| P0 | TTP223 capacitive touch pad | petting: tap, double tap, long stroke |
| P1 | INMP441 I2S microphone | hears noise, voice, claps, other cubes |
| P2 | MPU6050 IMU (I2C) | tilt-following eyes, shake, bump |
| P3 | BH1750 light sensor and other I2C extras | sleeps when the office lights go out |
| — | Qwiic/STEMMA QT port J1 | one plug-and-play I2C module, ≤ 50 mA (ADR-0016) |

Power: USB-C on the desk (V1), LiPo battery (V2), wireless charging pad (V3).
Radio between cubes: ESP-NOW broadcast, no pairing, no router, no app.

Full bill of materials: [docs/bom.md](docs/bom.md). Roadmap and prioritised backlog:
[docs/roadmap.md](docs/roadmap.md).

## Repository layout

```
docs/bom.md               bill of materials per cube and for the prototypes
docs/roadmap.md           V1 → V3 and the prioritised behaviour backlog
docs/decisions/           ADRs (Nygard format), one per structuring decision
docs/architecture/        design dossier: UML study (Mermaid), requirements, verification plan, gates
docs/hardware/            electronics spec, calculation log, assembly guide, bench procedures
firmware/                 PlatformIO project (pioarduino platform, Arduino core 3.3.x)
hardware/electronics/     KiCad schematic + PDF, sim/ (ngspice benches, closed-form cross-check)
hardware/case/            3D-printable enclosure (CC BY-SA 4.0), to come
scripts/                  local CI: gate attestation, commit-msg check, wiring tests, penv trust
tools/                    repository-wide scripts (local SonarQube scan)
justfile, mise.toml, lefthook.yml   local CI tiers, pinned toolchain, git hooks
PROJECT_LOG.md            operational logbook
```

## Local CI

Every check runs on the laptop (ADR-0019, skill `local-ci`); the repository has no GitHub
workflow. Once per clone:

```bash
just setup
```

| Tier | Recipe | When | Holds |
|---|---|---|---|
| L0 | `just fast` | pre-commit hook | gitleaks, ruff, typos, shellcheck on the staged files; Conventional Commits subject |
| L1 | `just check` | pre-push hook | ruff check + format, typos, shellcheck, `pio test -e native`, both board builds, Semgrep (Python), gitleaks on the history, wiring tests |
| L2 | `just gate` | before every merge, on the pushed head | `check` + SonarQube with the quality-gate wait, then the signed attestation; `just merge <pr>` requires it |
| L3 | `just nightly` | systemd user timer | `check` + TruffleHog + PlatformIO and toolchain pin drift |

PlatformIO runs as `uvx --from platformio==6.2.0 pio` (pins in the `justfile`). Behind a
TLS-inspecting endpoint agent, `just build` first lets the PlatformIO penv trust the machine's
local CAs (`scripts/pio-trust-local-ca.sh`). `mise.toml` keeps the PlatformIO packages of this
project under `~/.platformio/packages-cubbies`, so another project's toolchain pin cannot
replace ours.

The self-hosted SonarQube (Community Build, `main` only) reads `sonar-project.properties`.
`tools/sonar-scan.sh` measures the Python coverage (sim scripts, then the case script inside the
FreeCAD python in check-only mode), writes `coverage.xml` and runs the scan; `just sonar` calls
it with the global analysis token and the quality-gate wait:

```bash
SONAR_TOKEN=$(cat ~/.config/sonarqube/analysis-token) sh tools/sonar-scan.sh
```

C/C++ is not analysed by the Community server; `firmware/lib` is covered by `pio test -e native`.

## Licence

Firmware and documentation: [MIT](LICENSE). Enclosure files under `hardware/case/`:
[CC BY-SA 4.0](hardware/case/LICENSE.md).
