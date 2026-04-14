#include "pg/game.h"

void pg_game_init(PgGame *game, const PgGameVtable *vt, void *state)
{
  game->vt = vt;
  game->state = state;
}

void pg_game_destroy(PgGame *game)
{
  if (game->vt != NULL && game->state != NULL) {
    game->vt->destroy(game->state);
  }
  game->vt = NULL;
  game->state = NULL;
}
