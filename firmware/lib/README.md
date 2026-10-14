# firmware/lib

Pure behaviour logic, one PlatformIO library per domain, **no Arduino includes**: moods, sleep,
identity rules, the social protocol state machine, frame encoding. Time and randomness are
injected (a clock and an RNG interface), never read from `millis()` or `random()` here, so every
behaviour replays identically in the native tests under `../test/`.

Adapters (display, audio, touch, radio, sensors) live in `../src/` and are the only code allowed
to include Arduino or ESP-IDF headers.
