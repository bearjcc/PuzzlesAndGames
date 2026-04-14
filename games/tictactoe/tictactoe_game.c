#include "tictactoe_game.h"

#include "tictactoe_board.h"
#include "pg/text.h"
#include "pg/theme.h"

#include <SDL.h>

#include <math.h>
#include <stdio.h>
#include <string.h>

typedef struct TttGame {
  SDL_Renderer *renderer;
  int8_t cells[TTT_CELL_COUNT];
  TttAiDifficulty diff;
  uint32_t rng;
  int win_w;
  int win_h;
  float board_x;
  float board_y;
  float board_side;
  float cell_side;
  TttOutcome last_outcome;
  bool ai_pending;
  float ai_delay;
} TttGame;

static void ttt_reset_board(TttGame *g)
{
  ttt_board_clear(g->cells);
  g->last_outcome = TTT_IN_PROGRESS;
  g->ai_pending = false;
  g->ai_delay = 0.0f;
}

static void ttt_layout(TttGame *g, int w, int h)
{
  g->win_w = w;
  g->win_h = h;
  float margin = 20.0f;
  float header = 100.0f;
  float footer = 72.0f;
  float usable_w = (float)w - margin * 2.0f;
  float usable_h = (float)h - margin * 2.0f - header - footer;
  float side = usable_w < usable_h ? usable_w : usable_h;
  if (side < 180.0f) {
    side = 180.0f;
  }
  g->board_side = side;
  g->cell_side = side / 3.0f;
  g->board_x = ((float)w - side) * 0.5f;
  g->board_y = header + ((float)h - header - footer - side) * 0.5f;
  if (g->board_y < header) {
    g->board_y = header;
  }
}

static float ttt_text_w(const char *s, float cap_h)
{
  float w = 0.0f;
  float h = 0.0f;
  if (!pg_text_measure_nominal(s, cap_h, &w, &h)) {
    return 0.0f;
  }
  (void)h;
  return w;
}

static void ttt_cell_xy(const TttGame *g, int idx, float *ox, float *oy)
{
  int row = idx / 3;
  int col = idx % 3;
  *ox = g->board_x + (float)col * g->cell_side;
  *oy = g->board_y + (float)row * g->cell_side;
}

static int ttt_hit_cell(const TttGame *g, float mx, float my)
{
  if (mx < g->board_x || my < g->board_y) {
    return -1;
  }
  if (mx >= g->board_x + g->board_side || my >= g->board_y + g->board_side) {
    return -1;
  }
  int col = (int)((mx - g->board_x) / g->cell_side);
  int row = (int)((my - g->board_y) / g->cell_side);
  if (col < 0) {
    col = 0;
  }
  if (col > 2) {
    col = 2;
  }
  if (row < 0) {
    row = 0;
  }
  if (row > 2) {
    row = 2;
  }
  return row * 3 + col;
}

static void ttt_draw_x(SDL_Renderer *r, float x0, float y0, float side, SDL_Color ink)
{
  float pad = side * 0.18f;
  float x1 = x0 + pad;
  float y1 = y0 + pad;
  float x2 = x0 + side - pad;
  float y2 = y0 + side - pad;
  SDL_SetRenderDrawColor(r, ink.r, ink.g, ink.b, ink.a);
  float t = pg_theme_stroke_px(side, 0.08f);
  float step = t > 3.5f ? 1.0f : 0.5f;
  for (float u = -t; u <= t; u += step) {
    SDL_RenderDrawLineF(r, x1 + u, y1, x2 + u, y2);
    SDL_RenderDrawLineF(r, x2 + u, y1, x1 + u, y2);
  }
}

static void ttt_draw_o(SDL_Renderer *r, float x0, float y0, float side, SDL_Color ink)
{
  float cx = x0 + side * 0.5f;
  float cy = y0 + side * 0.5f;
  float rad = side * 0.32f;
  SDL_SetRenderDrawColor(r, ink.r, ink.g, ink.b, ink.a);
  int segs = 56;
  float prev_x = cx + rad;
  float prev_y = cy;
  float t = pg_theme_stroke_px(side, 0.08f);
  float step = t > 3.5f ? 1.0f : 0.5f;
  for (int i = 1; i <= segs; i++) {
    float a = (float)i * (2.0f * 3.1415926f / (float)segs);
    float x = cx + cosf(a) * rad;
    float y = cy + sinf(a) * rad;
    for (float k = -t; k <= t; k += step) {
      SDL_RenderDrawLineF(r, prev_x + k, prev_y, x + k, y);
    }
    prev_x = x;
    prev_y = y;
  }
}

static void ttt_draw_board(SDL_Renderer *r, const TttGame *g)
{
  SDL_FRect back = {g->board_x - 6.0f, g->board_y - 6.0f, g->board_side + 12.0f, g->board_side + 12.0f};
  SDL_Color surf = PG_COLOR_SURFACE;
  SDL_SetRenderDrawColor(r, surf.r, surf.g, surf.b, surf.a);
  SDL_RenderFillRectF(r, &back);

  SDL_Color rule = PG_COLOR_RULE;
  SDL_SetRenderDrawColor(r, rule.r, rule.g, rule.b, rule.a);
  float x0 = g->board_x;
  float y0 = g->board_y;
  for (int i = 1; i <= 2; i++) {
    float lx = x0 + (float)i * g->cell_side;
    SDL_RenderDrawLineF(r, lx, y0, lx, y0 + g->board_side);
    float ly = y0 + (float)i * g->cell_side;
    SDL_RenderDrawLineF(r, x0, ly, x0 + g->board_side, ly);
  }

  SDL_Color cx = PG_COLOR_STATE_NEGATIVE;
  SDL_Color co = PG_COLOR_STATE_POSITIVE;
  for (int i = 0; i < TTT_CELL_COUNT; i++) {
    float ox;
    float oy;
    ttt_cell_xy(g, i, &ox, &oy);
    if (g->cells[i] == TTT_X) {
      ttt_draw_x(r, ox, oy, g->cell_side, cx);
    } else if (g->cells[i] == TTT_O) {
      ttt_draw_o(r, ox, oy, g->cell_side, co);
    }
  }
}

static bool ttt_difficulty_hit(const TttGame *g, float mx, float my, TttAiDifficulty *out)
{
  const float cap = 20.0f;
  float y = g->board_y + g->board_side + 18.0f;
  float h = 44.0f;
  float gap = 12.0f;
  float pad_x = 20.0f;
  const char *caps[3] = {"EASY", "MEDIUM", "IMP"};
  float w0 = ttt_text_w(caps[0], cap) + pad_x * 2.0f;
  float w1 = ttt_text_w(caps[1], cap) + pad_x * 2.0f;
  float w2 = ttt_text_w(caps[2], cap) + pad_x * 2.0f;
  float total_w = w0 + w1 + w2 + gap * 2.0f;
  float x0 = ((float)g->win_w - total_w) * 0.5f;
  float x = x0;
  float bw_tab[3] = {w0, w1, w2};
  for (int d = 0; d < 3; d++) {
    float bw = bw_tab[d];
    SDL_FRect hit = {x, y, bw, h};
    if (mx >= hit.x && mx <= hit.x + hit.w && my >= hit.y && my <= hit.y + hit.h) {
      *out = (TttAiDifficulty)d;
      return true;
    }
    x += bw + gap;
  }
  return false;
}

static void ttt_draw_difficulty(SDL_Renderer *r, const TttGame *g)
{
  const float cap = 20.0f;
  float y = g->board_y + g->board_side + 18.0f;
  float h = 44.0f;
  float gap = 12.0f;
  float pad_x = 20.0f;
  const char *caps[3] = {"EASY", "MEDIUM", "IMP"};
  float w0 = ttt_text_w(caps[0], cap) + pad_x * 2.0f;
  float w1 = ttt_text_w(caps[1], cap) + pad_x * 2.0f;
  float w2 = ttt_text_w(caps[2], cap) + pad_x * 2.0f;
  float total_w = w0 + w1 + w2 + gap * 2.0f;
  float x0 = ((float)g->win_w - total_w) * 0.5f;
  float x = x0;
  float bw_tab[3] = {w0, w1, w2};
  for (int d = 0; d < 3; d++) {
    float bw = bw_tab[d];
    bool sel = ((int)g->diff == d);
    SDL_FRect box = {x, y, bw, h};
    SDL_Color fill = sel ? PG_COLOR_INK : PG_COLOR_SURFACE;
    SDL_SetRenderDrawColor(r, fill.r, fill.g, fill.b, fill.a);
    SDL_RenderFillRectF(r, &box);
    SDL_Color border = PG_COLOR_RULE;
    SDL_SetRenderDrawColor(r, border.r, border.g, border.b, border.a);
    SDL_RenderDrawRectF(r, &box);
    SDL_Color ink = sel ? PG_COLOR_INVERSE_INK : PG_COLOR_INK;
    SDL_FRect tbox = {x + 4.0f, y + 4.0f, bw - 8.0f, h - 8.0f};
    pg_text_draw_utf8_centered(r, &tbox, caps[d], ink);
    x += bw + gap;
  }
}

static void ttt_fire_ai(TttGame *g)
{
  int mv = -1;
  if (!ttt_ai_choose_move(g->cells, g->diff, &g->rng, &mv) || mv < 0) {
    return;
  }
  g->cells[mv] = TTT_O;
  g->last_outcome = ttt_outcome(g->cells);
}

static void ttt_after_human_move(TttGame *g)
{
  g->last_outcome = ttt_outcome(g->cells);
  if (g->last_outcome != TTT_IN_PROGRESS) {
    return;
  }
  g->ai_pending = true;
  g->ai_delay = 0.22f;
}

static void *ttt_create(SDL_Renderer *renderer)
{
  TttGame *g = (TttGame *)SDL_calloc(1, sizeof(TttGame));
  if (g == NULL) {
    return NULL;
  }
  if (!pg_text_ref()) {
    SDL_free(g);
    return NULL;
  }
  g->renderer = renderer;
  g->diff = TTT_AI_MEDIUM;
  g->rng = (uint32_t)SDL_GetTicks();
  if (g->rng == 0u) {
    g->rng = 1u;
  }
  ttt_reset_board(g);
  int w = 0;
  int h = 0;
  SDL_GetRendererOutputSize(renderer, &w, &h);
  ttt_layout(g, w, h);
  return g;
}

static void ttt_destroy(void *state)
{
  pg_text_unref();
  SDL_free(state);
}

static void ttt_reset_cb(void *state)
{
  TttGame *g = (TttGame *)state;
  ttt_reset_board(g);
}

static void ttt_on_event(void *state, const SDL_Event *event)
{
  TttGame *g = (TttGame *)state;
  if (event->type == SDL_MOUSEBUTTONDOWN && event->button.button == SDL_BUTTON_LEFT) {
    float mx = (float)event->button.x;
    float my = (float)event->button.y;
    if (g->renderer != NULL) {
      SDL_Window *win = SDL_RenderGetWindow(g->renderer);
      if (win != NULL) {
        int ww = 0;
        int wh = 0;
        int rw = 0;
        int rh = 0;
        SDL_GetWindowSize(win, &ww, &wh);
        SDL_GetRendererOutputSize(g->renderer, &rw, &rh);
        if (ww > 0 && wh > 0) {
          mx = mx * (float)rw / (float)ww;
          my = my * (float)rh / (float)wh;
        }
      }
    }
    TttAiDifficulty dh;
    if (ttt_difficulty_hit(g, mx, my, &dh)) {
      g->diff = dh;
      return;
    }
    if (g->ai_pending) {
      return;
    }
    if (g->last_outcome != TTT_IN_PROGRESS) {
      return;
    }
    if (ttt_side_to_move(g->cells) != TTT_X) {
      return;
    }
    int idx = ttt_hit_cell(g, mx, my);
    if (idx < 0 || !ttt_cell_empty(g->cells, idx)) {
      return;
    }
    g->cells[idx] = TTT_X;
    ttt_after_human_move(g);
    return;
  }
  if (event->type != SDL_KEYDOWN) {
    return;
  }
  const SDL_Keycode k = event->key.keysym.sym;
  if (k == SDLK_r) {
    ttt_reset_cb(g);
    return;
  }
  if (g->last_outcome != TTT_IN_PROGRESS && (k == SDLK_RETURN || k == SDLK_SPACE)) {
    ttt_reset_cb(g);
    return;
  }
  if (k == SDLK_1) {
    g->diff = TTT_AI_EASY;
  } else if (k == SDLK_2) {
    g->diff = TTT_AI_MEDIUM;
  } else if (k == SDLK_3) {
    g->diff = TTT_AI_IMPOSSIBLE;
  }
}

static void ttt_update(void *state, float dt)
{
  TttGame *g = (TttGame *)state;
  if (!g->ai_pending) {
    return;
  }
  g->ai_delay -= dt;
  if (g->ai_delay > 0.0f) {
    return;
  }
  g->ai_pending = false;
  ttt_fire_ai(g);
}

static void ttt_render(void *state, SDL_Renderer *renderer)
{
  TttGame *g = (TttGame *)state;
  pg_theme_clear_paper(renderer);

  SDL_Color title_c = PG_COLOR_INK;
  float tw = ttt_text_w("Tic tac toe", 26.0f);
  pg_text_draw_utf8(renderer, ((float)g->win_w - tw) * 0.5f, 20.0f, 26.0f, "Tic tac toe", title_c);

  SDL_Color sub = PG_COLOR_INK_MUTED;
  pg_text_draw_utf8(renderer, 20.0f, 58.0f, 17.0f, "You X  AI O", sub);
  const char *dh = "Easy";
  if (g->diff == TTT_AI_MEDIUM) {
    dh = "Medium";
  } else if (g->diff == TTT_AI_IMPOSSIBLE) {
    dh = "Impossible";
  }
  char buf[48];
  (void)snprintf(buf, sizeof(buf), "AI %s", dh);
  float rw = (float)g->win_w - 20.0f - ttt_text_w(buf, 17.0f);
  pg_text_draw_utf8(renderer, rw, 58.0f, 17.0f, buf, sub);

  ttt_draw_board(renderer, g);
  ttt_draw_difficulty(renderer, g);

  pg_text_draw_utf8(renderer, 20.0f, (float)g->win_h - 30.0f, 15.0f, "1 2 3 difficulty  R new game", PG_COLOR_INK_FAINT);

  if (g->last_outcome != TTT_IN_PROGRESS) {
    int ww = g->win_w;
    int hh = g->win_h;
    SDL_FRect dim = {0.0f, 0.0f, (float)ww, (float)hh};
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_Color scrim = PG_COLOR_OVERLAY_SCRIM;
    SDL_SetRenderDrawColor(renderer, scrim.r, scrim.g, scrim.b, scrim.a);
    SDL_RenderFillRectF(renderer, &dim);

    const char *msg = "Draw";
    if (g->last_outcome == TTT_X_WINS) {
      msg = "You win";
    } else if (g->last_outcome == TTT_O_WINS) {
      msg = "AI wins";
    }
    SDL_Color ink = PG_COLOR_INVERSE_INK;
    float cap = 30.0f;
    float mw = ttt_text_w(msg, cap);
    pg_text_draw_utf8(renderer, ((float)ww - mw) * 0.5f, (float)hh * 0.40f, cap, msg, ink);
    const char *hint = "Enter, space, or R";
    float hp = 17.0f;
    float hw = ttt_text_w(hint, hp);
    pg_text_draw_utf8(renderer, ((float)ww - hw) * 0.5f, (float)hh * 0.40f + cap * 1.35f, hp, hint, PG_COLOR_INVERSE_MUTED);
  }
}

static void ttt_resize(void *state, int width_px, int height_px)
{
  TttGame *g = (TttGame *)state;
  ttt_layout(g, width_px, height_px);
}

static const PgGameVtable kTttVt = {
    .id = "tictactoe",
    .create = ttt_create,
    .destroy = ttt_destroy,
    .reset = ttt_reset_cb,
    .on_event = ttt_on_event,
    .update = ttt_update,
    .render = ttt_render,
    .resize = ttt_resize,
};

const PgGameVtable *tictactoe_game_vt(void)
{
  return &kTttVt;
}
