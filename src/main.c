#include "slitherlink_game.h"

#include "pg/catalog/pg_catalog.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_usage(const char *argv0)
{
  fprintf(
      stderr,
      "Usage: %s [game] [options]\n"
      "  game: (default catalog) | g2048 | mastermind | letterlock | tictactoe | slitherlink\n"
      "  slitherlink options: --w N --h N --rect|--blob --easy|--normal|--hard --seed U32\n"
      "  Keys (slitherlink): click edges, N new puzzle, C clear lines, R same as N\n",
      argv0);
}

static int parse_uint(const char *s, unsigned int *out)
{
  char *end = NULL;
  unsigned long v = strtoul(s, &end, 10);
  if (end == s || *end != '\0' || v > 0xFFFFFFFFu) {
    return -1;
  }
  *out = (unsigned int)v;
  return 0;
}

int main(int argc, char **argv)
{
  const PgGameVtable *vt = pg_catalog_game_vt();
  const char *title = "PuzzlesAndGames";
  int win_w = 720;
  int win_h = 820;

  int sl_w = 8;
  int sl_h = 8;
  SlShape sl_shape = SL_SHAPE_RECT;
  SlDifficulty sl_diff = SL_DIFF_NORMAL;
  unsigned int sl_seed = 0u;

  if (argc >= 2) {
    if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
      print_usage(argv[0]);
      return 0;
    }
    const PgCatalogGameSpec *spec = pg_catalog_find_game_by_cli_name(argv[1]);
    if (spec != NULL) {
      vt = spec->vt_fn();
      title = spec->window_title;
      win_w = spec->default_win_w;
      win_h = spec->default_win_h;
    } else {
      fprintf(stderr, "Unknown game: %s\n", argv[1]);
      print_usage(argv[0]);
      return 2;
    }
    if (vt == slitherlink_game_vt()) {
      for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--rect") == 0) {
          sl_shape = SL_SHAPE_RECT;
        } else if (strcmp(argv[i], "--blob") == 0) {
          sl_shape = SL_SHAPE_BLOB;
        } else if (strcmp(argv[i], "--easy") == 0) {
          sl_diff = SL_DIFF_EASY;
        } else if (strcmp(argv[i], "--normal") == 0) {
          sl_diff = SL_DIFF_NORMAL;
        } else if (strcmp(argv[i], "--hard") == 0) {
          sl_diff = SL_DIFF_HARD;
        } else if (strcmp(argv[i], "--w") == 0 && i + 1 < argc) {
          unsigned int u = 0;
          if (parse_uint(argv[++i], &u) == 0) {
            sl_w = (int)u;
          }
        } else if (strcmp(argv[i], "--h") == 0 && i + 1 < argc) {
          unsigned int u = 0;
          if (parse_uint(argv[++i], &u) == 0) {
            sl_h = (int)u;
          }
        } else if (strcmp(argv[i], "--seed") == 0 && i + 1 < argc) {
          (void)parse_uint(argv[++i], &sl_seed);
        } else {
          fprintf(stderr, "Unknown argument: %s\n", argv[i]);
          print_usage(argv[0]);
          return 2;
        }
      }
    }
  }

  PgApp app;
  if (!pg_app_init(&app, title, win_w, win_h, vt)) {
    return 1;
  }

  if (vt == slitherlink_game_vt()) {
    uint32_t seed32 = sl_seed == 0u ? 1u : (uint32_t)sl_seed;
    slitherlink_game_configure(app.game.state, sl_w, sl_h, sl_shape, sl_diff, seed32);
    vt->resize(app.game.state, app.window_w, app.window_h);
  }

  pg_app_run(&app);
  pg_app_shutdown(&app);
  return 0;
}
