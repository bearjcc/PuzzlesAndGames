#include "g2048_game.h"
#include "mastermind_game.h"
#include "pg/app.h"
#include "pg/catalog/pg_catalog.h"

#include <string.h>

int main(int argc, char **argv)
{
  const PgGameVtable *vt = pg_catalog_game_vt();
  const char *title = "PuzzlesAndGames";

  if (argc >= 2) {
    if (strcmp(argv[1], "2048") == 0 || strcmp(argv[1], "g2048") == 0) {
      vt = g2048_game_vt();
      title = "PuzzlesAndGames - 2048";
    } else if (strcmp(argv[1], "mastermind") == 0 || strcmp(argv[1], "mm") == 0) {
      vt = mastermind_game_vt();
      title = "PuzzlesAndGames - Mastermind";
    }
  }

  PgApp app;
  if (!pg_app_init(&app, title, 720, 820, vt)) {
    return 1;
  }
  pg_app_run(&app);
  pg_app_shutdown(&app);
  return 0;
}
