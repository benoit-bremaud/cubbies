# ADR-0007: Sensor tiers under an 11-GPIO budget

**Status:** Accepted — 2026-10-14

## Context

The XIAO ESP32-C3 exposes 11 GPIO. The owner's wish list (colour OLED, buzzer, capacitive
touch, I2S microphone, IMU, light sensor) costs 12 pins with a photoresistor on an ADC input:
SPI display 4 (RESET tied to 3V3 through an RC), I2S mic 3, buzzer 1, TTP223 1, I2C 2, LDR 1.
Everything that can sit on the I2C bus costs no pin: a BH1750 light sensor replaces the LDR, and
distance (VL53L0X), temperature and other extras follow the same rule. The owner asked for clear
priorities and for every behaviour to be implemented crescendo.

## Decision

Tiers, each demonstrable on its own, in implementation order:

| Tier | Parts | Pins | Enables |
|------|-------|------|---------|
| P0 | SSD1331 OLED, passive buzzer, TTP223 touch | 4 + 1 + 1 | eyes, moods, sleep, sounds, petting, identity |
| P1 | INMP441 I2S microphone | 3 | wake on noise, voice level, claps; hearing other cubes |
| P2 | MPU6050 (or LSM6DS3) on I2C | 2 (bus) | tilt-following eyes, shake, bump, upside-down sleep |
| P3 | I2C extras: BH1750 light, VL53L0X distance, temperature | 0 | sleeps with the lights, watches who approaches |
| Out of V1 | RGB LED (1 pin; colour is already carried by eyes and enclosure), vibration motor (V2) | — | — |

Total: **11 GPIO, the hard budget.** Any new peripheral is an I2C device or replaces one.

## Consequences

- The V1 wiring is final from the first cube: all 11 pins are routed, the I2C extras plug in
  later without re-soldering.
- The pin map is a single header (`firmware/include/pins.h`) with one block per board env.
- A GPIO expander on I2C is the escape hatch if a future version needs more digital outputs.
