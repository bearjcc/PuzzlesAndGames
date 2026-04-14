#include "pg/catalog/pg_catalog.h"

#include "g2048_game.h"
#include "mastermind_game.h"
#include "pg/app.h"
#include "pg/digits.h"

#include <SDL.h>

#include <stdio.h>
#include <string.h>

typedef struct PgCatalogState {
  SDL_Renderer *renderer;
  int win_w;
  int win_h;
  int card_y;
  int card_h;
  int card1_x;
  int card2_x;
  int card_w;
  int gap;
} PgCatalogState;

static void catalog_layout(PgCatalogState *s, int w, int h)
{
  s->win_w = w;
  s->win_h = h;
  s->gap = 24;
  s->card_y = 120;
  s->card_h = h - s->card_y - 80;
  if (s->card_h < 200) {
    s->card_h = 200;
  }
  int usable = w - s->gap * 3;
  if (usable < 200) {
    usable = 200;
  }
  s->card_w = (usable - s->gap) / 2;
  s->card1_x = s->gap;
  s->card2_x = s->gap * 2 + s->card_w;
}

static bool catalog_hit_card(const PgCatalogState *s, float x, float y, int *which)
{
  *which = -1;
  if (y < (float)s->card_y || y > (float)(s->card_y + s->card_h)) {
    return false;
  }
  if (x >= (float)s->card1_x && x < (float)(s->card1_x + s->card_w)) {
    *which = 0;
    return true;
  }
  if (x >= (float)s->card2_x && x < (float)(s->card2_x + s->card_w)) {
    *which = 1;
    return true;
  }
  return false;
}

static void catalog_start_game(PgCatalogState *s, const PgGameVtable *vt)
{
  PgApp *app = pg_app_from_renderer(s->renderer);
  if (app == NULL) {
    return;
  }
  const char *title = "PuzzlesAndGames";
  if (vt->id != NULL && strcmp(vt->id, "g2048") == 0) {
    title = "PuzzlesAndGames - 2048";
  } else if (vt->id != NULL && strcmp(vt->id, "mastermind") == 0) {
    title = "PuzzlesAndGames - Mastermind";
  }
  SDL_SetWindowTitle(app->window, title);
  if (!pg_app_replace_game(app, vt)) {
    fprintf(stderr, "catalog: failed to start game %s\n", vt->id != NULL ? vt->id : "?");
    app->running = false;
  }
}

static void *catalog_create(SDL_Renderer *renderer)
{
  PgCatalogState *s = (PgCatalogState *)SDL_calloc(1, sizeof(PgCatalogState));
  if (s == NULL) {
    return NULL;
  }
  s->renderer = renderer;
  int w = 0;
  int h = 0;
  SDL_GetRendererOutputSize(renderer, &w, &h);
  catalog_layout(s, w, h);
  return s;
}

static void catalog_destroy(void *state)
{
  SDL_free(state);
}

static void catalog_reset(void *state)
{
  (void)state;
}

static void catalog_mouse_to_logical(SDL_Renderer *r, int wx, int wy, float *lx, float *ly)
{
  if (r == NULL) {
    *lx = (float)wx;
    *ly = (float)wy;
    return;
  }
  SDL_RenderWindowToLogical(r, (float)wx, (float)wy, lx, ly);
}

static void catalog_on_event(void *state, const SDL_Event *event)
{
  PgCatalogState *s = (PgCatalogState *)state;
  if (event->type == SDL_QUIT) {
    PgApp *app = pg_app_from_renderer(s->renderer);
    if (app != NULL) {
      app->running = false;
    }
    return;
  }
  if (event->type == SDL_MOUSEBUTTONDOWN && event->button.button == SDL_BUTTON_LEFT) {
    float lx;
    float ly;
    catalog_mouse_to_logical(s->renderer, event->button.x, event->button.y, &lx, &ly);
    int which = -1;
    if (catalog_hit_card(s, lx, ly, &which)) {
      if (which == 0) {
        catalog_start_game(s, g2048_game_vt());
      } else {
        catalog_start_game(s, mastermind_game_vt());
      }
    }
    return;
  }
  if (event->type != SDL_KEYDOWN) {
    return;
  }
  const SDL_Keycode k = event->key.keysym.sym;
  if (k == SDLK_ESCAPE) {
    PgApp *app = pg_app_from_renderer(s->renderer);
    if (app != NULL) {
      app->running = false;
    }
    return;
  }
  if (k == SDLK_1 || k == SDLK_KP_1) {
    catalog_start_game(s, g2048_game_vt());
  } else if (k == SDLK_2 || k == SDLK_KP_2) {
    catalog_start_game(s, mastermind_game_vt());
  }
}

static void catalog_update(void *state, float dt_seconds)
{
  (void)state;
  (void)dt_seconds;
}

static void catalog_draw_card(SDL_Renderer *r, const SDL_FRect *rect, SDL_Color fill, SDL_Color border)
{
  SDL_SetRenderDrawColor(r, fill.r, fill.g, fill.b, fill.a);
  SDL_RenderFillRectF(r, rect);
  SDL_SetRenderDrawColor(r, border.r, border.g, border.b, border.a);
  SDL_RenderDrawRectF(r, rect);
}

static void catalog_render(void *state, SDL_Renderer *renderer)
{
  PgCatalogState *s = (PgCatalogState *)state;
  SDL_SetRenderDrawColor(renderer, 0x2A, 0x28, 0x26, 255);
  SDL_RenderClear(renderer);

  SDL_FRect a = {(float)s->card1_x, (float)s->card_y, (float)s->card_w, (float)s->card_h};
  SDL_FRect b = {(float)s->card2_x, (float)s->card_y, (float)s->card_w, (float)s->card_h};
  catalog_draw_card(renderer, &a, (SDL_Color){0x5C, 0x56, 0x4F, 255}, (SDL_Color){0xD0, 0xC8, 0xBC, 255});
  catalog_draw_card(renderer, &b, (SDL_Color){0x4A, 0x52, 0x5C, 255}, (SDL_Color){0xD0, 0xC8, 0xBC, 255});

  /* Large digits 1 and 2 as card labels (bitmap font from pg_digits). */
  SDL_Color ink = {0xFA, 0xF6, 0xF0, 255};
  float digit_px = 8.0f;
  float dw = 6.0f * digit_px;
  pg_digits_draw_uint(renderer, a.x + (a.w - dw) * 0.5f, a.y + (a.h - 7.0f * digit_px) * 0.5f, digit_px, 1u, ink);
  pg_digits_draw_uint(renderer, b.x + (b.w - dw) * 0.5f, b.y + (b.h - 7.0f * digit_px) * 0.5f, digit_px, 2u, ink);
}

static void catalog_resize(void *state, int width_px, int height_px)
{
  PgCatalogState *s = (PgCatalogState *)state;
  catalog_layout(s, width_px, height_px);
}

static const PgGameVtable kCatalogVt = {
    .id = "catalog",
    .create = catalog_create,
    .destroy = catalog_destroy,
    .reset = catalog_reset,
    .on_event = catalog_on_event,
    .update = catalog_update,
    .render = catalog_render,
    .resize = catalog_resize,
};

const PgGameVtable *pg_catalog_game_vt(void)
{
  return &kCatalogVt;
}
