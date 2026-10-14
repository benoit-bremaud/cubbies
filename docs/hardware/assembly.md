# Assembly guide — cubbies V1

> OSHWA-style build documentation: anyone with the BOM, the printed shells and a USB-C cable
> can build a cube. Photos are added at each step from the prototype build. Target: ≤ 45 min per
> cube after the first (REQ-NF-10). Status: skeleton, to be completed on prototype 3.

## Parts (reference designators match the schematic)

| Ref | Part | Qty |
|-----|------|-----|
| U1 | Seeed XIAO ESP32-C3 | 1 |
| DS1 | 0.95" OLED SSD1331 module | 1 |
| MK1 | INMP441 module | 1 |
| U2 | MPU6050 module | 1 |
| U3 | BH1750 module (tier P3, optional) | 1 |
| SW1 | TTP223 module + copper tape pad 15 × 15 mm | 1 |
| BZ1 | passive buzzer 12 mm | 1 |
| Q1, D1, R1–R3 | buzzer stage: AO3400 N-MOSFET, 1N4148, 68 Ω, 100 Ω, 100 kΩ | 1 set |
| R4, C1 | display RESET RC: 10 kΩ, 100 nF | 1 set |
| — | silicone wire 28 AWG, heat-shrink, foam pads, 4 feet | — |
| — | printed shells: front, rear | 1 each |

## Steps

1. **Flash first.** Connect the bare XIAO by USB-C, flash the release image
   (`pio run -e xiao_c3 -t upload`), check the serial banner. A flashed board fails fast.
2. **Prepare the modules.** Solder short wires on DS1, MK1, U2, SW1 per the pin table of
   [electronics-spec.md](electronics-spec.md). Set MK1 L/R to GND. Set SW1 to momentary, active high.
3. **Buzzer stage.** Build Q1/D1/R1–R3 on a 10 × 10 mm perfboard scrap; test with a 3.3 V
   square wave at 2 kHz on the gate.
4. **Wire to the XIAO.** Follow the contract table; one colour per bus. Test on the bench:
   eyes, tone, tap, serial log of the sound level and IMU.
5. **Front shell.** Seat DS1 in the window recess, tape the copper pad under the top face,
   wire SW1 to it.
6. **Rear shell.** XIAO on its rail with the USB-C flush; MK1 behind the side holes; BZ1 behind
   the slots; U2 flat on the floor (axis orientation marked); foam pads.
7. **Close and test.** Snap the shells; power; the cube boots into the adoption ritual. Run the
   end-of-line check: colour cycle, confirm, tap, double tap, stroke, clap, shake, sleep timer
   shortened by the test build flag.
8. **Record.** Serial number (MAC), assembly time, anomalies, in `measurements.md`.

## End-of-line checklist

- [ ] Eyes render in all palette colours
- [ ] Touch detected through the shell at every corner of the pad, and the invitation window
      after a cold boot detects a hold (TTP223 calibration, ADR-0013)
- [ ] Tone audible at the V1 constant volume; silent while asleep
- [ ] Sound level reacts to a clap at 1 m
- [ ] IMU wiring check: serial log shows tilt on the expected axis (behaviour is tier B, S9)
- [ ] Presence seen by a reference cube
