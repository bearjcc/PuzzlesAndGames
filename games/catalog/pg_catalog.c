#include "pg/catalog/pg_catalog.h"

#include "g2048_game.h"
#include "letterlock_game.h"
#include "mastermind_game.h"
#include "slitherlink_game.h"
#include "tictactoe_game.h"

#include "pg/app.h"
#include "pg/digits.h"

#include <SDL.h>

#include <stdio.h>
#include <string.h>

#define PG_CATALOG_NUM_GAMES 5
#define PG_CATALOG_COLS 3

typedef struct PgCatalogState {
  SDL_Renderer *renderer;
  int win_w;
  int win_h;
  int margin;
  int gap;
  int header_h;
  int card_y;
  int card_w;
  int card_h;
  int row0_x[PG_CATALOG_COLS];
  int row1_x0;
} PgCatalogState;

static void catalog_layout(PgCatalogState *s, int w, int h)
{
  s->win_w = w;
  s->win_h = h;
  s->margin = 16;
  s->gap = 12;
  s->header_h = 44;
  s->card_y = s->margin + s->header_h;
  int usable_w = w - s->margin * 2 - s->gap * (PG_CATALOG_COLS - 1);
  if (usable_w < 120) {
    usable_w = 120;
  }
  s->card_w = usable_w / PG_CATALOG_COLS;
  int usable_h = h - s->card_y - s->margin - s->gap;
  if (usable_h < 200) {
    usable_h = 200;
  }
  s->card_h = usable_h / 2;
  for (int c = 0; c < PG_CATALOG_COLS; c++) {
    s->row0_x[c] = s->margin + c * (s->card_w + s->gap);
  }
  int row1_w = s->card_w * 2 + s->gap;
  s->row1_x0 = (w - row1_w) / 2;
  if (s->row1_x0 < s->margin) {
    s->row1_x0 = s->margin;
  }
}

static bool catalog_hit_slot(const PgCatalogState *s, float x, float y, int *out_index)
{
  *out_index = -1;
  float y0 = (float)s->card_y;
  float y1 = y0 + (float)s->card_h + (float)s->gap;

  if (y >= y0 && y < y0 + (float)s->card_h) {
    for (int c = 0; c < PG_CATALOG_COLS; c++) {
      float x0 = (float)s->row0_x[c];
      if (x >= x0 && x < x0 + (float)s->card_w) {
        *out_index = c;
        return *out_index < PG_CATALOG_NUM_GAMES;
      }
    }
    return false;
  }
  if (y >= y1 && y < y1 + (float)s->card_h) {
    for (int c = 0; c < 2; c++) {
      float x0 = (float)s->row1_x0 + (float)c * (float)(s->card_w + s->gap);
      if (x >= x0 && x < x0 + (float)s->card_w) {
        *out_index = PG_CATALOG_COLS + c;
        return *out_index < PG_CATALOG_NUM_GAMES;
      }
    }
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
  if (vt->id != NULL) {
    if (strcmp(vt->id, "g2048") == 0) {
      title = "PuzzlesAndGames - 2048";
    } else if (strcmp(vt->id, "mastermind") == 0) {
      title = "PuzzlesAndGames - Mastermind";
    } else if (strcmp(vt->id, "letterlock") == 0) {
      title = "PuzzlesAndGames - Letterlock";
    } else if (strcmp(vt->id, "tictactoe") == 0) {
      title = "PuzzlesAndGames - Tic Tac Toe";
    } else if (strcmp(vt->id, "slitherlink") == 0) {
      title = "PuzzlesAndGames - Slitherlink";
    }
  }
  SDL_SetWindowTitle(app->window, title);
  if (!pg_app_replace_game(app, vt)) {
    fprintf(stderr, "catalog: failed to start game %s\n", vt->id != NULL ? vt->id : "?");
    app->running = false;
  }
}

static const PgGameVtable *catalog_vt_for_slot(int slot)
{
  switch (slot) {
  case 0:
    return g2048_game_vt();
  case 1:
    return mastermind_game_vt();
  case 2:
    return letterlock_game_vt();
  case 3:
    return tictactoe_game_vt();
  case 4:
    return slitherlink_game_vt();
  default:
    return NULL;
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
    int slot = -1;
    if (catalog_hit_slot(s, lx, ly, &slot)) {
      const PgGameVtable *vt = catalog_vt_for_slot(slot);
      if (vt != NULL) {
        catalog_start_game(s, vt);
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
  int n = -1;
  if (k == SDLK_1 || k == SDLK_KP_1) {
    n = 0;
  } else if (k == SDLK_2 || k == SDLK_KP_2) {
    n = 1;
  } else if (k == SDLK_3 || k == SDLK_KP_3) {
    n = 2;
  } else if (k == SDLK_4 || k == SDLK_KP_4) {
    n = 3;
  } else if (k == SDLK_5 || k == SDLK_KP_5) {
    n = 4;
  }
  if (n >= 0) {
    const PgGameVtable *vt = catalog_vt_for_slot(n);
    if (vt != NULL) {
      catalog_start_game(s, vt);
    }
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

  SDL_Color fills[PG_CATALOG_NUM_GAMES] = {
      {0x5C, 0x56, 0x4F, 255},
      {0x4A, 0x52, 0x5C, 255},
      {0x4A, 0x5C, 0x52, 255},
      {0x5C, 0x4E, 0x4A, 255},
      {0x52, 0x4A, 0x5C, 255},
  };

  SDL_Color border = {0xD0, 0xC8, 0xBC, 255};
  SDL_Color ink = {0xFA, 0xF6, 0xF0, 255};
  float digit_px = 6.0f;
  float dw = 6.0f * digit_px;

  for (int slot = 0; slot < PG_CATALOG_NUM_GAMES; slot++) {
    SDL_FRect rc;
    if (slot < PG_CATALOG_COLS) {
      rc.x = (float)s->row0_x[slot];
      rc.y = (float)s->card_y;
    } else {
      rc.x = (float)s->row1_x0 + (float)(slot - PG_CATALOG_COLS) * (float)(s->card_w + s->gap);
      rc.y = (float)s->card_y + (float)s->card_h + (float)s->gap;
    }
    rc.w = (float)s->card_w;
    rc.h = (float)s->card_h;
    catalog_draw_card(renderer, &rc, fills[slot], border);
    pg_digits_draw_uint(
        renderer,
        rc.x + (rc.w - dw) * 0.5f,
        rc.y + (rc.h - 7.0f * digit_px) * 0.5f,
        digit_px,
        (uint32_t)(slot + 1),
        ink);
  }
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
