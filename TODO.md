# PuzzlesAndGames working todo

Living checklist aligned with [PRD.md](PRD.md) and [PLAN.md](PLAN.md). Lists use plain bullets so new rows can be inserted without renumbering an ordered list.

---

## Research snapshot: upstream cores (verify before vendoring)

These four targets match the product direction (offline hub, reuse where licensing fits). Treat names as pointers; confirm current `LICENSE` / SPDX and API shape in the revision you vendor.

- **Simon Tatham Portable Puzzle Collection**
  - **Shape**: upstream **monorepo** (many puzzle backends behind one porting layer), not a single installable library artifact.
  - **Language**: **C** puzzle logic and shared helpers; front ends vary by platform upstream.
  - **License**: **MIT** (confirm `LICENCE` in the tree you import).
  - **Integration notes**: prefer a **native SDL2 frontend** against the existing porting API; start with a **subset** of puzzles ([PLAN.md](PLAN.md)).
  - **Porting**: none for the core; work is **adapter + rendering + saves** in this repo.

- **Chess: Stockfish**
  - **Shape**: **source repo** plus usual **UCI binary** distribution pattern.
  - **Language**: **C++** (not C).
  - **License**: **GPL-3.0** (combined-build boundary called out in [PRD.md](PRD.md) / [PLAN.md](PLAN.md)).
  - **Integration notes**: typical hub pattern is **spawn local engine process** and speak UCI so the shell stays permissively licensed, unless you deliberately ship a GPL aggregate.
  - **Porting**: **no C port expected**; treat as **engine protocol + packaging**, or swap in a different engine.

- **Chess: micro-Max family (example permissive C path)**
  - **Shape**: small **C source** distributions (original micro-Max style sources) and derivative **repos** such as MCU-focused forks; audit the exact tarball or commit you use.
  - **Language**: **C**.
  - **License**: varies by snapshot; **mcu-max** is often described as **MIT** upstream (verify on the repo you pick). Do not assume all forks match.
  - **Integration notes**: weak play vs Stockfish but tiny; useful for **offline demos**, **size budgets**, or **license simplicity** when UCI is optional.
  - **Porting**: none if you stay on C; may still want a thin **move legality / state** layer for the hub.

- **Sudoku: libsudoku-style projects**
  - **Shape**: **Git repo** / header library style projects (for example **tcbrindle/libsudoku** exposes a **C API** but the implementation is **C++** and may pull **C++17** and **Range-V3**; **cbranch/libsudoku** is another **C++** generator or solver oriented repo).
  - **Language**: commonly **C++** with an optional **C** façade; **pure C** MIT generators exist but tend to be **small unmaintained snippets** rather than polished libraries (re-evaluate case by case).
  - **License**: project-dependent; **MIT** appears on some candidates (verify per repo).
  - **Integration notes**: if you require **C-only** object code for a module, prefer **in-repo C** generator or a dependency-light C++ static lib with a **narrow C linkage boundary** and document the exception.
  - **Porting**: possible **from C++ to C** only if you drop Range-V3-heavy paths or wrap compiled C++; **qqwing** is a known **C++** generator with **GPL** (usually a poor default for a permissive hub unless GPL boundary is explicit).

**Also in-repo today**

- **2048**
  - **Shape**: first-party **C** module under `games/g2048` ([README.md](README.md)); no external puzzle library.

**PRD examples without a single obvious “drop-in C library”**

- **Kakuro**, **Slitherlink-style**, **Minesweeper**: many are covered **inside STP** under specific puzzle programs, or are **small enough to implement in C** here; treat STP subset vs bespoke as an engineering choice after the spike.

---

## Phase foundation and trust

- Finalize **SPDX license matrix** for shell, SDL2, each game, and any GPL-adjacent engine packaging ([PLAN.md](PLAN.md) checklist).
- Harden **no-network** story for release presets and CI greps ([PLAN.md](PLAN.md)); document any justified dev-only exceptions.
- Keep **`third_party/`** convention: pristine upstream copies, license files, and review notes for anything bundled.

---

## Phase module platform and shell

- Define stable **game module contract** (identity, lifecycle, input, tick, render, serialize, rules surface) and keep **2048** as the reference implementation; include **per-game config** (presets, validation, `new_game(config)`), and require **config in saves and copy-seed** per [PRD.md](PRD.md).
- Shell: **Configure** (or equivalent) before **New Game** for titles that expose parameters; grid games should support **independent width and height** where rules allow (for example 2048).
- **Catalog shell MVP**: launcher grid or list, search, favorites, last-played ([PRD.md](PRD.md) functional requirements).
- Shared services: **save or load paths**, settings, optional export or import of saves ([PRD.md](PRD.md)).
- **Keyboard-first** navigation and scalable UI hooks; stub theming or high-contrast toggles if time allows ([PRD.md](PRD.md) stretch).

---

## Phase first vertical slice beyond 2048

- Pick **one grid or logic title** with clear rules text and seeds; ship **new game**, **restart**, **undo or redo** if applicable, and **local rules** ([PRD.md](PRD.md)).
- **Determinism**: seeded RNG, **copy seed**, and serialized state for bug reports ([PRD.md](PRD.md)).
- Tests: extend property or unit tests for board logic and **fuzz or harden** save and seed parsers ([PLAN.md](PLAN.md)).

---

## Phase Simon Tatham spike

- Time-box **one STP puzzle** end-to-end on **SDL2** using upstream **C** backend ([PLAN.md](PLAN.md)).
  - Vendor minimal slice under `third_party/` with **MIT** file intact.
  - Implement drawing and input for **one** puzzle; measure binary size and update cost.
- Decide **subset vs broad parity** for later milestones ([PRD.md](PRD.md) roadmap).

---

## Phase board games

- **Chess**
  - Decide **Stockfish subprocess (GPL binary)** vs **in-process permissive weak engine** vs **no engine** (human-human or puzzles only).
  - If UCI: **local process** lifecycle, option storage, and **license README** section.
- **Checkers**
  - Survey **C vs C++** engines and licenses; [PLAN.md](PLAN.md) prefers **maintained MIT-compatible** logic.
  - If only **C++** template libraries fit, plan **C linkage boundary** or **in-repo C** implementation.

---

## Phase hardening and release

- CI: **sanitizers** and static analysis where runners allow ([PLAN.md](PLAN.md)).
- **Portable artifacts** for Windows, Linux, macOS; document extract-and-run layout; no first-party installer as primary path ([PRD.md](PRD.md), [PLAN.md](PLAN.md)).
- Optional downstream packaging notes (**winget**, distro packages, **Homebrew**) without in-app auto-update ([PLAN.md](PLAN.md)).

---

## Parking lot and open decisions

- Graphics remain **SDL2** for v1; revisit **Raylib** only if a future title benefits ([PLAN.md](PLAN.md)).
- **All-C shell** vs **C++ shell with C cores** once catalog complexity grows ([PRD.md](PRD.md) open decisions).
- **Mobile** support explicitly deferred unless PRD changes ([PRD.md](PRD.md)).
