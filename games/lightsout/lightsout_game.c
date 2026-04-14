#include "lightsout_game.h"

#include "lightsout_board.h"
#include "pg/digits.h"

#include <SDL.h>

#include <math.h>

typedef struct LightsoutGame {
  SDL_Renderer *renderer;
  LightsoutBoard board;
  int win_w;
  int win_h;
  int origin_x;
  int origin_y;
  int cell_px;
  int gap_px;
  bool show_win_overlay;
} LightsoutGame;

static void lightsout_layout(LightsoutGame *g, int width, int height)
{
  g->win_w = width;
  g->win_h = height;
  int margin = 24;
  int header = 88;
  int usable_w = width - margin * 2;
  int usable_h = height - margin * 2 - header;
  int board_cells = LIGHTSOUT_SIZE;
  int gaps = board_cells + 1;
  int side = usable_w < usable_h ? usable_w : usable_h;
  if (side < 220) {
    side = 220;
  }
  g->gap_px = 6;
  g->cell_px = (side - g->gap_px * gaps) / board_cells;
  if (g->cell_px < 28) {
    g->cell_px = 28;
  }
  int board_px = g->cell_px * board_cells + g->gap_px * gaps;
  g->origin_x = (width - board_px) / 2;
  g->origin_y = header + (height - header - board_px - margin) / 2;
  if (g->origin_y < header) {
    g->origin_y = header;
  }
}

static void lightsout_cell_rect(const LightsoutGame *g, int row, int col, SDL_FRect *out)
{
  float x = (float)g->origin_x + (float)g->gap_px + (float)col * ((float)g->cell_px + (float)g->gap_px);
  float y = (float)g->origin_y + (float)g->gap_px + (float)row * ((float)g->cell_px + (float)g->gap_px);
  out->x = x;
  out->y = y;
  out->w = (float)g->cell_px;
  out->h = (float)g->cell_px;
}

static int lightsout_pick_cell(const LightsoutGame *g, float mx, float my, int *row_out, int *col_out)
{
  for (int r = 0; r < LIGHTSOUT_SIZE; r++) {
    for (int c = 0; c < LIGHTSOUT_SIZE; c++) {
      SDL_FRect cell;
      lightsout_cell_rect(g, r, c, &cell);
      if (mx >= cell.x && mx < cell.x + cell.w && my >= cell.y && my < cell.y + cell.h) {
        *row_out = r;
        *col_out = c;
        return 1;
      }
    }
  }
  return 0;
}

static void lightsout_reset(LightsoutGame *g, uint32_t seed)
{
  lightsout_board_reset(&g->board, seed);
  g->show_win_overlay = false;
}

static void *lightsout_create(SDL_Renderer *renderer)
{
  LightsoutGame *g = (LightsoutGame *)SDL_calloc(1, sizeof(LightsoutGame));
  if (g == NULL) {
    return NULL;
  }
  g->renderer = renderer;
  uint32_t seed = (uint32_t)SDL_GetTicks();
  lightsout_reset(g, seed == 0u ? 1u : seed);
  int w = 0;
  int h = 0;
  SDL_GetRendererOutputSize(renderer, &w, &h);
  lightsout_layout(g, w, h);
  return g;
}

static void lightsout_destroy(void *state)
{
  SDL_free(state);
}

static void lightsout_reset_cb(void *state)
{
  LightsoutGame *g = (LightsoutGame *)state;
  uint32_t seed = (uint32_t)SDL_GetTicks();
  lightsout_reset(g, seed == 0u ? 1u : seed);
}

static void lightsout_on_event(void *state, const SDL_Event *event)
{
  LightsoutGame *g = (LightsoutGame *)state;
  if (event->type == SDL_KEYDOWN) {
    const SDL_Keycode k = event->key.keysym.sym;
    if (k == SDLK_r) {
      lightsout_reset_cb(g);
      return;
    }
    if (g->show_win_overlay && (k == SDLK_RETURN || k == SDLK_SPACE)) {
      lightsout_reset_cb(g);
      return;
    }
  } else if (event->type == SDL_MOUSEBUTTONDOWN && event->button.button == SDL_BUTTON_LEFT) {
    if (g->show_win_overlay) {
      lightsout_reset_cb(g);
      return;
    }
    SDL_Event be = *event;
    float mx = (float)be.button.x;
    float my = (float)be.button.y;
    if (g->renderer != NULL) {
      SDL_RenderWindowToLogical(g->renderer, mx, my, &mx, &my);
    }
    int r = 0;
    int c = 0;
    if (lightsout_pick_cell(g, mx, my, &r, &c)) {
      lightsout_board_toggle_at(&g->board, r, c);
      if (lightsout_board_is_solved(&g->board)) {
        g->show_win_overlay = true;
      }
    }
  }
}

static void lightsout_update(void *state, float dt_seconds)
{
  (void)state;
  (void)dt_seconds;
}

static void lightsout_draw_hud(SDL_Renderer *r, const LightsoutGame *g)
{
  SDL_Color ink = {0x2C, 0x3E, 0x50, 255};
  float y = 24.0f;
  float x = 28.0f;
  pg_digits_draw_uint(r, x, y, 3.4f, g->board.moves, ink);
}

static void lightsout_render(void *state, SDL_Renderer *renderer)
{
  LightsoutGame *g = (LightsoutGame *)state;
  SDL_SetRenderDrawColor(renderer, 0xF5, 0xF6, 0xFA, 255);
  SDL_RenderClear(renderer);

  lightsout_draw_hud(renderer, g);

  SDL_FRect shell;
  shell.x = (float)g->origin_x;
  shell.y = (float)g->origin_y;
  shell.w = (float)(g->cell_px * LIGHTSOUT_SIZE + g->gap_px * (LIGHTSOUT_SIZE + 1));
  shell.h = shell.w;
  SDL_SetRenderDrawColor(renderer, 0xDF, 0xE6, 0xF1, 255);
  SDL_RenderFillRectF(renderer, &shell);

  for (int row = 0; row < LIGHTSOUT_SIZE; row++) {
    for (int col = 0; col < LIGHTSOUT_SIZE; col++) {
      SDL_FRect cell;
      lightsout_cell_rect(g, row, col, &cell);
      bool on = lightsout_board_is_on(&g->board, row, col);
      if (on) {
        SDL_SetRenderDrawColor(renderer, 0xF3, 0x9C, 0x12, 255);
      } else {
        SDL_SetRenderDrawColor(renderer, 0x34, 0x49, 0x5E, 255);
      }
      SDL_RenderFillRectF(renderer, &cell);
      SDL_SetRenderDrawColor(renderer, 0x1A, 0x25, 0x36, 255);
      float inset = 1.0f;
      SDL_FRect border = {cell.x - inset, cell.y - inset, cell.w + inset * 2.0f, cell.h + inset * 2.0f};
      SDL_RenderDrawRectF(renderer, &border);
    }
  }

  if (g->show_win_overlay) {
    int ww = 0;
    int hh = 0;
    SDL_GetRendererOutputSize(renderer, &ww, &hh);
    SDL_FRect dim = {0.0f, 0.0f, (float)ww, (float)hh};
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 20, 30, 40, 190);
    SDL_RenderFillRectF(renderer, &dim);

    float cx = (float)ww * 0.5f;
    float cy = (float)hh * 0.46f;
    float big = 5.0f;
    uint32_t moves = g->board.moves;
    float step = 6.0f * big;
    int digits = 1;
    uint32_t t = moves;
    while (t >= 10u) {
      t /= 10u;
      digits++;
    }
    float text_w = step * (float)digits;
    pg_digits_draw_uint(
        renderer,
        cx - text_w * 0.5f,
        cy,
        big,
        moves,
        (SDL_Color){236, 240, 241, 255});

    float ring_r = (float)g->cell_px * 0.85f;
    float pulse = 0.92f + 0.08f * sinf((float)SDL_GetTicks() * 0.004f);
    ring_r *= pulse;
    SDL_SetRenderDrawColor(renderer, 0x2E, 0xCC, 0x71, 255);
    float rr = ring_r * 0.5f;
    for (int i = 0; i < 48; i++) {
      float a = (float)i * (6.2831853f / 48.0f);
      float px = cx + cosf(a) * rr;
      float py = cy + 80.0f + sinf(a) * rr;
      SDL_FRect dot = {px - 2.0f, py - 2.0f, 4.0f, 4.0f};
      SDL_RenderFillRectF(renderer, &dot);
    }
  }
}

static void lightsout_resize(void *state, int width_px, int height_px)
{
  LightsoutGame *g = (LightsoutGame *)state;
  lightsout_layout(g, width_px, height_px);
}

static const PgGameVtable kLightsoutVt = {
    .id = "lightsout",
    .create = lightsout_create,
    .destroy = lightsout_destroy,
    .reset = lightsout_reset_cb,
    .on_event = lightsout_on_event,
    .update = lightsout_update,
    .render = lightsout_render,
    .resize = lightsout_resize,
};

const PgGameVtable *lightsout_game_vt(void)
{
  return &kLightsoutVt;
}
