#include "pg/catalog/pg_catalog.h"

#include "g2048_game.h"
#include "letterlock_game.h"
#include "mastermind_game.h"
#include "slitherlink_game.h"
#include "tictactoe_game.h"

#include "pg/sdl.h"
#include "pg/text.h"
#include "pg/theme.h"

#include <SDL.h>

#include <math.h>
#include <stdio.h>
#include <string.h>

/* #region Game table (CLI + menu + window title) */

static const char *kCliG2048[] = {"2048", "g2048", NULL};
static const char *kCliMastermind[] = {"mastermind", "mm", NULL};
static const char *kCliLetterlock[] = {"letterlock", NULL};
static const char *kCliTictactoe[] = {"tictactoe", NULL};
static const char *kCliSlitherlink[] = {"slitherlink", "loopy", NULL};

static const PgCatalogGameSpec kCatalogGames[] = {
    {kCliG2048, g2048_game_vt, "PuzzlesAndGames - 2048", "2048", 720, 820},
    {kCliMastermind, mastermind_game_vt, "PuzzlesAndGames - Mastermind", "Mastermind", 720, 820},
    {kCliLetterlock, letterlock_game_vt, "PuzzlesAndGames - Letterlock", "Letterlock", 780, 620},
    {kCliTictactoe, tictactoe_game_vt, "PuzzlesAndGames - Tic Tac Toe", "Tic-tac-toe", 640, 720},
    {kCliSlitherlink, slitherlink_game_vt, "PuzzlesAndGames - Slitherlink", "Slitherlink", 900, 820},
};

enum { PG_CATALOG_NUM_GAMES = (int)(sizeof(kCatalogGames) / sizeof(kCatalogGames[0])) };

/* #endregion */

typedef struct PgCatalogState {
  PgTextSession text_session;
  SDL_Renderer *renderer;
  int win_w;
  int win_h;
  int margin;
  int gap;
  int cols;
  int rows;
  int header_block_h;
  int footer_h;
  SDL_FRect tiles[PG_CATALOG_NUM_GAMES];
  int hover_slot;
} PgCatalogState;

int pg_catalog_game_count(void)
{
  return PG_CATALOG_NUM_GAMES;
}

const PgCatalogGameSpec *pg_catalog_game_spec(int index)
{
  if (index < 0 || index >= PG_CATALOG_NUM_GAMES) {
    return NULL;
  }
  return &kCatalogGames[index];
}

const PgCatalogGameSpec *pg_catalog_find_game_by_cli_name(const char *name)
{
  if (name == NULL) {
    return NULL;
  }
  for (int g = 0; g < PG_CATALOG_NUM_GAMES; g++) {
    for (const char *const *p = kCatalogGames[g].cli_names; p != NULL && *p != NULL; p++) {
      if (strcmp(name, *p) == 0) {
        return &kCatalogGames[g];
      }
    }
  }
  return NULL;
}

bool pg_catalog_launch(PgApp *app)
{
  if (app == NULL || app->window == NULL) {
    return false;
  }
  SDL_SetWindowTitle(app->window, "PuzzlesAndGames");
  if (!pg_app_replace_game(app, pg_catalog_game_vt())) {
    app->running = false;
    return false;
  }
  return true;
}

bool pg_catalog_launch_from_renderer(SDL_Renderer *renderer)
{
  PgApp *app = pg_app_from_renderer(renderer);
  if (app == NULL) {
    return false;
  }
  return pg_catalog_launch(app);
}

static int catalog_pick_cols(int usable_w, int gap)
{
  const int min_tile = 152;
  if (usable_w >= min_tile * 3 + gap * 2) {
    return 3;
  }
  if (usable_w >= min_tile * 2 + gap) {
    return 2;
  }
  return 1;
}

static void catalog_layout(PgCatalogState *s, int w, int h)
{
  s->win_w = w;
  s->win_h = h;
  s->margin = 24;
  s->gap = 16;
  s->header_block_h = 72;
  s->footer_h = 36;
  s->hover_slot = -1;

  int usable_w = w - s->margin * 2;
  if (usable_w < 100) {
    usable_w = 100;
  }
  s->cols = catalog_pick_cols(usable_w, s->gap);
  s->rows = (PG_CATALOG_NUM_GAMES + s->cols - 1) / s->cols;

  float tile_w = ((float)usable_w - (float)(s->cols - 1) * (float)s->gap) / (float)s->cols;
  if (tile_w < 120.0f) {
    tile_w = 120.0f;
  }

  int body_top = s->margin + s->header_block_h;
  int body_h = h - body_top - s->margin - s->footer_h;
  if (body_h < 120) {
    body_h = 120;
  }

  float tile_h = ((float)body_h - (float)(s->rows - 1) * (float)s->gap) / (float)s->rows;
  if (tile_h < 100.0f) {
    tile_h = 100.0f;
  }

  float grid_w = (float)s->cols * tile_w + (float)(s->cols - 1) * (float)s->gap;
  float x0 = ((float)w - grid_w) * 0.5f;
  if (x0 < (float)s->margin) {
    x0 = (float)s->margin;
  }

  for (int i = 0; i < PG_CATALOG_NUM_GAMES; i++) {
    int c = i % s->cols;
    int r = i / s->cols;
    s->tiles[i].x = x0 + (float)c * (tile_w + (float)s->gap);
    s->tiles[i].y = (float)body_top + (float)r * (tile_h + (float)s->gap);
    s->tiles[i].w = tile_w;
    s->tiles[i].h = tile_h;
  }
}

static bool catalog_point_in_tile(const PgCatalogState *s, float x, float y, int *out_slot)
{
  for (int i = 0; i < PG_CATALOG_NUM_GAMES; i++) {
    const SDL_FRect *rc = &s->tiles[i];
    if (x >= rc->x && x < rc->x + rc->w && y >= rc->y && y < rc->y + rc->h) {
      *out_slot = i;
      return true;
    }
  }
  *out_slot = -1;
  return false;
}

static void catalog_start_game_at(PgCatalogState *st, int game_index)
{
  if (game_index < 0 || game_index >= PG_CATALOG_NUM_GAMES) {
    return;
  }
  const PgCatalogGameSpec *spec = &kCatalogGames[game_index];
  PgApp *app = pg_app_from_renderer(st->renderer);
  if (app == NULL) {
    return;
  }
  const PgGameVtable *vt = spec->vt_fn();
  SDL_SetWindowTitle(app->window, spec->window_title);
  if (!pg_app_replace_game(app, vt)) {
    fprintf(stderr, "catalog: failed to start game %s\n", vt->id != NULL ? vt->id : "?");
    app->running = false;
  }
}

static int catalog_digit_key_slot(SDL_Keycode k)
{
  if (k == SDLK_1 || k == SDLK_KP_1) {
    return 0;
  }
  if (k == SDLK_2 || k == SDLK_KP_2) {
    return 1;
  }
  if (k == SDLK_3 || k == SDLK_KP_3) {
    return 2;
  }
  if (k == SDLK_4 || k == SDLK_KP_4) {
    return 3;
  }
  if (k == SDLK_5 || k == SDLK_KP_5) {
    return 4;
  }
  if (k == SDLK_6 || k == SDLK_KP_6) {
    return 5;
  }
  if (k == SDLK_7 || k == SDLK_KP_7) {
    return 6;
  }
  if (k == SDLK_8 || k == SDLK_KP_8) {
    return 7;
  }
  if (k == SDLK_9 || k == SDLK_KP_9) {
    return 8;
  }
  return -1;
}

static void *catalog_create(SDL_Renderer *renderer)
{
  PgCatalogState *s = (PgCatalogState *)SDL_calloc(1, sizeof(PgCatalogState));
  if (s == NULL) {
    return NULL;
  }
  pg_text_session_begin(&s->text_session);
  if (!s->text_session.active) {
    SDL_free(s);
    return NULL;
  }
  s->renderer = renderer;
  s->hover_slot = -1;
  int w = 0;
  int h = 0;
  (void)pg_sdl_renderer_output_size(renderer, &w, &h);
  catalog_layout(s, w, h);
  return s;
}

static void catalog_destroy(void *state)
{
  PgCatalogState *s = (PgCatalogState *)state;
  if (s == NULL) {
    return;
  }
  pg_text_session_end(&s->text_session);
  SDL_free(s);
}

static void catalog_reset(void *state)
{
  (void)state;
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
  if (event->type == SDL_MOUSEMOTION) {
    float lx;
    float ly;
    if (pg_sdl_event_pointer_logical(s->renderer, event, &lx, &ly)) {
      int slot = -1;
      if (catalog_point_in_tile(s, lx, ly, &slot)) {
        s->hover_slot = slot;
      } else {
        s->hover_slot = -1;
      }
    }
    return;
  }
  if (event->type == SDL_MOUSEBUTTONDOWN && event->button.button == SDL_BUTTON_LEFT) {
    float lx;
    float ly;
    if (pg_sdl_event_pointer_logical(s->renderer, event, &lx, &ly)) {
      int slot = -1;
      if (catalog_point_in_tile(s, lx, ly, &slot) && slot >= 0 && slot < PG_CATALOG_NUM_GAMES) {
        catalog_start_game_at(s, slot);
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
  int n = catalog_digit_key_slot(k);
  if (n >= 0 && n < PG_CATALOG_NUM_GAMES) {
    catalog_start_game_at(s, n);
  }
}

static void catalog_update(void *state, float dt_seconds)
{
  (void)state;
  (void)dt_seconds;
}

static void catalog_draw_tile_frame(SDL_Renderer *r, const SDL_FRect *rect, float stroke_px, SDL_Color border)
{
  SDL_FRect inner = *rect;
  float inset = stroke_px * 0.5f;
  inner.x += inset;
  inner.y += inset;
  inner.w = fmaxf(1.0f, inner.w - stroke_px);
  inner.h = fmaxf(1.0f, inner.h - stroke_px);
  SDL_SetRenderDrawColor(r, border.r, border.g, border.b, border.a);
  SDL_RenderDrawRectF(r, &inner);
}

static void catalog_render(void *state, SDL_Renderer *renderer)
{
  PgCatalogState *s = (PgCatalogState *)state;
  pg_theme_clear_paper(renderer);

  SDL_Color ink = PG_COLOR_INK;
  SDL_Color muted = PG_COLOR_INK_MUTED;
  SDL_FRect title_box;
  title_box.x = (float)s->margin;
  title_box.y = (float)s->margin;
  title_box.w = (float)(s->win_w - s->margin * 2);
  title_box.h = 40.0f;
  pg_text_draw_utf8_centered(renderer, &title_box, "Choose a game", ink);

  SDL_FRect sub_box = title_box;
  sub_box.y += 36.0f;
  sub_box.h = 28.0f;
  pg_text_draw_utf8_centered(renderer, &sub_box, "Click a tile or press a number key", muted);

  for (int i = 0; i < PG_CATALOG_NUM_GAMES; i++) {
    const SDL_FRect *rc = &s->tiles[i];
    bool hover = (i == s->hover_slot);
    SDL_Color fill = hover ? PG_COLOR_SURFACE_STRONG : PG_COLOR_SURFACE;
    SDL_Color border = hover ? PG_COLOR_INK_MUTED : PG_COLOR_RULE;
    SDL_SetRenderDrawColor(renderer, fill.r, fill.g, fill.b, fill.a);
    SDL_RenderFillRectF(renderer, rc);
    float stroke = pg_theme_stroke_px(fminf(rc->w, rc->h), hover ? 0.022f : 0.014f);
    catalog_draw_tile_frame(renderer, rc, stroke, border);

    char hint[8];
    SDL_snprintf(hint, sizeof(hint), "%u", (unsigned)(i + 1));
    float key_h = fminf(22.0f, rc->h * 0.18f);
    pg_text_draw_utf8(renderer, rc->x + 10.0f, rc->y + 8.0f, key_h, hint, PG_COLOR_INK_FAINT);

    SDL_FRect label_rc = *rc;
    float pad = fmaxf(8.0f, rc->h * 0.06f);
    label_rc.x += pad;
    label_rc.y += rc->h * 0.28f;
    label_rc.w = rc->w - pad * 2.0f;
    label_rc.h = rc->h * 0.62f;
    pg_text_draw_utf8_centered(renderer, &label_rc, kCatalogGames[i].menu_label, ink);
  }

  SDL_FRect foot;
  foot.x = (float)s->margin;
  foot.y = (float)(s->win_h - s->margin - s->footer_h + 4);
  foot.w = (float)(s->win_w - s->margin * 2);
  foot.h = (float)s->footer_h;
  pg_text_draw_utf8_centered(renderer, &foot, "Esc in a game returns here", muted);
}

static void catalog_resize(void *state, int width_px, int height_px)
{
  PgCatalogState *s = (PgCatalogState *)state;
  int keep_hover = s->hover_slot;
  catalog_layout(s, width_px, height_px);
  s->hover_slot = keep_hover;
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
