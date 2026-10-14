# State of the art — desk companions with animated eyes (October 2026)

What exists, what cubbies reuses, what it adds. Checked on 2026-10-14; licences and hardware as
stated by each repository, to re-check before reusing anything.

## Products

| Product | What it is | Relevance |
|---------|------------|-----------|
| Dasai Mochi | commercial car-dashboard companion with an LCD face reacting to driving | the look everyone copies: small box, big expressive eyes, few colours |
| Emo, Eilik, Loona | commercial desk robots with screens, motors, voice | far above scope; confirm that expressive eyes carry the product |

## Open-source projects

| Project | Board | Display | Inputs | Social | Licence | Takeaway |
|---------|-------|---------|--------|--------|---------|----------|
| [FluxGarage RoboEyes](https://github.com/FluxGarage/RoboEyes) | any Arduino | SSD1306 / SH1106 mono, Adafruit GFX | — | — | GPL-3 | reference for eye geometry and moods; not linkable (licence), 1-bit only |
| [OliEyes](https://github.com/Oliray-ka/OliEyes) | Arduino, ESP32-C3 | mono OLED | serial | — | see repo | 20 emotions, procedural; same 1-bit limit |
| [hazzma/ESP32C3-Dasai-mochi-bot](https://github.com/hazzma/ESP32C3-Dasai-mochi-bot) | ESP32-C3 Super Mini | mono OLED | touch, MPU6050 | — | see repo | gesture vocabulary (tap / double / triple / long = love mode) |
| [dulnith-liyanage/desk-companion](https://github.com/dulnith-liyanage/desk-companion) | ESP32-C3 Super Mini | mono OLED | touch, USB audio from macOS | — | see repo | procedural tween engine, 19 expressions, mini-games, Pomodoro |
| [SukunDev/ESP32-Pet-Robot](https://github.com/SukunDev/ESP32-Pet-Robot) | ESP32-C3 DevKitM | SSD1306 | mic, touch, MPU6050 | — | see repo | closest input set to ours; Wokwi simulation included |
| [denuwanbogoda33/ESP-EMO](https://github.com/denuwanbogoda33/ESP-EMO) | ESP32 | OLED | Wi-Fi | server | see repo | moods + web dashboard; Wi-Fi dependence is what we avoid |
| [dzonder/mochi-esp32](https://github.com/dzonder/mochi-esp32) | ESP32 | SSD1306 | touch | server (Gemini) | see repo | buzzer chiptunes; server-driven expressions |
| [upiir/dual_lcd_robot_eyes](https://github.com/upiir/dual_lcd_robot_eyes) | ESP32-C3 Zero | Waveshare dual LCD | — | — | see repo | colour eyes on tiny LCDs; shows colour is feasible on a C3 |
| LEGO-minifig Mochi clone ([Hackster](https://www.hackster.io/news/build-a-diy-dasai-mochi-with-an-animated-lego-minifig-lcd-face-d25ebdc4af5c)) | XIAO ESP32-S3 | 0.85" 128×128 colour LCD | — | — | — | a colour face in a tiny shell |

## What is missing everywhere

- **Cube-to-cube interaction without infrastructure.** Every social feature above goes through
  Wi-Fi and a server or a phone. None uses ESP-NOW or BLE between companions.
- **Colour eyes as identity.** Colour displays appear, but no project ties the colour to the
  owner and to a group.
- **Series build documentation.** Builds are one-offs; no assembly guide for N units.
- **A design dossier.** No requirements, no verification; code first everywhere.

## What cubbies reuses and what it adds

| Reused (as ideas, not code) | Added |
|-----------------------------|-------|
| Gesture vocabulary (tap, double tap, long stroke) | ESP-NOW presence, contagion, chain, choir |
| Mood set and autoblink timing from RoboEyes | identity colour + adoption ritual |
| Procedural tweening approach from desk-companion | colour procedural eyes on SSD1331 with golden-image tests |
| Buzzer chiptune expressiveness from mochi-esp32 | a single Sound gate (silent asleep or in the dark), series assembly guide, design dossier |
