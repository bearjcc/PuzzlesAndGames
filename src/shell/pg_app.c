#include "pg/app.h"

#include "pg/sdl.h"
#include "pg/theme.h"

#include <SDL.h>

#include <stdio.h>

bool pg_app_init(PgApp *app, const char *title, int width, int height, const PgGameVtable *game_vt)
{
  app->window = NULL;
  app->renderer = NULL;
  app->game.vt = NULL;
  app->game.state = NULL;
  app->window_w = width;
  app->window_h = height;
  app->running = false;

  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_TIMER) != 0) {
    fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
    return false;
  }

  app->window = SDL_CreateWindow(
      title,
      SDL_WINDOWPOS_CENTERED,
      SDL_WINDOWPOS_CENTERED,
      width,
      height,
      SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
  if (app->window == NULL) {
    fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
    SDL_Quit();
    return false;
  }

  SDL_SetWindowData(app->window, PG_APP_WINDOW_DATA_KEY, app);

  app->renderer = SDL_CreateRenderer(app->window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (app->renderer == NULL) {
    app->renderer = SDL_CreateRenderer(app->window, -1, SDL_RENDERER_SOFTWARE);
  }
  if (app->renderer == NULL) {
    fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
    SDL_DestroyWindow(app->window);
    app->window = NULL;
    SDL_Quit();
    return false;
  }

  SDL_SetRenderDrawBlendMode(app->renderer, SDL_BLENDMODE_BLEND);

  void *state = game_vt->create(app->renderer);
  if (state == NULL) {
    SDL_DestroyRenderer(app->renderer);
    app->renderer = NULL;
    SDL_DestroyWindow(app->window);
    app->window = NULL;
    SDL_Quit();
    return false;
  }

  pg_game_init(&app->game, game_vt, state);
  game_vt->resize(state, width, height);
  app->running = true;
  return true;
}

PgApp *pg_app_from_renderer(SDL_Renderer *renderer)
{
  if (renderer == NULL) {
    return NULL;
  }
  SDL_Window *win = SDL_RenderGetWindow(renderer);
  if (win == NULL) {
    return NULL;
  }
  return (PgApp *)SDL_GetWindowData(win, PG_APP_WINDOW_DATA_KEY);
}

bool pg_app_replace_game(PgApp *app, const PgGameVtable *game_vt)
{
  if (app == NULL || app->renderer == NULL || game_vt == NULL) {
    return false;
  }
  pg_game_destroy(&app->game);
  void *state = game_vt->create(app->renderer);
  if (state == NULL) {
    fprintf(stderr, "pg_app_replace_game: game create failed\n");
    app->game.vt = NULL;
    app->game.state = NULL;
    return false;
  }
  pg_game_init(&app->game, game_vt, state);
  game_vt->resize(state, app->window_w, app->window_h);
  return true;
}

void pg_app_shutdown(PgApp *app)
{
  pg_game_destroy(&app->game);
  if (app->window != NULL) {
    SDL_SetWindowData(app->window, PG_APP_WINDOW_DATA_KEY, NULL);
  }
  if (app->renderer != NULL) {
    SDL_DestroyRenderer(app->renderer);
    app->renderer = NULL;
  }
  if (app->window != NULL) {
    SDL_DestroyWindow(app->window);
    app->window = NULL;
  }
  SDL_Quit();
}

void pg_app_run(PgApp *app)
{
  const Uint64 perf_freq = SDL_GetPerformanceFrequency();
  Uint64 last = SDL_GetPerformanceCounter();

  while (app->running) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
      if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
        int w = 0;
        int h = 0;
        (void)pg_sdl_renderer_output_size(app->renderer, &w, &h);
        app->window_w = w;
        app->window_h = h;
        if (app->game.vt != NULL && app->game.state != NULL) {
          app->game.vt->resize(app->game.state, w, h);
        }
      }
      if (app->game.vt != NULL && app->game.state != NULL) {
        app->game.vt->on_event(app->game.state, &e);
      }
      if (e.type == SDL_QUIT) {
        app->running = false;
      }
    }

    Uint64 now = SDL_GetPerformanceCounter();
    float dt = (float)((double)(now - last) / (double)perf_freq);
    last = now;
    if (dt > 0.1f) {
      dt = 0.1f;
    }

    if (app->game.vt != NULL && app->game.state != NULL) {
      app->game.vt->update(app->game.state, dt);
      pg_theme_clear_paper(app->renderer);
      app->game.vt->render(app->game.state, app->renderer);
      SDL_RenderPresent(app->renderer);
    }
  }
}
