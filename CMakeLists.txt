cmake_minimum_required(VERSION 3.16)
# SDL2_ttf vendors FreeType with an old cmake_minimum_required; satisfy CMake 4.x policy checks on subprojects.
if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.31")
  set(CMAKE_POLICY_VERSION_MINIMUM "3.5" CACHE STRING "" FORCE)
endif()

project(PuzzlesAndGames C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option(PG_BUILD_TESTS "Build unit test executables" ON)

include(FetchContent)

# Emscripten ships SDL ports (-sUSE_SDL=2, -sUSE_SDL_TTF=2); avoid FetchContent SDL here so CMake does not force
# shared SDL2_ttf on a platform with no shared libraries (breaks TARGET_SONAME_FILE_NAME generator expressions).
if(EMSCRIPTEN)
  if(NOT TARGET SDL2_emscripten_port)
    add_library(SDL2_emscripten_port INTERFACE)
    target_compile_options(SDL2_emscripten_port INTERFACE "SHELL:-sUSE_SDL=2")
    target_link_options(SDL2_emscripten_port INTERFACE "SHELL:-sUSE_SDL=2")
    add_library(SDL2::SDL2 ALIAS SDL2_emscripten_port)
  endif()
  if(NOT TARGET SDL2_emscripten_main_stub)
    add_library(SDL2_emscripten_main_stub INTERFACE)
    add_library(SDL2::SDL2main ALIAS SDL2_emscripten_main_stub)
  endif()
  if(NOT TARGET SDL2_ttf_emscripten_port)
    add_library(SDL2_ttf_emscripten_port INTERFACE)
    target_compile_options(SDL2_ttf_emscripten_port INTERFACE "SHELL:-sUSE_SDL_TTF=2")
    target_link_options(SDL2_ttf_emscripten_port INTERFACE "SHELL:-sUSE_SDL_TTF=2")
    add_library(SDL2_ttf::SDL2_ttf ALIAS SDL2_ttf_emscripten_port)
  endif()
else()
  set(SDL2_DISABLE_INSTALL ON CACHE BOOL "" FORCE)
  set(SDL2_DISABLE_UNINSTALL ON CACHE BOOL "" FORCE)
  set(SDL_SHARED ON CACHE BOOL "" FORCE)

  set(PG_SDL2_ARCHIVE "${CMAKE_SOURCE_DIR}/third_party/SDL2-2.30.10.tar.gz")
  get_filename_component(PG_SDL2_ABS "${PG_SDL2_ARCHIVE}" ABSOLUTE)
  if(EXISTS "${PG_SDL2_ABS}")
    # Prefer a local archive: reliable on Windows and avoids TLS trust issues in some bundled curl setups.
    set(PG_SDL2_ROOT "${CMAKE_BINARY_DIR}/_deps/SDL2-2.30.10")
    if(NOT EXISTS "${PG_SDL2_ROOT}/CMakeLists.txt")
      file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/_deps")
      execute_process(
        COMMAND "${CMAKE_COMMAND}" -E tar xf "${PG_SDL2_ABS}"
        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/_deps"
        RESULT_VARIABLE PG_SDL2_EXTRACT_RESULT
      )
      if(NOT PG_SDL2_EXTRACT_RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to extract SDL2 archive: ${PG_SDL2_ABS}")
      endif()
    endif()
    add_subdirectory("${PG_SDL2_ROOT}" "${CMAKE_BINARY_DIR}/_deps/sdl2-build")
  else()
    FetchContent_Declare(
      SDL2
      URL https://github.com/libsdl-org/SDL/releases/download/release-2.30.10/SDL2-2.30.10.tar.gz
      DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(SDL2)
  endif()
endif()

if(NOT EMSCRIPTEN)
  set(SDL2TTF_VENDORED ON CACHE BOOL "" FORCE)
  set(SDL2TTF_SAMPLES OFF CACHE BOOL "" FORCE)
  set(SDL2TTF_INSTALL OFF CACHE BOOL "" FORCE)
  FetchContent_Declare(
    SDL2_ttf
    URL https://github.com/libsdl-org/SDL_ttf/releases/download/release-2.22.0/SDL2_ttf-2.22.0.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  )
  FetchContent_MakeAvailable(SDL2_ttf)
endif()

add_library(pg_core STATIC
  src/core/pg_digits.c
  src/core/pg_text.c
)
target_include_directories(pg_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(pg_core PUBLIC SDL2::SDL2 SDL2_ttf::SDL2_ttf)
if(EMSCRIPTEN)
  target_compile_definitions(
    pg_core
    PRIVATE
    "PG_ASSET_ROOT=\"\""
  )
else()
  target_compile_definitions(
    pg_core
    PRIVATE
    "PG_ASSET_ROOT=\"${CMAKE_SOURCE_DIR}\""
  )
endif()

add_library(pg_shell STATIC
  src/shell/pg_app.c
  src/shell/pg_game.c
)
target_include_directories(pg_shell PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(pg_shell PUBLIC SDL2::SDL2 SDL2::SDL2main pg_core)

add_library(pg_gamepack STATIC
  games/catalog/pg_catalog.c
  games/g2048/g2048_board.c
  games/g2048/g2048_game.c
  games/mastermind/mastermind_logic.c
  games/mastermind/mastermind_game.c
  games/letterlock/letterlock_puzzle.c
  games/letterlock/letterlock_glyphs.c
  games/letterlock/letterlock_game.c
  games/tictactoe/tictactoe_board.c
  games/tictactoe/tictactoe_game.c
  games/slitherlink/slitherlink_board.c
  games/slitherlink/slitherlink_game.c
)
target_include_directories(pg_gamepack PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
  "${CMAKE_CURRENT_SOURCE_DIR}/games/g2048"
  "${CMAKE_CURRENT_SOURCE_DIR}/games/mastermind"
  "${CMAKE_CURRENT_SOURCE_DIR}/games/catalog"
  "${CMAKE_CURRENT_SOURCE_DIR}/games/letterlock"
  "${CMAKE_CURRENT_SOURCE_DIR}/games/tictactoe"
  "${CMAKE_CURRENT_SOURCE_DIR}/games/slitherlink")
target_link_libraries(pg_gamepack PUBLIC pg_shell)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  target_link_libraries(pg_gamepack PUBLIC m)
endif()

add_executable(puzzlesgames src/main.c)
target_include_directories(puzzlesgames PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
  "${CMAKE_CURRENT_SOURCE_DIR}/games/g2048"
  "${CMAKE_CURRENT_SOURCE_DIR}/games/mastermind"
  "${CMAKE_CURRENT_SOURCE_DIR}/games/catalog"
  "${CMAKE_CURRENT_SOURCE_DIR}/games/letterlock"
  "${CMAKE_CURRENT_SOURCE_DIR}/games/tictactoe"
  "${CMAKE_CURRENT_SOURCE_DIR}/games/slitherlink")
target_link_libraries(puzzlesgames PRIVATE pg_gamepack SDL2::SDL2 SDL2::SDL2main)

if(EMSCRIPTEN)
  set_target_properties(puzzlesgames PROPERTIES SUFFIX ".html")
  target_link_options(puzzlesgames PRIVATE
    "SHELL:-sMIN_WEBGL_VERSION=2"
    "SHELL:-sMAX_WEBGL_VERSION=2"
    "SHELL:-sALLOW_MEMORY_GROWTH=1"
    "SHELL:--embed-file;${CMAKE_SOURCE_DIR}/assets/fonts/SourceSans3-Regular.ttf@/assets/fonts/SourceSans3-Regular.ttf"
    "SHELL:--shell-file;${CMAKE_CURRENT_SOURCE_DIR}/web/shell_minimal.html")
endif()

if(WIN32)
  target_link_libraries(puzzlesgames PRIVATE shell32)
endif()

if(PG_BUILD_TESTS)
  enable_testing()
  add_executable(test_g2048_board tests/test_g2048_board.c games/g2048/g2048_board.c)
  target_include_directories(test_g2048_board PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/games/g2048")
  add_test(NAME g2048_board COMMAND test_g2048_board)

  add_executable(test_mastermind_logic tests/test_mastermind_logic.c games/mastermind/mastermind_logic.c)
  target_include_directories(test_mastermind_logic PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/games/mastermind")
  add_test(NAME mastermind_logic COMMAND test_mastermind_logic)

  add_executable(test_letterlock_puzzle tests/test_letterlock_puzzle.c games/letterlock/letterlock_puzzle.c)
  target_include_directories(test_letterlock_puzzle PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/games/letterlock")
  add_test(NAME letterlock_puzzle COMMAND test_letterlock_puzzle)

  add_executable(test_tictactoe_board tests/test_tictactoe_board.c games/tictactoe/tictactoe_board.c)
  target_include_directories(test_tictactoe_board PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/games/tictactoe")
  add_test(NAME tictactoe_board COMMAND test_tictactoe_board)

  add_executable(test_slitherlink_board tests/test_slitherlink_board.c games/slitherlink/slitherlink_board.c)
  target_include_directories(test_slitherlink_board PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/games/slitherlink")
  add_test(NAME slitherlink_board COMMAND test_slitherlink_board)
endif()
