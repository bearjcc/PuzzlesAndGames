#ifndef PG_GAME_H
#define PG_GAME_H

#include <SDL.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct PgGame PgGame;

typedef struct PgGameVtable {
  const char *id;
  void *(*create)(SDL_Renderer *renderer);
  void (*destroy)(void *state);
  void (*reset)(void *state);
  void (*on_event)(void *state, const SDL_Event *event);
  void (*update)(void *state, float dt_seconds);
  void (*render)(void *state, SDL_Renderer *renderer);
  void (*resize)(void *state, int width_px, int height_px);
} PgGameVtable;

struct PgGame {
  const PgGameVtable *vt;
  void *state;
};

void pg_game_init(PgGame *game, const PgGameVtable *vt, void *state);
void pg_game_destroy(PgGame *game);

#ifdef __cplusplus
}
#endif

#endif
