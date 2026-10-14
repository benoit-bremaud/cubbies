# Project Log — cubbies

Reverse-chronological operational logbook (most recent first). It complements `git log` with the
human context: what was done, why, and by which PR.

---

## 2026-10-14

### Design dossier scaffolded (branch `docs/design-dossier`)

- Owner decisions after scored options (ADR-0011): dossier = arc42-lite overview + functional
  requirements (NF X50-151 style) + Cockburn textual use cases + UML + traceability matrix +
  hardware specs with a signed GPIO contract + OSHWA assembly guide + PDR/CDR gates;
  verification = calculations and targeted ngspice simulations committed with plots, native
  tests, golden-image tests of the eyes through a native display adapter, bench measurements;
  Wokwi for exploration only (no SSD1331, no I2S mic in its catalogue).
- Enclosure tooling (ADR-0010): FreeCAD 1.1 driven by a versioned Python script, generated
  `.FCStd`/STEP/STL as artefacts, `freecad-mcp` for inspection; `.FCStd` is a binary zip, so the
  script is the reviewable source. KiCad follows the V-SEA method (KiUtils + `kicad-cli` ERC,
  no MCP; provisional, no ADR yet).
- Written: `specs/requirements.md` (9 service functions, 10 constraints, 15 functional and 11
  non-functional requirements), `specs/use-cases.md` (11 UCs + 3 traced system behaviours), `verification-plan.md`,
  `gates.md`, `traceability-matrix.md`, `docs/hardware/{electronics-spec, calculations,
  mechanical-spec, assembly}.md`, `docs/research/state-of-the-art.md`; roadmap gains S12 (rare
  gag, owner request, cartoon level, rate-limited) and T8–T10; CLAUDE.md non-negotiables updated.
- The first UML study (eight diagrams, separate session, branch `docs/uml-study`) is kept as a
  first draft and re-validated against the requirements at the `cube-core` PDR. The arc42
  restructure of `docs/architecture/README.md` waits for that branch to merge (same file).
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
- The accepted ESP-NOW residual risk and the V2 fleet-key hook are recorded in ADR-0012 (amends
  ADR-0005, PR #3). Double tap cycling backwards in the ritual stays open until the palette size.
- Next: PR for this branch, then backlog items S1–S7 and G1 become issues with these diagrams as
  their contract.
### Repository public, private-repository guards removed (branch `ci/remove-private-guards`)

- The repository went public after PR #1 (security CI baseline) merged.
- Removed the `if: ${{ !github.event.repository.private }}` guards from `codeql.yml`,
  `dependency-review.yml` and `scorecard.yml`; the three workflows now run for real (CodeQL on
  `actions` and `c-cpp`, dependency review on PRs, Scorecard on `main` after merge).

### ADR-0012: unauthenticated ESP-NOW broadcast recorded as an accepted residual risk (amends ADR-0005)

- The social-presence UML study (branch `docs/uml-study`, data-flow and sequence diagrams) found
  that ESP-NOW broadcast cannot be encrypted, that the source MAC is forgeable, and that V1 has
  no path to provision a key. An attacker in radio range can flood the 16-slot neighbour table,
  impersonate a cube's presence and mood byte, or trigger one greeting per cooldown.
- Four options scored (accept bounded risk / fleet key in NVS at adoption / per-peer unicast
  keys / BLE instead). Decision: accept in V1, bounded by eviction of absent entries only,
  greeting cooldown plus a global budget, allowlist and rate-bounded receive path, RAM-only and
  eyes-and-beep-only effects. ADR-0005's transport choice stands; its status line now reads
  "Amended by ADR-0012".
- V2 hook recorded: fleet key in NVS, never in source. V1.5 relayed OTA must ship a signed image,
  not a hash alone.

### Security CI baseline (branch `chore/security-ci-baseline`)

- Owner decision: the repository goes public (free Copilot review and analysis tools) only
  after the public release checklist passes; the missing item was the CI secret-detection job.
- Added the `security-ci-baseline` set, SHA-pinned, same set as the owner's other repositories:
  Gitleaks (full history, weekly), CodeQL (`actions` + `c-cpp` traced on the native host
  build), dependency review, OSSF Scorecard, Dependabot for GitHub Actions; plus the CI workflow
  (native tests, both board builds) and `.gitleaks.toml`.
- CodeQL, dependency review and Scorecard are guarded by `if: !repository.private` (code
  scanning and dependency review need a public repository or Advanced Security, and Scorecard's
  official template has no `contents: read` for a private checkout); the guards go away in the
  public-release PR. Until then only CI and Gitleaks are meaningful checks.
- Local gitleaks over history and working tree: no leaks. Pre-push `review-local` gate run:
  no Must Have; the Should Haves (private-repo guards, `cancel-in-progress` limited to pull
  requests, comment accuracy, CLAUDE.md mention of CI) applied before push.
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
