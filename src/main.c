#include "g2048_game.h"
#include "letterlock_game.h"
#include "pg/app.h"

#include <string.h>

int main(int argc, char **argv)
{
  const PgGameVtable *vt = g2048_game_vt();
  const char *title = "PuzzlesAndGames - 2048";
  int win_w = 720;
  int win_h = 820;

  if (argc >= 2 && strcmp(argv[1], "letterlock") == 0) {
    vt = letterlock_game_vt();
    title = "PuzzlesAndGames - Letterlock";
    win_w = 780;
    win_h = 620;
  }

  PgApp app;
  if (!pg_app_init(&app, title, win_w, win_h, vt)) {
    return 1;
  }
  pg_app_run(&app);
  pg_app_shutdown(&app);
  return 0;
}
