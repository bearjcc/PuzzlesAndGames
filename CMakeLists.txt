cmake_minimum_required(VERSION 3.16)
project(PuzzlesAndGames C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option(PG_BUILD_TESTS "Build unit test executables" ON)

include(FetchContent)

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

add_library(pg_core STATIC
  src/core/pg_digits.c
)
target_include_directories(pg_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(pg_core PUBLIC SDL2::SDL2)

add_library(pg_shell STATIC
  src/shell/pg_app.c
  src/shell/pg_game.c
)
target_include_directories(pg_shell PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(pg_shell PUBLIC SDL2::SDL2 SDL2::SDL2main pg_core)

add_library(pg_g2048 STATIC
  games/g2048/g2048_board.c
  games/g2048/g2048_game.c
)
target_include_directories(pg_g2048 PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
  "${CMAKE_CURRENT_SOURCE_DIR}/games/g2048")
target_link_libraries(pg_g2048 PUBLIC pg_shell)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  target_link_libraries(pg_g2048 PUBLIC m)
endif()

add_library(pg_letterlock STATIC
  games/letterlock/letterlock_puzzle.c
  games/letterlock/letterlock_glyphs.c
  games/letterlock/letterlock_game.c
)
target_include_directories(pg_letterlock PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
  "${CMAKE_CURRENT_SOURCE_DIR}/games/letterlock")
target_link_libraries(pg_letterlock PUBLIC pg_shell)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  target_link_libraries(pg_letterlock PUBLIC m)
endif()

find_package(SDL2_ttf REQUIRED)

set(PG_STP_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/puzzles")
add_library(pg_stp STATIC
  games/stp/stp_sdl_game.c
  "${PG_STP_DIR}/blackbox.c"
  "${PG_STP_DIR}/bridges.c"
  "${PG_STP_DIR}/combi.c"
  "${PG_STP_DIR}/cube.c"
  "${PG_STP_DIR}/divvy.c"
  "${PG_STP_DIR}/dominosa.c"
  "${PG_STP_DIR}/drawing.c"
  "${PG_STP_DIR}/dsf.c"
  "${PG_STP_DIR}/fifteen.c"
  "${PG_STP_DIR}/filling.c"
  "${PG_STP_DIR}/flip.c"
  "${PG_STP_DIR}/flood.c"
  "${PG_STP_DIR}/galaxies.c"
  "${PG_STP_DIR}/grid.c"
  "${PG_STP_DIR}/guess.c"
  "${PG_STP_DIR}/inertia.c"
  "${PG_STP_DIR}/keen.c"
  "${PG_STP_DIR}/latin.c"
  "${PG_STP_DIR}/laydomino.c"
  "${PG_STP_DIR}/lightup.c"
  "${PG_STP_DIR}/list.c"
  "${PG_STP_DIR}/loopgen.c"
  "${PG_STP_DIR}/loopy.c"
  "${PG_STP_DIR}/magnets.c"
  "${PG_STP_DIR}/malloc.c"
  "${PG_STP_DIR}/map.c"
  "${PG_STP_DIR}/maxflow.c"
  "${PG_STP_DIR}/midend.c"
  "${PG_STP_DIR}/mines.c"
  "${PG_STP_DIR}/misc.c"
  "${PG_STP_DIR}/net.c"
  "${PG_STP_DIR}/netslide.c"
  "${PG_STP_DIR}/pattern.c"
  "${PG_STP_DIR}/printing.c"
  "${PG_STP_DIR}/ps.c"
  "${PG_STP_DIR}/pearl.c"
  "${PG_STP_DIR}/pegs.c"
  "${PG_STP_DIR}/penrose.c"
  "${PG_STP_DIR}/random.c"
  "${PG_STP_DIR}/range.c"
  "${PG_STP_DIR}/rect.c"
  "${PG_STP_DIR}/samegame.c"
  "${PG_STP_DIR}/signpost.c"
  "${PG_STP_DIR}/singles.c"
  "${PG_STP_DIR}/sixteen.c"
  "${PG_STP_DIR}/slant.c"
  "${PG_STP_DIR}/solo.c"
  "${PG_STP_DIR}/tdq.c"
  "${PG_STP_DIR}/tents.c"
  "${PG_STP_DIR}/towers.c"
  "${PG_STP_DIR}/tracks.c"
  "${PG_STP_DIR}/tree234.c"
  "${PG_STP_DIR}/twiddle.c"
  "${PG_STP_DIR}/undead.c"
  "${PG_STP_DIR}/unequal.c"
  "${PG_STP_DIR}/unruly.c"
  "${PG_STP_DIR}/untangle.c"
  "${PG_STP_DIR}/version.c"
)
target_include_directories(pg_stp PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
  "${CMAKE_CURRENT_SOURCE_DIR}/games/stp"
  "${PG_STP_DIR}")
target_compile_definitions(pg_stp PRIVATE COMBINED)
target_link_libraries(pg_stp PUBLIC pg_shell SDL2::SDL2 SDL2_ttf::SDL2_ttf)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  target_link_libraries(pg_stp PUBLIC m)
endif()

add_executable(puzzlesgames src/main.c)
target_include_directories(puzzlesgames PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
  "${CMAKE_CURRENT_SOURCE_DIR}/games/g2048"
  "${CMAKE_CURRENT_SOURCE_DIR}/games/letterlock"
  "${CMAKE_CURRENT_SOURCE_DIR}/games/stp")
target_link_libraries(puzzlesgames PRIVATE pg_g2048 pg_letterlock pg_stp SDL2::SDL2 SDL2::SDL2main)

add_custom_command(TARGET puzzlesgames POST_BUILD
  COMMAND "${CMAKE_COMMAND}" -E make_directory "$<TARGET_FILE_DIR:puzzlesgames>/assets/fonts"
  COMMAND "${CMAKE_COMMAND}" -E copy_if_different
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts/SourceSans3-wght.ttf"
    "$<TARGET_FILE_DIR:puzzlesgames>/assets/fonts/SourceSans3-wght.ttf"
  COMMAND "${CMAKE_COMMAND}" -E copy_if_different
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts/SourceSans3-OFL.txt"
    "$<TARGET_FILE_DIR:puzzlesgames>/assets/fonts/SourceSans3-OFL.txt"
  COMMENT "Copy bundled STP UI font next to puzzlesgames"
)

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

  add_executable(test_letterlock_puzzle tests/test_letterlock_puzzle.c games/letterlock/letterlock_puzzle.c)
  target_include_directories(test_letterlock_puzzle PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/games/letterlock")
  add_test(NAME letterlock_puzzle COMMAND test_letterlock_puzzle)
endif()
