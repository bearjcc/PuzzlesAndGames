#include "mastermind_game.h"

#include "mastermind_logic.h"
#include "pg/catalog/pg_catalog.h"
#include "pg/digits.h"
#include "pg/sdl.h"

#include <SDL.h>

#include <math.h>
#include <string.h>

typedef struct MmUi {
  MmState st;
  SDL_Renderer *renderer;
  int win_w;
  int win_h;
  int margin;
  int peg_r;
  int slot_step;
  int row_h;
  int palette_x;
  int palette_y;
  int palette_step;
  int board_x;
  int board_y;
  int feedback_w;
  int submit_x;
  int submit_y;
  int submit_w;
  int submit_h;
  int secret_y;
} MmUi;

static const SDL_Color kMmPalette[MM_MAX_COLOURS] = {
    {200, 60, 60, 255},
    {60, 140, 220, 255},
    {50, 160, 80, 255},
    {220, 180, 40, 255},
    {140, 80, 200, 255},
    {30, 30, 30, 255},
    {240, 120, 180, 255},
    {100, 200, 200, 255},
    {180, 100, 50, 255},
    {160, 160, 170, 255},
};

static void mm_draw_filled_circle(SDL_Renderer *r, float cx, float cy, float rad, SDL_Color col)
{
  int ir = (int)ceilf(rad);
  SDL_SetRenderDrawColor(r, col.r, col.g, col.b, col.a);
  for (int dy = -ir; dy <= ir; dy++) {
    float y = (float)dy;
    float inner = rad * rad - y * y;
    if (inner < 0.0f) {
      continue;
    }
    int w = (int)floorf(sqrtf(inner));
    SDL_RenderDrawLineF(r, cx - (float)w, cy + y, cx + (float)w, cy + y);
  }
}

static void mm_draw_hole(SDL_Renderer *r, float cx, float cy, float rad)
{
  SDL_Color rim = {140, 130, 120, 255};
  SDL_Color hole = {90, 85, 78, 255};
  mm_draw_filled_circle(r, cx, cy, rad + 1.5f, rim);
  mm_draw_filled_circle(r, cx, cy, rad - 0.5f, hole);
}

static void mm_layout(MmUi *u)
{
  const MmConfig *c = &u->st.cfg;
  u->margin = 20;
  u->peg_r = 14;
  if (u->win_w < 520) {
    u->peg_r = 11;
  }
  u->slot_step = u->peg_r * 2 + 10;
  u->row_h = u->peg_r * 2 + 14;
  u->palette_step = u->peg_r * 2 + 8;

  u->feedback_w = 52;
  int board_w = c->npegs * u->slot_step + u->feedback_w + 24;
  int palette_w = u->palette_step + 24;
  if (palette_w < board_w / 4) {
    palette_w = board_w / 4;
  }
  u->palette_x = u->margin;
  u->palette_y = u->margin + 56;

  u->board_x = u->margin + palette_w;
  u->board_y = u->margin + 56;

  u->submit_w = 100;
  u->submit_h = 32;
  u->submit_x = u->board_x;
  u->submit_y = u->board_y + c->nguesses * u->row_h + 12;
  u->secret_y = u->submit_y + u->submit_h + 16;
}

static int mm_active_row(const MmUi *u)
{
  if (u->st.outcome != 0) {
    return -1;
  }
  return u->st.n_committed;
}

static void mm_reset(MmUi *u, uint32_t seed)
{
  MmConfig cfg;
  mm_config_default(&cfg);
  /* Enable blank pegs so the palette matches STP Guess optional mode (classic default remains off in mm_config_default). */
  cfg.allow_blank = true;
  mm_state_reset(&u->st, &cfg, seed);
}

/* Sets *out_value to 0 (blank) or 1..ncolours. *out_value = -1 if no hit. */
static void mm_palette_hit(const MmUi *u, float mx, float my, int *out_value)
{
  *out_value = -1;
  int row = 0;
  if (u->st.cfg.allow_blank) {
    float cx = (float)u->palette_x + (float)u->peg_r + 12.0f;
    float cy = (float)u->palette_y + (float)u->peg_r;
    float dx = mx - cx;
    float dy = my - cy;
    if (dx * dx + dy * dy <= (float)(u->peg_r + 6) * (float)(u->peg_r + 6)) {
      *out_value = 0;
      return;
    }
    row = 1;
  }
  for (int i = 0; i < u->st.cfg.ncolours; i++) {
    float cx = (float)u->palette_x + (float)u->peg_r + 12.0f;
    float cy = (float)u->palette_y + (float)(row + i) * (float)u->palette_step + (float)u->peg_r;
    float dx = mx - cx;
    float dy = my - cy;
    if (dx * dx + dy * dy <= (float)(u->peg_r + 6) * (float)(u->peg_r + 6)) {
      *out_value = i + 1;
      return;
    }
  }
}

static bool mm_submit_hit(const MmUi *u, float mx, float my)
{
  return mx >= (float)u->submit_x && mx < (float)(u->submit_x + u->submit_w) && my >= (float)u->submit_y &&
         my < (float)(u->submit_y + u->submit_h);
}

static bool mm_slot_hit(const MmUi *u, float mx, float my, int *out_slot)
{
  *out_slot = -1;
  int row = mm_active_row(u);
  if (row < 0) {
    return false;
  }
  float y0 = (float)u->board_y + (float)row * (float)u->row_h + (float)u->peg_r;
  if (fabsf(my - y0) > (float)(u->peg_r + 8)) {
    return false;
  }
  for (int s = 0; s < u->st.cfg.npegs; s++) {
    float cx = (float)u->board_x + (float)u->peg_r + 10.0f + (float)s * (float)u->slot_step;
    float dx = mx - cx;
    if (fabsf(dx) <= (float)(u->peg_r + 8)) {
      *out_slot = s;
      return true;
    }
  }
  return false;
}

static void *mm_create(SDL_Renderer *renderer)
{
  MmUi *u = (MmUi *)SDL_calloc(1, sizeof(MmUi));
  if (u == NULL) {
    return NULL;
  }
  uint32_t seed = (uint32_t)SDL_GetTicks();
  mm_reset(u, seed == 0u ? 1u : seed);
  u->renderer = renderer;
  int w = 0;
  int h = 0;
  (void)pg_sdl_renderer_output_size(renderer, &w, &h);
  u->win_w = w;
  u->win_h = h;
  mm_layout(u);
  return u;
}

static void mm_destroy(void *state)
{
  SDL_free(state);
}

static void mm_reset_cb(void *state)
{
  MmUi *u = (MmUi *)state;
  uint32_t seed = (uint32_t)SDL_GetTicks();
  mm_reset(u, seed == 0u ? 1u : seed);
  mm_layout(u);
}

static void mm_on_event(void *state, const SDL_Event *event)
{
  MmUi *u = (MmUi *)state;
  if (event->type == SDL_MOUSEBUTTONDOWN && event->button.button == SDL_BUTTON_LEFT) {
    float mx;
    float my;
    if (!pg_sdl_event_pointer_logical(u->renderer, event, &mx, &my)) {
      return;
    }
    int pi = -1;
    mm_palette_hit(u, mx, my, &pi);
    if (pi >= 0) {
      u->st.selected_colour = pi;
    }
    int slot = -1;
    if (mm_slot_hit(u, mx, my, &slot) && slot >= 0) {
      u->st.cursor_slot = slot;
      u->st.draft[slot] = u->st.selected_colour;
    }
    if (mm_submit_hit(u, mx, my)) {
      (void)mm_try_submit(&u->st);
    }
  }
  if (event->type != SDL_KEYDOWN) {
    return;
  }
  const SDL_Keycode k = event->key.keysym.sym;
  if (k == SDLK_ESCAPE) {
    (void)pg_catalog_launch_from_renderer(u->renderer);
    return;
  }
  if (k == SDLK_r) {
    mm_reset_cb(u);
    return;
  }
  if (u->st.outcome != 0 && (k == SDLK_RETURN || k == SDLK_SPACE)) {
    mm_reset_cb(u);
    return;
  }
  if (u->st.outcome != 0) {
    return;
  }
  if (k == SDLK_LEFT) {
    if (u->st.cursor_slot > 0) {
      u->st.cursor_slot--;
    }
  } else if (k == SDLK_RIGHT) {
    if (u->st.cursor_slot < u->st.cfg.npegs - 1) {
      u->st.cursor_slot++;
    }
  } else if (k == SDLK_UP) {
    int hi = u->st.cfg.ncolours;
    if (u->st.selected_colour < hi) {
      u->st.selected_colour++;
    }
  } else if (k == SDLK_DOWN) {
    int lo = u->st.cfg.allow_blank ? 0 : 1;
    if (u->st.selected_colour > lo) {
      u->st.selected_colour--;
    }
  } else if (k == SDLK_SPACE) {
    u->st.draft[u->st.cursor_slot] = u->st.selected_colour;
  } else if (k == SDLK_BACKSPACE || k == SDLK_DELETE) {
    u->st.draft[u->st.cursor_slot] = 0;
  } else if (k == SDLK_RETURN || k == SDLK_KP_ENTER) {
    (void)mm_try_submit(&u->st);
  } else if (k == SDLK_0 && u->st.cfg.allow_blank) {
    u->st.selected_colour = 0;
    u->st.draft[u->st.cursor_slot] = 0;
  } else if (k >= SDLK_1 && k <= SDLK_9) {
    int d = (int)(k - SDLK_1) + 1;
    if (d <= u->st.cfg.ncolours) {
      u->st.selected_colour = d;
      u->st.draft[u->st.cursor_slot] = d;
    }
  }
}

static void mm_update(void *state, float dt_seconds)
{
  (void)state;
  (void)dt_seconds;
}

static void mm_draw_row(
    SDL_Renderer *r,
    const MmUi *u,
    int row_index,
    const int *pegs,
    int exact,
    int partial,
    bool is_draft,
    bool show_cursor)
{
  float y = (float)u->board_y + (float)row_index * (float)u->row_h + (float)u->peg_r;
  for (int s = 0; s < u->st.cfg.npegs; s++) {
    float cx = (float)u->board_x + (float)u->peg_r + 10.0f + (float)s * (float)u->slot_step;
    int pv = pegs[s];
    if (pv >= 1 && pv <= u->st.cfg.ncolours) {
      mm_draw_filled_circle(r, cx, y, (float)u->peg_r, kMmPalette[pv - 1]);
    } else if (pv == 0 && u->st.cfg.allow_blank && (is_draft || row_index < u->st.n_committed)) {
      mm_draw_hole(r, cx, y, (float)u->peg_r);
      SDL_SetRenderDrawColor(r, 200, 195, 188, 255);
      float inset = (float)u->peg_r * 0.45f;
      SDL_RenderDrawLineF(r, cx - inset, y - inset, cx + inset, y + inset);
      SDL_RenderDrawLineF(r, cx - inset, y + inset, cx + inset, y - inset);
    } else {
      mm_draw_hole(r, cx, y, (float)u->peg_r);
    }
    if (is_draft && show_cursor && s == u->st.cursor_slot) {
      SDL_SetRenderDrawColor(r, 255, 255, 255, 200);
      SDL_FRect ring = {cx - (float)u->peg_r - 4.0f, y - (float)u->peg_r - 4.0f,
          (float)u->peg_r * 2.0f + 8.0f, (float)u->peg_r * 2.0f + 8.0f};
      SDL_RenderDrawRectF(r, &ring);
    }
  }
  if (!is_draft && u->st.n_committed > row_index) {
    float base_x = (float)u->board_x + (float)u->st.cfg.npegs * (float)u->slot_step + 10.0f;
    float base_y = y - 2.0f;
    float hr = 4.5f;
    int k = 0;
    for (int i = 0; i < exact && k < u->st.cfg.npegs; i++) {
      float cx = base_x + (float)(k % 4) * (hr * 2.0f + 3.0f);
      float cy = base_y + (float)(k / 4) * (hr * 2.0f + 3.0f);
      mm_draw_filled_circle(r, cx, cy, hr, (SDL_Color){25, 25, 28, 255});
      k++;
    }
    for (int i = 0; i < partial && k < u->st.cfg.npegs; i++) {
      float cx = base_x + (float)(k % 4) * (hr * 2.0f + 3.0f);
      float cy = base_y + (float)(k / 4) * (hr * 2.0f + 3.0f);
      mm_draw_filled_circle(r, cx, cy, hr + 0.5f, (SDL_Color){55, 50, 48, 255});
      mm_draw_filled_circle(r, cx, cy, hr - 0.5f, (SDL_Color){240, 238, 232, 255});
      k++;
    }
  }
}

static void mm_render(void *state, SDL_Renderer *renderer)
{
  MmUi *u = (MmUi *)state;
  SDL_SetRenderDrawColor(renderer, 0xF5, 0xF0, 0xE8, 255);
  SDL_RenderClear(renderer);

  SDL_Color hud = {60, 55, 50, 255};
  if (u->st.outcome == 0) {
    pg_digits_draw_uint(renderer, (float)u->win_w - 120.0f, 18.0f, 2.2f, (uint32_t)(u->st.n_committed + 1), hud);
  }

  if (u->st.cfg.allow_blank) {
    float cx = (float)u->palette_x + (float)u->peg_r + 12.0f;
    float cy = (float)u->palette_y + (float)u->peg_r;
    if (u->st.selected_colour == 0) {
      SDL_SetRenderDrawColor(renderer, 80, 70, 60, 255);
      SDL_FRect hi = {cx - (float)u->peg_r - 6.0f, cy - (float)u->peg_r - 6.0f,
          (float)u->peg_r * 2.0f + 12.0f, (float)u->peg_r * 2.0f + 12.0f};
      SDL_RenderFillRectF(renderer, &hi);
    }
    mm_draw_hole(renderer, cx, cy, (float)u->peg_r);
    SDL_SetRenderDrawColor(renderer, 200, 195, 188, 255);
    float inset = (float)u->peg_r * 0.45f;
    SDL_RenderDrawLineF(renderer, cx - inset, cy - inset, cx + inset, cy + inset);
    SDL_RenderDrawLineF(renderer, cx - inset, cy + inset, cx + inset, cy - inset);
  }
  for (int i = 0; i < u->st.cfg.ncolours; i++) {
    float cx = (float)u->palette_x + (float)u->peg_r + 12.0f;
    int pr = u->st.cfg.allow_blank ? 1 : 0;
    float cy = (float)u->palette_y + (float)(pr + i) * (float)u->palette_step + (float)u->peg_r;
    SDL_Color col = kMmPalette[i];
    if (i + 1 == u->st.selected_colour) {
      SDL_SetRenderDrawColor(renderer, 80, 70, 60, 255);
      SDL_FRect hi = {cx - (float)u->peg_r - 6.0f, cy - (float)u->peg_r - 6.0f,
          (float)u->peg_r * 2.0f + 12.0f, (float)u->peg_r * 2.0f + 12.0f};
      SDL_RenderFillRectF(renderer, &hi);
    }
    mm_draw_filled_circle(renderer, cx, cy, (float)u->peg_r, col);
  }

  for (int row = 0; row < u->st.cfg.nguesses; row++) {
    if (row < u->st.n_committed) {
      mm_draw_row(renderer, u, row, u->st.history[row].pegs, u->st.history[row].exact, u->st.history[row].partial,
          false, false);
    } else if (row == u->st.n_committed && u->st.outcome == 0) {
      mm_draw_row(renderer, u, row, u->st.draft, 0, 0, true, true);
    } else {
      int empty[MM_MAX_PEGS] = {0};
      mm_draw_row(renderer, u, row, empty, 0, 0, false, false);
    }
  }

  SDL_SetRenderDrawColor(renderer, 120, 108, 96, 255);
  SDL_FRect sub = {(float)u->submit_x, (float)u->submit_y, (float)u->submit_w, (float)u->submit_h};
  SDL_RenderFillRectF(renderer, &sub);
  SDL_SetRenderDrawColor(renderer, 40, 36, 32, 255);
  SDL_RenderDrawRectF(renderer, &sub);

  if (u->st.outcome != 0) {
    float y = (float)u->secret_y + 8.0f;
    for (int s = 0; s < u->st.cfg.npegs; s++) {
      float cx = (float)u->board_x + (float)u->peg_r + 10.0f + (float)s * (float)u->slot_step;
      int pv = u->st.secret[s];
      mm_draw_filled_circle(renderer, cx, y, (float)u->peg_r, kMmPalette[pv - 1]);
    }
  }

  if (u->st.outcome == 1) {
    int ww = 0;
    int hh = 0;
    (void)pg_sdl_renderer_output_size(renderer, &ww, &hh);
    SDL_FRect dim = {0.0f, 0.0f, (float)ww, (float)hh};
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 40, 90, 40, 120);
    SDL_RenderFillRectF(renderer, &dim);
  } else if (u->st.outcome == -1) {
    int ww = 0;
    int hh = 0;
    (void)pg_sdl_renderer_output_size(renderer, &ww, &hh);
    SDL_FRect dim = {0.0f, 0.0f, (float)ww, (float)hh};
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 60, 40, 40, 140);
    SDL_RenderFillRectF(renderer, &dim);
  }
}

static void mm_resize(void *state, int width_px, int height_px)
{
  MmUi *u = (MmUi *)state;
  u->win_w = width_px;
  u->win_h = height_px;
  mm_layout(u);
}

static const PgGameVtable kMmVt = {
    .id = "mastermind",
    .create = mm_create,
    .destroy = mm_destroy,
    .reset = mm_reset_cb,
    .on_event = mm_on_event,
    .update = mm_update,
    .render = mm_render,
    .resize = mm_resize,
};

const PgGameVtable *mastermind_game_vt(void)
{
  return &kMmVt;
}
