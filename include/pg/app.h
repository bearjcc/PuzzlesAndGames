#ifndef PG_APP_H
#define PG_APP_H

#include "pg/game.h"

#include <SDL.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct PgApp {
  SDL_Window *window;
  SDL_Renderer *renderer;
  PgGame game;
  int window_w;
  int window_h;
  bool running;
} PgApp;

#define PG_APP_WINDOW_DATA_KEY "pg_app"

bool pg_app_init(PgApp *app, const char *title, int width, int height, const PgGameVtable *game_vt);
PgApp *pg_app_from_renderer(SDL_Renderer *renderer);
bool pg_app_replace_game(PgApp *app, const PgGameVtable *game_vt);
void pg_app_shutdown(PgApp *app);
void pg_app_run(PgApp *app);

#ifdef __cplusplus
}
#endif

#endif
