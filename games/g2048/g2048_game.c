#include "g2048_game.h"

#include "g2048_board.h"
#include "pg/app.h"
#include "pg/catalog/pg_catalog.h"
#include "pg/digits.h"

#include <SDL.h>

#include <math.h>
#include <string.h>

static const float kMoveDuration = 0.13f;
static const float kMergePulse = 0.11f;

typedef struct G2048Game {
  SDL_Renderer *renderer;
  G2048Board board;
  G2048Board pending;
  bool animating;
  float anim_t;
  int sprite_count;
  G2048AnimSprite sprites[G2048_MAX_ANIM_SPRITES];
  int layout_x;
  int layout_y;
  int board_px;
  int cell_px;
  int gap_px;
  bool pulse_active;
  float pulse_t;
  bool pulse_grid[G2048_SIZE][G2048_SIZE];
  bool game_over;
  bool show_won_banner;
} G2048Game;

static void g2048_tile_colors(int value, SDL_Color *bg, SDL_Color *fg)
{
  if (value <= 0) {
    *bg = (SDL_Color){0xCD, 0xC1, 0xB4, 255};
    *fg = (SDL_Color){0xEE, 0xE4, 0xDA, 255};
    return;
  }
  switch (value) {
  case 2:
    *bg = (SDL_Color){0xEE, 0xE4, 0xDA, 255};
    *fg = (SDL_Color){0x77, 0x6E, 0x65, 255};
    break;
  case 4:
    *bg = (SDL_Color){0xED, 0xE0, 0xC8, 255};
    *fg = (SDL_Color){0x77, 0x6E, 0x65, 255};
    break;
  case 8:
    *bg = (SDL_Color){0xF2, 0xB1, 0x79, 255};
    *fg = (SDL_Color){0xF9, 0xF6, 0xF2, 255};
    break;
  case 16:
    *bg = (SDL_Color){0xF5, 0x95, 0x63, 255};
    *fg = (SDL_Color){0xF9, 0xF6, 0xF2, 255};
    break;
  case 32:
    *bg = (SDL_Color){0xF6, 0x7C, 0x5F, 255};
    *fg = (SDL_Color){0xF9, 0xF6, 0xF2, 255};
    break;
  case 64:
    *bg = (SDL_Color){0xF6, 0x4D, 0x3F, 255};
    *fg = (SDL_Color){0xF9, 0xF6, 0xF2, 255};
    break;
  default:
    *bg = (SDL_Color){0xED, 0xCC, 0x61, 255};
    *fg = (SDL_Color){0xF9, 0xF6, 0xF2, 255};
    break;
  }
}

static void g2048_layout(G2048Game *g, int width, int height)
{
  int margin = 24;
  int header = 96;
  int usable_w = width - margin * 2;
  int usable_h = height - margin * 2 - header;
  int side = usable_w < usable_h ? usable_w : usable_h;
  if (side < 260) {
    side = 260;
  }
  g->board_px = side;
  g->gap_px = 8;
  g->cell_px = (side - g->gap_px * (G2048_SIZE + 1)) / G2048_SIZE;
  if (g->cell_px < 48) {
    g->cell_px = 48;
    g->board_px = g->cell_px * G2048_SIZE + g->gap_px * (G2048_SIZE + 1);
  }
  g->layout_x = (width - g->board_px) / 2;
  g->layout_y = header + (height - header - g->board_px - margin) / 2;
  if (g->layout_y < header) {
    g->layout_y = header;
  }
}

static void g2048_cell_rect(const G2048Game *g, float cr, float cc, SDL_FRect *out)
{
  float x = (float)g->layout_x + (float)g->gap_px + cc * ((float)g->cell_px + (float)g->gap_px);
  float y = (float)g->layout_y + (float)g->gap_px + cr * ((float)g->cell_px + (float)g->gap_px);
  out->x = x;
  out->y = y;
  out->w = (float)g->cell_px;
  out->h = (float)g->cell_px;
}

static void g2048_draw_round_fill(SDL_Renderer *r, const SDL_FRect *rect, SDL_Color color, float radius)
{
  SDL_SetRenderDrawColor(r, color.r, color.g, color.b, color.a);
  /* Rounded look without SDL_gfx: soft corners via a few inset rects is overkill; keep simple filled rect. */
  SDL_RenderFillRectF(r, rect);
  (void)radius;
}

static void g2048_draw_board_shell(SDL_Renderer *r, const G2048Game *g)
{
  SDL_FRect back;
  back.x = (float)g->layout_x;
  back.y = (float)g->layout_y;
  back.w = (float)g->board_px;
  back.h = (float)g->board_px;
  SDL_Color shell = {0xBB, 0xAD, 0xA0, 255};
  g2048_draw_round_fill(r, &back, shell, 6.0f);

  for (int row = 0; row < G2048_SIZE; row++) {
    for (int col = 0; col < G2048_SIZE; col++) {
      SDL_FRect cell;
      g2048_cell_rect(g, (float)row, (float)col, &cell);
      SDL_Color empty = {0xCE, 0xC1, 0xB5, 255};
      g2048_draw_round_fill(r, &cell, empty, 4.0f);
    }
  }
}

static void g2048_draw_tile_value(SDL_Renderer *r, const G2048Game *g, const SDL_FRect *cell, int value)
{
  if (value == 0) {
    return;
  }
  SDL_Color bg;
  SDL_Color fg;
  g2048_tile_colors(value, &bg, &fg);
  g2048_draw_round_fill(r, cell, bg, 4.0f);

  float px = (float)g->cell_px * 0.11f;
  if (px < 2.0f) {
    px = 2.0f;
  }
  if (value >= 1000) {
    px *= 0.85f;
  }
  float text_w = 6.0f * px * 4.0f;
  float x = cell->x + (cell->w - text_w) * 0.5f;
  float y = cell->y + (cell->h - 7.0f * px) * 0.5f;
  pg_digits_draw_uint(r, x, y, px, (uint32_t)value, fg);
}

static void g2048_begin_pulse_from_sprites(G2048Game *g)
{
  memset(g->pulse_grid, 0, sizeof(g->pulse_grid));
  for (int i = 0; i < g->sprite_count; i++) {
    if (g->sprites[i].merge_pulse) {
      int rr = (int)(g->sprites[i].tr + 0.5f);
      int cc = (int)(g->sprites[i].tc + 0.5f);
      if (rr >= 0 && rr < G2048_SIZE && cc >= 0 && cc < G2048_SIZE) {
        g->pulse_grid[rr][cc] = true;
      }
    }
  }
  g->pulse_active = false;
  for (int r = 0; r < G2048_SIZE; r++) {
    for (int c = 0; c < G2048_SIZE; c++) {
      if (g->pulse_grid[r][c]) {
        g->pulse_active = true;
      }
    }
  }
  g->pulse_t = 0.0f;
}

static void g2048_try_direction(G2048Game *g, G2048Dir dir)
{
  if (g->animating || g->pulse_active) {
    return;
  }
  int score_add = 0;
  bool won_here = false;
  G2048Board pending;
  int sc = 0;
  if (!g2048_board_prepare_move(&g->board, dir, &pending, g->sprites, &sc, &score_add, &won_here)) {
    return;
  }
  (void)score_add;
  (void)won_here;
  g->pending = pending;
  g->sprite_count = sc;
  g->animating = true;
  g->anim_t = 0.0f;
}

static void g2048_finish_animation(G2048Game *g)
{
  g->board = g->pending;
  g2048_begin_pulse_from_sprites(g);
  g->animating = false;
  g->sprite_count = 0;
  g->anim_t = 0.0f;
  if (g2048_board_has_empty(&g->board)) {
    g2048_board_spawn_random(&g->board);
  }
  if (!g2048_board_has_move(&g->board)) {
    g->game_over = true;
  }
  if (g->board.won) {
    g->show_won_banner = true;
  }
}

static void g2048_reset(G2048Game *g, uint32_t seed)
{
  g2048_board_reset(&g->board, seed);
  g->animating = false;
  g->pulse_active = false;
  g->sprite_count = 0;
  g->game_over = false;
  g->show_won_banner = g->board.won;
}

static void *g2048_create(SDL_Renderer *renderer)
{
  G2048Game *g = (G2048Game *)SDL_calloc(1, sizeof(G2048Game));
  if (g == NULL) {
    return NULL;
  }
  uint32_t seed = (uint32_t)SDL_GetTicks();
  g2048_reset(g, seed == 0u ? 1u : seed);
  g->renderer = renderer;
  int w = 0;
  int h = 0;
  SDL_GetRendererOutputSize(renderer, &w, &h);
  g2048_layout(g, w, h);
  return g;
}

static void g2048_destroy(void *state)
{
  SDL_free(state);
}

static void g2048_reset_cb(void *state)
{
  G2048Game *g = (G2048Game *)state;
  uint32_t seed = (uint32_t)SDL_GetTicks();
  g2048_reset(g, seed == 0u ? 1u : seed);
}

static void g2048_on_event(void *state, const SDL_Event *event)
{
  G2048Game *g = (G2048Game *)state;
  if (event->type == SDL_KEYDOWN) {
    const SDL_Keycode k = event->key.keysym.sym;
    if (k == SDLK_ESCAPE) {
      PgApp *app = pg_app_from_renderer(g->renderer);
      if (app != NULL) {
        SDL_SetWindowTitle(app->window, "PuzzlesAndGames");
        if (!pg_app_replace_game(app, pg_catalog_game_vt())) {
          app->running = false;
        }
      }
      return;
    }
    if (k == SDLK_r) {
      g2048_reset_cb(g);
      return;
    }
    if (g->game_over && (k == SDLK_RETURN || k == SDLK_SPACE)) {
      g2048_reset_cb(g);
      return;
    }
    if (k == SDLK_LEFT || k == SDLK_a) {
      g2048_try_direction(g, G2048_DIR_LEFT);
    } else if (k == SDLK_RIGHT || k == SDLK_d) {
      g2048_try_direction(g, G2048_DIR_RIGHT);
    } else if (k == SDLK_UP || k == SDLK_w) {
      g2048_try_direction(g, G2048_DIR_UP);
    } else if (k == SDLK_DOWN || k == SDLK_s) {
      g2048_try_direction(g, G2048_DIR_DOWN);
    }
  }
}

static void g2048_update(void *state, float dt)
{
  G2048Game *g = (G2048Game *)state;
  if (g->animating) {
    g->anim_t += dt;
    if (g->anim_t >= kMoveDuration) {
      g2048_finish_animation(g);
    }
  } else if (g->pulse_active) {
    g->pulse_t += dt;
    if (g->pulse_t >= kMergePulse) {
      g->pulse_active = false;
      memset(g->pulse_grid, 0, sizeof(g->pulse_grid));
    }
  }
}

static void g2048_render_animating(SDL_Renderer *r, G2048Game *g)
{
  float t = g->anim_t / kMoveDuration;
  if (t > 1.0f) {
    t = 1.0f;
  }
  float ease = 1.0f - powf(1.0f - t, 3.0f);

  for (int i = 0; i < g->sprite_count; i++) {
    const G2048AnimSprite *s = &g->sprites[i];
    float fr = s->fr + 0.5f;
    float fc = s->fc + 0.5f;
    float tr = s->tr + 0.5f;
    float tc = s->tc + 0.5f;
    float cr = fr + (tr - fr) * ease;
    float cc = fc + (tc - fc) * ease;
    SDL_FRect cell;
    g2048_cell_rect(g, cr - 0.5f, cc - 0.5f, &cell);
    g2048_draw_tile_value(r, g, &cell, s->value);
  }
}

static void g2048_render_static(SDL_Renderer *r, G2048Game *g)
{
  float pulse_scale = 1.0f;
  if (g->pulse_active) {
    float u = g->pulse_t / kMergePulse;
    pulse_scale = 1.0f + 0.06f * sinf(u * 3.1415926f);
  }
  for (int row = 0; row < G2048_SIZE; row++) {
    for (int col = 0; col < G2048_SIZE; col++) {
      int v = g->board.v[row][col];
      if (v == 0) {
        continue;
      }
      SDL_FRect cell;
      g2048_cell_rect(g, (float)row, (float)col, &cell);
      if (g->pulse_grid[row][col] && g->pulse_active) {
        float cx = cell.x + cell.w * 0.5f;
        float cy = cell.y + cell.h * 0.5f;
        cell.w *= pulse_scale;
        cell.h *= pulse_scale;
        cell.x = cx - cell.w * 0.5f;
        cell.y = cy - cell.h * 0.5f;
      }
      g2048_draw_tile_value(r, g, &cell, v);
    }
  }
}

static void g2048_render_hud(SDL_Renderer *r, G2048Game *g)
{
  SDL_Color ink = {0x77, 0x6E, 0x65, 255};
  float score_px = 3.4f;
  float x = 28.0f;
  float y = 20.0f;
  pg_digits_draw_uint(r, x, y, score_px, (uint32_t)g->board.score, ink);

  float title_px = 3.0f;
  float title_x = x + 6.0f * score_px * 7.0f + 48.0f;
  float step = 6.0f * title_px;
  SDL_FRect tag;
  tag.x = title_x - 10.0f;
  tag.y = y - 6.0f;
  tag.w = step * 4.0f + 28.0f;
  tag.h = 7.0f * title_px + 12.0f;
  SDL_SetRenderDrawColor(r, 0xBB, 0xAD, 0xA0, 255);
  SDL_RenderFillRectF(r, &tag);
  pg_digits_draw_uint(r, title_x + 0.0f * step, y, title_px, 2u, ink);
  pg_digits_draw_uint(r, title_x + 1.0f * step, y, title_px, 0u, ink);
  pg_digits_draw_uint(r, title_x + 2.0f * step, y, title_px, 4u, ink);
  pg_digits_draw_uint(r, title_x + 3.0f * step, y, title_px, 8u, ink);
}

static void g2048_render(void *state, SDL_Renderer *renderer)
{
  G2048Game *g = (G2048Game *)state;
  SDL_SetRenderDrawColor(renderer, 0xFA, 0xF8, 0xEF, 255);
  SDL_RenderClear(renderer);

  g2048_render_hud(renderer, g);
  g2048_draw_board_shell(renderer, g);
  if (g->animating) {
    g2048_render_animating(renderer, g);
  } else {
    g2048_render_static(renderer, g);
  }

  if (g->show_won_banner) {
    SDL_FRect banner;
    banner.x = 24.0f;
    banner.y = (float)g->layout_y + (float)g->board_px + 16.0f;
    int ww = 0;
    int hh = 0;
    SDL_GetRendererOutputSize(renderer, &ww, &hh);
    (void)hh;
    banner.w = (float)ww - 48.0f;
    if (banner.w < 120.0f) {
      banner.w = 120.0f;
    }
    banner.h = 10.0f;
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0xED, 0xCC, 0x61, 220);
    SDL_RenderFillRectF(renderer, &banner);
  }

  if (g->game_over) {
    int ww = 0;
    int hh = 0;
    SDL_GetRendererOutputSize(renderer, &ww, &hh);
    SDL_FRect dim = {0.0f, 0.0f, (float)ww, (float)hh};
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 25, 22, 20, 170);
    SDL_RenderFillRectF(renderer, &dim);
    float cx = (float)ww * 0.5f;
    float cy = (float)hh * 0.48f;
    float big = 4.2f;
    float text_w = 6.0f * big * 6.0f;
    pg_digits_draw_uint(
        renderer,
        cx - text_w * 0.5f,
        cy,
        big,
        (uint32_t)g->board.score,
        (SDL_Color){250, 248, 240, 255});
  }
}

static void g2048_resize(void *state, int width_px, int height_px)
{
  G2048Game *g = (G2048Game *)state;
  g2048_layout(g, width_px, height_px);
}

static const PgGameVtable kG2048Vt = {
    .id = "g2048",
    .create = g2048_create,
    .destroy = g2048_destroy,
    .reset = g2048_reset_cb,
    .on_event = g2048_on_event,
    .update = g2048_update,
    .render = g2048_render,
    .resize = g2048_resize,
};

const PgGameVtable *g2048_game_vt(void)
{
  return &kG2048Vt;
}
