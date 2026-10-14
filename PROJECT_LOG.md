# Project Log — cubbies

Reverse-chronological operational logbook (most recent first). It complements `git log` with the
human context: what was done, why, and by which PR.

---

## 2026-10-14

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
