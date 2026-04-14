#include "mastermind_game.h"
#include "pg/app.h"

int main(int argc, char **argv)
{
  (void)argc;
  (void)argv;

  PgApp app;
  if (!pg_app_init(&app, "PuzzlesAndGames - Mastermind", 720, 820, mastermind_game_vt())) {
    return 1;
  }
  pg_app_run(&app);
  pg_app_shutdown(&app);
  return 0;
}
