# PuzzlesAndGames

Offline-first, cross-platform **puzzles and single-player games**. No ads, no telemetry, **no network** in shipped builds. Built for users who want a small, auditable native app and developers who want a **C/C++** codebase with **bundled** dependencies.

## Docs

| File | Purpose |
|------|---------|
| [PRD.md](PRD.md) | Product vision, requirements, personas, roadmap |
| [PLAN.md](PLAN.md) | Engineering stack, architecture, CI guardrails, checklist |

## Principles

- **Trust**: ship what you can explain; minimize dependencies; document licenses.
- **Portable**: Windows, Linux, macOS as first-class targets.
- **Reuse**: integrate permissively licensed upstreams (for example Simon Tatham-style MIT C puzzle cores) with clear `third_party/` attribution.
- **Configurable**: STP-style **defaults** plus a **Configure** path for dimensions, counts, and rule knobs where the game supports it; **presets** stay first-class so casual play stays one click away.

## Build

Requirements: **CMake 3.16+**, a **C11** compiler, and **Internet once** on the first configure so CMake can download the SDL2 source archive (then everything is built locally).

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build --output-on-failure
```

Run the app from `build/` (exact binary name depends on the generator; on Windows MSVC it is typically `build/Release/puzzlesgames.exe`).

SDL2 is pulled via `FetchContent` into your build tree (for example `build/_deps/sdl2-src`); it is not committed to this repository.

## Current games

### Tic Tac Toe (`games/tictactoe`)

- **Run**: `./build/puzzlesgames tictactoe` (after building).
- **Play**: you are **X**, the computer is **O**. Click an empty square to move.
- **AI**: **Easy** (random legal moves), **Medium** (mix of random and optimal), **Impossible** (minimax, cannot lose). Switch with the on-screen buttons or keys **1**, **2**, **3**.
- **New game**: **R**, or **Enter** / **Space** after a finished game.

### 2048 (`games/g2048`)

- **Product direction**: independent **board width and height** (and other rule knobs) are a first-class goal; see [PRD.md](PRD.md) requirement on per-game configuration. Until the shell exposes a full Configure flow, play may use a fixed default grid.
- **Controls**: arrow keys or **WASD** to slide, **R** for a fresh game (new seed).
- **Game over**: dim overlay with final score; press **Enter** or **Space** to restart.
- **Animations**: short eased slide for tiles; a small merge pulse on combined cells.

## Status

First vertical slice in place: **SDL2 shell** (`src/shell`, `include/pg`) and a **2048** module behind a small game vtable. Next steps are in [PLAN.md](PLAN.md) (catalog UI, more games, STP spike).

## License

TBD (intent: permissive for the hub; see PRD for GPL boundaries if bundling strong chess engines).
