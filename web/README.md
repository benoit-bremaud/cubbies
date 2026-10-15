# web — simulator and configurator

Reserved for the browser application decided on 2026-10-15 (ADR-0015, to be written before any
code lands here):

- `firmware/lib/cubbies_core` compiled to WebAssembly (Emscripten, `extern "C"` bridge), drawn in
  a canvas at the OLED's 96×64 resolution. The pixels are the cube's pixels: no second renderer.
- First scope: simulator (moods, reactions, identity frames, colour, slow motion, PNG export) plus
  eye-rhythm sliders; the character editor follows the `Personality` structure of the firmware.
- Stack: Vite + TypeScript + Vitest, static site on GitHub Pages, no backend, no secrets.

Dependency rule: `web/` depends on `firmware/lib/`, never the reverse.
