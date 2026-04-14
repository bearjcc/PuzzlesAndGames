# Product requirements: offline puzzles and games hub

## Document control

| Field | Value |
|-------|-------|
| Product | PuzzlesAndGames (working title) |
| Status | Draft |
| Stack | C/C++, lightweight native graphics (see [PLAN.md](PLAN.md)) |

---

## Product vision

Ship a **single-player, fully local** puzzle and games hub that a privacy-focused user can **audit, build from source, and run without any network capability**. Positioning is **utility over spectacle**: fast launch, clear rules, stable saves, minimal chrome. The project draws inspiration from [Simon Tatham's Portable Puzzle Collection](https://www.chiark.greenend.org.uk/~sgtatham/puzzles/): reuse **permissively licensed** logic where it helps, implement a clear shell and integration layer in this repository.

---

## Goals

- **Trust**: no telemetry, ads, or third-party SDKs; **no outbound network** in shipped builds (enforcement approach documented in [PLAN.md](PLAN.md)).
- **Cross-platform desktop**: Windows, Linux, macOS as tier 1; mobile optional later.
- **Bundled assets**: fonts, icons, help text, and native dependencies ship **inside the release artifact** (portable folder or package-managed layout); no CDN or runtime downloads.
- **Open source**: permissive license for the hub (MIT or Apache-2.0) unless a bundled component forces GPL (e.g. some chess engines); document boundaries.
- **Modular games**: shared shell (launcher, settings, save format, input) with per-game modules behind a stable interface.
- **Distribution**: ship as **portable artifacts** (for example zip or tarball) or via **system package managers**; **no first-party installer** that assumes elevated privileges.

## Non-goals (initial)

- Multiplayer, accounts, cloud sync, leaderboards, storefronts.
- Anti-cheat or server-side validation.
- Marketing or engagement fluff (ads, trackers, optional analytics).
- **Dedicated installers** (MSI, Setup.exe, macOS pkg that require admin by default) and **in-app self-update** clients we maintain ourselves.

---

## Personas

- **Paranoid power user**: reads dependency list and build flags; may build from source; wants a small, explainable binary.
- **Casual offline player**: wants a **simple catalog** and friction-free setup (**portable folder** or **package manager** install); **no admin rights**; no ads.

## Trust mechanisms

- **Supply chain**: vendored or pinned third-party sources; minimal dependencies; reviewed licenses; optional SBOM.
- **Build**: documented toolchain; CI policy against accidental networking (see plan).
- **Runtime**: documented **no sockets** policy for release builds.
- **Attribution**: `third_party/` with SPDX identifiers and upstream LICENSE files.

---

## Functional requirements (MVP)

1. **Catalog**: grid or list of games with search, favorites, last-played.
2. **Per game**: new game, restart, undo/redo where applicable; hints configurable; in-app rules (local HTML or Markdown).
3. **Persistence**: save/load per title; predictable user data location; optional export/import.
4. **Accessibility**: scalable UI, keyboard navigation; color-safe palettes where relevant.
5. **Determinism**: seeded randomness where applicable; **copy seed** and serialized state for bug reports.

### Stretch

- Theming (light/dark), high contrast, localization hooks.

---

## Non-functional requirements

| Area | Requirement |
|------|-------------|
| Privacy | Zero network I/O in release builds; no crash reporting that phones home unless explicitly opted in (default off). |
| Security | C/C++ baseline: sanitizers in CI where feasible, static analysis, fuzzing of save/seed parsers, narrow APIs to third-party C cores. |
| Performance | Define cold-start and frame-time budgets per platform; 60 FPS 2D target for grid games. |
| Maintainability | One module interface per game; shared types for input and timing. |
| Licensing | SPDX per component; isolate GPL engines if used. |
| Distribution | **No installer** as a product requirement: prefer portable bundles and OS or third-party **package managers** (for example **winget**, **apt**, **Homebrew**). **No administrator elevation** for normal install and play. |
| Updates | Any **automatic** or low-friction updates are **out of scope for in-app code**; users get new versions through **package manager updates** or by replacing a portable folder. |

---

## Content scope (examples)

Initial target genres (subject to licensing and effort):

- Grid and logic: Sudoku, Kakuro, Minesweeper, Slitherlink-style, 2048-style.
- Board: chess, checkers (engine and license choices in [PLAN.md](PLAN.md)).
- **Simon Tatham**-style puzzles: integrate via upstream **MIT** C backends where practical (verify `LICENCE` on each import).

Legal note: puzzle **rules** are not a substitute for **clear OSS licensing** on code you ship. Prefer explicit licenses and attribution.

---

## Roadmap (product phases)

1. **Foundation**: repository layout, build, CI, dependency policy, stub shell plus one stub game.
2. **Vertical slice**: one complete grid game with saves and rules.
3. **Board games**: chess or checkers behind the module API; GPL boundary decided in advance.
4. **STP spike**: one upstream puzzle wired through a native frontend; measure size and maintenance.
5. **Hardening**: a11y, performance, reproducible builds, license aggregation.

---

## Open decisions

- Graphics: SDL2 vs Raylib (vs minimal GLFW 2D).
- All-C vs C++ shell with C game cores.
- Chess: strong GPL engine vs weaker permissive engine.
- STP: subset in v1 vs broad parity over time.
- Mobile: in or out of v1.

---

## References

- [Simon Tatham's Portable Puzzle Collection](https://www.chiark.greenend.org.uk/~sgtatham/puzzles/) (upstream; MIT per site and source `LICENCE`).
- Engineering and stack details: [PLAN.md](PLAN.md).
