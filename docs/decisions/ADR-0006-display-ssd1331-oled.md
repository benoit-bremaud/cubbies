# ADR-0006: Display — 0.95" SSD1331 colour OLED, with an ST7735 LCD compared on prototypes

**Status:** Accepted — 2026-10-14

## Context

The face must show eyes in the cube's colour, readable at arm's length, inside a ~40 mm cube,
on 4 SPI pins. Candidates: 0.95" SSD1331 OLED 96×64 (~6 €, true blacks, 4-wire SPI), 1.5"
SSD1351 OLED 128×128 (~12–15 €, needs a 50 mm+ cube), 0.96" ST7735 IPS LCD 160×80 (~3 €, grey
backlit black, one more pin for the backlight), 1.3" ST7789 IPS 240×240 (~5 €, same backlight
issue, bigger). There is no 0.96" SSD1331; "0.96 colour" modules use other drivers.

## Options and scoring

Criteria: readable eyes with deep blacks (3), price ×N (2), pins (2), availability and libraries
(2), size for a cube (2).

| Criterion (weight) | SSD1331 0.95" | SSD1351 1.5" | ST7735 0.96" | ST7789 1.3" |
|---|---|---|---|---|
| Eyes, blacks (3) | 3 | 3 | 2 | 2 |
| Price ×N (2) | 2 | 1 | 3 | 3 |
| Pins (2) | 3 | 3 | 2 | 2 |
| Availability (2) | 2 | 2 | 3 | 3 |
| Cube size (2) | 3 | 1 | 3 | 2 |
| **Total /33** | **29** | 22 | 28 | 26 |

## Decision

**0.95" SSD1331 OLED** is the series target. The scores of the OLED and the ST7735 LCD are too
close to call on paper, so the prototype batch includes one ST7735 for a side-by-side test of the
eyes (blacks, brightness, viewing angle, price). The series display is confirmed by a follow-up
ADR after that test.

## Consequences

- The eye renderer targets a 96×64 16-bit framebuffer; a 160×80 LCD would reuse it with a
  different panel driver, so the comparison costs no behaviour code.
- OLED burn-in: static eyes must drift slightly and the screen dims during sleep.
