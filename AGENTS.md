# AGENTS.md

## Cursor Cloud specific instructions

### Overview

PuzzlesAndGames is an offline-first, native C11 desktop app (SDL2 graphics) with a 2048 game module. No services, databases, or network dependencies. See `README.md` for build commands.

### Build and test (quick reference)

```bash
CC=gcc CXX=g++ cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j$(nproc)
ctest --test-dir build --output-on-failure
```

### Non-obvious caveats

- **Compiler**: The default system compiler (`cc`/`c++`) is Clang 18 which fails to link SDL2 due to a missing `-lstdc++` search path. Always configure with `CC=gcc CXX=g++` explicitly.
- **X11 headers**: SDL2 built from source via `FetchContent` requires `libxext-dev` and related X11 dev packages (`libx11-dev`, `libxrandr-dev`, `libxcursor-dev`, `libxi-dev`, `libxss-dev`). These are installed by the update script.
- **Running the GUI app**: The app needs a display. Use `DISPLAY=:1` for the VNC desktop available in the Cloud Agent VM, or `Xvfb :99` for headless testing.
- **Unit tests are headless**: `test_g2048_board` tests pure board logic with no SDL2 runtime dependency; runs fine without a display.
- **SDL2 download**: First `cmake` configure fetches SDL2 2.30.10 source from GitHub via `FetchContent`. This requires internet access once; subsequent builds reuse the cached source in `build/_deps/`.
- **No lint tool currently**: There is no configured linter (cppcheck, clang-tidy, etc.) in the project yet. Static analysis is a planned CI guardrail per `PLAN.md`.
