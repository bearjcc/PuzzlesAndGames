#include "tictactoe_game.h"

#include "letterlock_glyphs.h"
#include "tictactoe_board.h"

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

static void ttt_draw_str(SDL_Renderer *r, float x, float y, float px, const char *s, SDL_Color c)
{
  float step = 6.0f * px;
  float cx = x;
  for (const char *p = s; *p != '\0'; p++) {
    if (*p == ' ') {
      cx += step * 0.45f;
      continue;
    }
    letterlock_draw_char(r, cx, y, px, *p, c);
    cx += step;
  }
}

static float ttt_text_width(float px, const char *s)
{
  float step = 6.0f * px;
  float w = 0.0f;
  for (const char *p = s; *p != '\0'; p++) {
    if (*p == ' ') {
      w += step * 0.45f;
    } else {
      w += step;
    }
  }
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
  float t = fmaxf(2.0f, side * 0.08f);
  for (float u = -t; u <= t; u += 1.0f) {
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
  int segs = 48;
  float prev_x = cx + rad;
  float prev_y = cy;
  for (int i = 1; i <= segs; i++) {
    float a = (float)i * (2.0f * 3.1415926f / (float)segs);
    float x = cx + cosf(a) * rad;
    float y = cy + sinf(a) * rad;
    float t = fmaxf(2.0f, side * 0.08f);
    for (float k = -t; k <= t; k += 1.0f) {
      SDL_RenderDrawLineF(r, prev_x + k, prev_y, x + k, y);
    }
    prev_x = x;
    prev_y = y;
  }
}

static void ttt_draw_board(SDL_Renderer *r, const TttGame *g)
{
  SDL_FRect back = {g->board_x - 6.0f, g->board_y - 6.0f, g->board_side + 12.0f, g->board_side + 12.0f};
  SDL_SetRenderDrawColor(r, 0xD5, 0xDB, 0xDB, 255);
  SDL_RenderFillRectF(r, &back);

  SDL_SetRenderDrawColor(r, 0x2C, 0x3E, 0x50, 255);
  float x0 = g->board_x;
  float y0 = g->board_y;
  for (int i = 1; i <= 2; i++) {
    float lx = x0 + (float)i * g->cell_side;
    SDL_RenderDrawLineF(r, lx, y0, lx, y0 + g->board_side);
    float ly = y0 + (float)i * g->cell_side;
    SDL_RenderDrawLineF(r, x0, ly, x0 + g->board_side, ly);
  }

  SDL_Color cx = {0xC0, 0x39, 0x2B, 255};
  SDL_Color co = {0x27, 0xAE, 0x60, 255};
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
  float y = g->board_y + g->board_side + 18.0f;
  float h = 40.0f;
  float gap = 10.0f;
  float labels_w = ttt_text_width(2.0f, "EASY") + ttt_text_width(2.0f, "MEDIUM") + ttt_text_width(2.0f, "IMP");
  float total_w = labels_w + gap * 2.0f + 36.0f * 3.0f;
  float x0 = ((float)g->win_w - total_w) * 0.5f;
  float x = x0;
  const char *caps[3] = {"EASY", "MEDIUM", "IMP"};
  for (int d = 0; d < 3; d++) {
    float bw = ttt_text_width(2.0f, caps[d]) + 24.0f;
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
  float y = g->board_y + g->board_side + 18.0f;
  float h = 40.0f;
  float gap = 10.0f;
  const char *caps[3] = {"EASY", "MEDIUM", "IMP"};
  float labels_w = ttt_text_width(2.0f, caps[0]) + ttt_text_width(2.0f, caps[1]) + ttt_text_width(2.0f, caps[2]);
  float total_w = labels_w + gap * 2.0f + 36.0f * 3.0f;
  float x0 = ((float)g->win_w - total_w) * 0.5f;
  float x = x0;
  for (int d = 0; d < 3; d++) {
    float bw = ttt_text_width(2.0f, caps[d]) + 24.0f;
    bool sel = ((int)g->diff == d);
    SDL_FRect box = {x, y, bw, h};
    if (sel) {
      SDL_SetRenderDrawColor(r, 0x34, 0x98, 0xDB, 255);
    } else {
      SDL_SetRenderDrawColor(r, 0xEC, 0xF0, 0xF1, 255);
    }
    SDL_RenderFillRectF(r, &box);
    SDL_SetRenderDrawColor(r, 0x2C, 0x3E, 0x50, 255);
    SDL_RenderDrawRectF(r, &box);
    SDL_Color ink = {0x2C, 0x3E, 0x50, 255};
    if (sel) {
      ink = (SDL_Color){255, 255, 255, 255};
    }
    float tx = x + (bw - ttt_text_width(2.0f, caps[d])) * 0.5f;
    float ty = y + (h - 7.0f * 2.0f) * 0.5f;
    ttt_draw_str(r, tx, ty, 2.0f, caps[d], ink);
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
  SDL_SetRenderDrawColor(renderer, 0xEC, 0xF0, 0xF1, 255);
  SDL_RenderClear(renderer);

  SDL_Color title_c = {0x2C, 0x3E, 0x50, 255};
  float tw = ttt_text_width(3.2f, "TIC TAC TOE");
  ttt_draw_str(renderer, ((float)g->win_w - tw) * 0.5f, 22.0f, 3.2f, "TIC TAC TOE", title_c);

  SDL_Color sub = {0x5D, 0x6D, 0x7E, 255};
  ttt_draw_str(renderer, 20.0f, 62.0f, 2.0f, "YOU X AI O", sub);
  const char *dh = "EASY";
  if (g->diff == TTT_AI_MEDIUM) {
    dh = "MEDIUM";
  } else if (g->diff == TTT_AI_IMPOSSIBLE) {
    dh = "IMPOSSIBLE";
  }
  char buf[48];
  (void)snprintf(buf, sizeof(buf), "AI %s", dh);
  float rw = (float)g->win_w - 20.0f - ttt_text_width(2.0f, buf);
  ttt_draw_str(renderer, rw, 62.0f, 2.0f, buf, sub);

  ttt_draw_board(renderer, g);
  ttt_draw_difficulty(renderer, g);

  ttt_draw_str(renderer, 20.0f, (float)g->win_h - 28.0f, 1.85f, "1 2 3 DIFFICULTY R NEW GAME", sub);

  if (g->last_outcome != TTT_IN_PROGRESS) {
    int ww = g->win_w;
    int hh = g->win_h;
    SDL_FRect dim = {0.0f, 0.0f, (float)ww, (float)hh};
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0x2C, 0x3E, 0x50, 190);
    SDL_RenderFillRectF(renderer, &dim);

    const char *msg = "DRAW";
    if (g->last_outcome == TTT_X_WINS) {
      msg = "YOU WIN";
    } else if (g->last_outcome == TTT_O_WINS) {
      msg = "AI WINS";
    }
    SDL_Color ink = {255, 255, 255, 255};
    float px = 3.4f;
    float mw = ttt_text_width(px, msg);
    ttt_draw_str(renderer, ((float)ww - mw) * 0.5f, (float)hh * 0.42f, px, msg, ink);
    const char *hint = "ENTER OR SPACE OR R";
    float hp = 2.2f;
    float hw = ttt_text_width(hp, hint);
    ttt_draw_str(renderer, ((float)ww - hw) * 0.5f, (float)hh * 0.42f + 8.0f * px, hp, hint, sub);
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
