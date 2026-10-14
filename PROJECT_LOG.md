# Project Log — cubbies

Reverse-chronological operational logbook (most recent first). It complements `git log` with the
human context: what was done, why, and by which PR.

---

## 2026-10-14

### First UML study drafted (branch `docs/uml-study`, PR pending owner validation)

- Eight Mermaid diagrams under `docs/architecture/diagrams/`: `cube-core` (use case by domain,
  component lib-vs-adapters with eight ports, mood engine state, sleep/wake state), `identity`
  (boot resolves identity, adoption ritual entered by the held-at-boot gesture),
  `social-presence` (presence sequence with greeting policy, frame data-flow with the 8-byte v1
  PRESENCE layout).
- Every block rendered with `mmdc` without a parse error. Three critical review passes
  (architecture guardian) before the `review-local` gate. First pass: ten Must items (MAC in the
  payload in one diagram and not the other, quiet hours with no time source, audio policy and
  gesture timers in adapters against the lib-purity rule, ports not matching the sequence calls,
  a system event modelled as a use case, shake needing the tier-B IMU). Second pass: four Must
  (wake-up melody owned by three diagrams, a Sleep/Mood circular dependency, a Router → Identity
  edge the ritual never uses, a storage edge with nothing stored) and a greeting policy that
  could skip a newcomer. All fixed. Third pass: no Must left, two stale use-case numbers from the renumbering and
  the "greet while awake" wording fixed. `review-local` gate (design, docs, security,
  correctness): four Must (receive-path rate budget and length check in the callback, spoofing
  risk stated honestly, one stale reference), all applied; report in
  `docs/architecture/review-local.md`.
- Design choices submitted as scored options (Q&A, two rounds with deeper analysis) and
  **validated by the owner**: quiet hours out of V1 (no time source; silence comes from sleep,
  then from the light sensor in S8), discrete moods, wake restarts in Curious, greeting as a
  local rate-limited stimulus (GREET frame re-evaluated with G3), palette index on the air with
  a version bump on palette change (RGB565 revisited in V1.5), ritual entered only by the
  held-at-boot gesture, derived MAC colour not persisted, constant default volume until a UC6
  gesture is chosen, unauthenticated ESP-NOW broadcast accepted as residual risk. Consequences
  applied: `CLAUDE.md` "never loud" rule reworded, roadmap S4/S5/S8 reworded.
- Follow-up: a superseding ADR recording the accepted ESP-NOW residual risk and the V2 key hook,
  in its own PR. Double tap cycling backwards in the ritual stays open until the palette size.
- Next: PR for this branch, then backlog items S1–S7 and G1 become issues with these diagrams as
  their contract.

### Project kick-off: scoped Q&A session, decisions recorded, repository scaffolded

- Owner's intent: a fleet of small cube desk companions (OLED eyes, mic, buzzer) that interact
  with each other, offered to office colleagues on an "adoption day", one colour per cube,
  3D-printed enclosures. Prototypes first (2–3 cubes), then more than 10.
- Desk research on the existing landscape (Dasai Mochi clones on ESP32-C3, RoboEyes / OliEyes
  eye libraries, ESP-NOW vs BLE for peer discovery, PlatformIO vs pioarduino core status).
  Finding: solo desk companions are well covered; the multi-cube social layer is the
  differentiator.
- Nine structuring decisions taken through scored Q&A and recorded as ADR-0001..0009:
  concept (solo companion + proximity social layer), power roadmap (USB-C → LiPo → Qi), board
  (XIAO ESP32-C3), stack (PlatformIO + pioarduino, Arduino core 3.3.x), radio (ESP-NOW
  broadcast), display (SSD1331 OLED, ST7735 LCD compared on prototypes), sensor tiers under an
  11-GPIO budget, identity (MAC default + adoption ritual in NVS), licensing (MIT code,
  CC BY-SA 4.0 enclosure).
- Repository scaffolded: README, BOM, roadmap with prioritised backlog, ADRs, architecture
  conventions, PlatformIO skeleton with two envs (XIAO C3, C3 Super Mini) and a native test.
  Private GitHub repository `benoit-bremaud/cubbies` created and `main` pushed.
- Next: UML study (use cases, mood state machine, ESP-NOW presence sequence, component view)
  by PR, before any firmware; order prototype parts (BOM).
