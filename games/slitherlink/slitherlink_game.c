#include "slitherlink_game.h"

#include "slitherlink_board.h"

#include "pg/app.h"
#include "pg/catalog/pg_catalog.h"
#include "pg/digits.h"
#include "pg/theme.h"

#include <SDL.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct SlitherlinkGame {
  SDL_Renderer *renderer;
  SlitherlinkBoard board;
  int win_w;
  int win_h;
  int grid_x;
  int grid_y;
  float cell;
  float line_hit;
} SlitherlinkGame;

static void sl_layout(SlitherlinkGame *g, int w, int h)
{
  g->win_w = w;
  g->win_h = h;
  int margin = 20;
  int hud = 72;
  int bw = g->board.w;
  int bh = g->board.h;
  int usable_w = w - margin * 2;
  int usable_h = h - margin * 2 - hud;
  if (usable_w < 120) {
    usable_w = 120;
  }
  if (usable_h < 120) {
    usable_h = 120;
  }
  float cw = (float)usable_w / (float)bw;
  float ch = (float)usable_h / (float)bh;
  g->cell = cw < ch ? cw : ch;
  if (g->cell < 18.0f) {
    g->cell = 18.0f;
  }
  float grid_w = g->cell * (float)bw;
  float grid_h = g->cell * (float)bh;
  g->grid_x = (w - (int)grid_w) / 2;
  g->grid_y = hud + (h - hud - (int)grid_h - margin) / 2;
  if (g->grid_y < hud) {
    g->grid_y = hud;
  }
  g->line_hit = g->cell * 0.22f;
  if (g->line_hit < 10.0f) {
    g->line_hit = 10.0f;
  }
}

static void sl_new_puzzle(SlitherlinkGame *g)
{
  g->board.rng = g->board.rng * 1664525u + 1013904223u;
  if (!sl_board_generate(&g->board)) {
    /* Rare: keep seed advancing until a valid puzzle is found */
    for (int k = 0; k < 200 && !sl_board_generate(&g->board); k++) {
      g->board.rng = g->board.rng * 1103515245u + 12345u;
    }
  }
  sl_board_clear_user(&g->board);
  sl_layout(g, g->win_w, g->win_h);

  if (g->renderer != NULL) {
    SDL_Window *win = SDL_RenderGetWindow(g->renderer);
    if (win != NULL) {
      const char *shape = g->board.shape == SL_SHAPE_BLOB ? "blob" : "rect";
      const char *diff = "normal";
      if (g->board.difficulty == SL_DIFF_EASY) {
        diff = "easy";
      } else if (g->board.difficulty == SL_DIFF_HARD) {
        diff = "hard";
      }
      char buf[160];
      (void)snprintf(
          buf,
          sizeof(buf),
          "PuzzlesAndGames - Slitherlink %dx%d %s %s seed %u",
          g->board.w,
          g->board.h,
          shape,
          diff,
          (unsigned)g->board.rng);
      SDL_SetWindowTitle(win, buf);
    }
  }
}

static void *sl_create(SDL_Renderer *renderer)
{
  SlitherlinkGame *g = calloc(1, sizeof(SlitherlinkGame));
  if (g == NULL) {
    return NULL;
  }
  g->renderer = renderer;
  sl_board_init_defaults(&g->board);
  if (!sl_board_alloc(&g->board, g->board.w, g->board.h)) {
    free(g);
    return NULL;
  }
  sl_new_puzzle(g);
  return g;
}

void slitherlink_game_configure(void *state, int w, int h, SlShape shape, SlDifficulty diff, uint32_t seed)
{
  SlitherlinkGame *g = (SlitherlinkGame *)state;
  if (g == NULL) {
    return;
  }
  if (w < 3) {
    w = 3;
  }
  if (h < 3) {
    h = 3;
  }
  if (w > 16) {
    w = 16;
  }
  if (h > 16) {
    h = 16;
  }
  g->board.shape = shape;
  g->board.difficulty = diff;
  g->board.rng = seed ? seed : 1u;
  sl_board_free(&g->board);
  if (!sl_board_alloc(&g->board, w, h)) {
    sl_board_init_defaults(&g->board);
    (void)sl_board_alloc(&g->board, g->board.w, g->board.h);
  }
  sl_new_puzzle(g);
}

static void sl_destroy(void *state)
{
  SlitherlinkGame *g = (SlitherlinkGame *)state;
  if (g == NULL) {
    return;
  }
  sl_board_free(&g->board);
  free(g);
}

static void sl_reset_cb(void *state)
{
  SlitherlinkGame *g = (SlitherlinkGame *)state;
  sl_new_puzzle(g);
}

static bool sl_hit_h(const SlitherlinkGame *g, float x, float y, int *ri, int *ci)
{
  float gx = (float)g->grid_x;
  float gy = (float)g->grid_y;
  float c = g->cell;
  float tol = g->line_hit;
  for (int r = 0; r <= g->board.h; r++) {
    float yy = gy + (float)r * c;
    if (y < yy - tol || y > yy + tol) {
      continue;
    }
    for (int col = 0; col < g->board.w; col++) {
      float x0 = gx + (float)col * c;
      float x1 = gx + (float)(col + 1) * c;
      if (x >= x0 - tol && x <= x1 + tol) {
        *ri = r;
        *ci = col;
        return true;
      }
    }
  }
  return false;
}

static bool sl_hit_v(const SlitherlinkGame *g, float x, float y, int *ri, int *ci)
{
  float gx = (float)g->grid_x;
  float gy = (float)g->grid_y;
  float c = g->cell;
  float tol = g->line_hit;
  for (int col = 0; col <= g->board.w; col++) {
    float xx = gx + (float)col * c;
    if (x < xx - tol || x > xx + tol) {
      continue;
    }
    for (int r = 0; r < g->board.h; r++) {
      float y0 = gy + (float)r * c;
      float y1 = gy + (float)(r + 1) * c;
      if (y >= y0 - tol && y <= y1 + tol) {
        *ri = r;
        *ci = col;
        return true;
      }
    }
  }
  return false;
}

static void sl_on_event(void *state, const SDL_Event *event)
{
  SlitherlinkGame *g = (SlitherlinkGame *)state;
  if (event->type == SDL_MOUSEBUTTONDOWN && event->button.button == SDL_BUTTON_LEFT) {
    float x = (float)event->button.x;
    float y = (float)event->button.y;
    if (g->renderer != NULL) {
      int ren_w = 0;
      int ren_h = 0;
      SDL_GetRendererOutputSize(g->renderer, &ren_w, &ren_h);
      SDL_Window *win = SDL_RenderGetWindow(g->renderer);
      if (win != NULL && ren_w > 0 && ren_h > 0) {
        int win_w = 0;
        int win_h = 0;
        SDL_GetWindowSize(win, &win_w, &win_h);
        if (win_w > 0 && win_h > 0) {
          x *= (float)ren_w / (float)win_w;
          y *= (float)ren_h / (float)win_h;
        }
      }
    }
    int ri, ci;
    float dh = 1e9f;
    float dv = 1e9f;
    int h_ok = 0;
    int v_ok = 0;
    int hr, hc, vr, vc;
    if (sl_hit_h(g, x, y, &hr, &hc)) {
      float yy = (float)g->grid_y + (float)hr * g->cell;
      dh = (y - yy) >= 0 ? (y - yy) : -(y - yy);
      h_ok = 1;
    }
    if (sl_hit_v(g, x, y, &vr, &vc)) {
      float xx = (float)g->grid_x + (float)vc * g->cell;
      dv = (x - xx) >= 0 ? (x - xx) : -(x - xx);
      v_ok = 1;
    }
    if (h_ok && (!v_ok || dh <= dv)) {
      sl_board_toggle_h(&g->board, hr, hc);
    } else if (v_ok) {
      sl_board_toggle_v(&g->board, vr, vc);
    }
  } else if (event->type == SDL_KEYDOWN) {
    SDL_Keycode k = event->key.keysym.sym;
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
    if (k == SDLK_n || k == SDLK_r) {
      sl_new_puzzle(g);
    } else if (k == SDLK_c) {
      sl_board_clear_user(&g->board);
    }
  }
}

static void sl_update(void *state, float dt_seconds)
{
  (void)state;
  (void)dt_seconds;
}

static void sl_draw_edge(SDL_Renderer *r, float x0, float y0, float x1, float y1, SDL_Color col, float thick)
{
  SDL_SetRenderDrawColor(r, col.r, col.g, col.b, col.a);
  float dx = x1 - x0;
  float dy = y1 - y0;
  float len = dx * dx + dy * dy;
  if (len < 1e-6f) {
    return;
  }
  len = (float)SDL_sqrt((double)len);
  dx /= len;
  dy /= len;
  float px = -dy;
  float py = dx;
  float hw = thick * 0.5f;
  SDL_FPoint quad[4];
  quad[0].x = x0 + px * hw;
  quad[0].y = y0 + py * hw;
  quad[1].x = x1 + px * hw;
  quad[1].y = y1 + py * hw;
  quad[2].x = x1 - px * hw;
  quad[2].y = y1 - py * hw;
  quad[3].x = x0 - px * hw;
  quad[3].y = y0 - py * hw;
  SDL_Vertex v[6];
  SDL_Color c = col;
  for (int i = 0; i < 4; i++) {
    v[i].position.x = quad[i].x;
    v[i].position.y = quad[i].y;
    v[i].color = c;
    v[i].tex_coord.x = 0.0f;
    v[i].tex_coord.y = 0.0f;
  }
  int idx[6] = {0, 1, 2, 0, 2, 3};
  SDL_RenderGeometry(r, NULL, v, 4, idx, 6);
}

static void sl_render(void *state, SDL_Renderer *renderer)
{
  SlitherlinkGame *g = (SlitherlinkGame *)state;
  pg_theme_clear_paper(renderer);

  float gx = (float)g->grid_x;
  float gy = (float)g->grid_y;
  float c = g->cell;

  /* Dots at lattice points */
  SDL_Color dot = PG_SLITHER_DOT;
  SDL_SetRenderDrawColor(renderer, dot.r, dot.g, dot.b, dot.a);
  for (int r = 0; r <= g->board.h; r++) {
    for (int col = 0; col <= g->board.w; col++) {
      float cx = gx + (float)col * c;
      float cy = gy + (float)r * c;
      SDL_FRect d = {cx - 2.0f, cy - 2.0f, 4.0f, 4.0f};
      SDL_RenderFillRectF(renderer, &d);
    }
  }

  /* Inactive cells (blob exterior): light wash */
  if (g->board.shape == SL_SHAPE_BLOB) {
    for (int r = 0; r < g->board.h; r++) {
      for (int col = 0; col < g->board.w; col++) {
        if (g->board.cell_active[(size_t)r * (size_t)g->board.w + (size_t)col]) {
          continue;
        }
        SDL_FRect cell = {
            gx + (float)col * c + 1.0f,
            gy + (float)r * c + 1.0f,
            c - 2.0f,
            c - 2.0f,
        };
        SDL_Color wash = PG_SLITHER_BLOB_INACTIVE;
        SDL_SetRenderDrawColor(renderer, wash.r, wash.g, wash.b, wash.a);
        SDL_RenderFillRectF(renderer, &cell);
      }
    }
  }

  /* Clues */
  for (int r = 0; r < g->board.h; r++) {
    for (int col = 0; col < g->board.w; col++) {
      if (g->board.shape == SL_SHAPE_BLOB && !g->board.cell_active[(size_t)r * (size_t)g->board.w + (size_t)col]) {
        continue;
      }
      int clue = g->board.clue[(size_t)r * (size_t)g->board.w + (size_t)col];
      if (clue < 0) {
        continue;
      }
      float cx = gx + ((float)col + 0.5f) * c;
      float cy = gy + ((float)r + 0.5f) * c;
      float px = c * 0.14f;
      if (px < 2.0f) {
        px = 2.0f;
      }
      if (px > 16.0f) {
        px = 16.0f;
      }
      float tw = 6.0f * px;
      pg_digits_draw_uint(
          renderer,
          cx - tw * 0.5f,
          cy - 3.5f * px,
          px,
          (uint32_t)clue,
          PG_SLITHER_CLUE);
    }
  }

  SDL_Color ink_user = PG_SLITHER_EDGE_USER;
  SDL_Color ink_sol = PG_SLITHER_EDGE_SOL;
  float thick = c * 0.09f;
  if (thick < 3.0f) {
    thick = 3.0f;
  }

  int w = g->board.w;
  int hh = g->board.h;
  for (int r = 0; r <= hh; r++) {
    for (int col = 0; col < w; col++) {
      size_t ix = (size_t)r * (size_t)w + (size_t)col;
      if (g->board.h_user[ix]) {
        float x0 = gx + (float)col * c;
        float x1 = gx + (float)(col + 1) * c;
        float y0 = gy + (float)r * c;
        sl_draw_edge(renderer, x0, y0, x1, y0, ink_user, thick);
      }
    }
  }
  for (int r = 0; r < hh; r++) {
    for (int col = 0; col <= w; col++) {
      size_t ix = (size_t)r * (size_t)(w + 1u) + (size_t)col;
      if (g->board.v_user[ix]) {
        float x0 = gx + (float)col * c;
        float y0 = gy + (float)r * c;
        float y1 = gy + (float)(r + 1) * c;
        sl_draw_edge(renderer, x0, y0, x0, y1, ink_user, thick);
      }
    }
  }

  if (g->board.solved) {
    for (int r = 0; r <= hh; r++) {
      for (int col = 0; col < w; col++) {
        size_t ix = (size_t)r * (size_t)w + (size_t)col;
        if (g->board.h_sol[ix]) {
          float x0 = gx + (float)col * c;
          float x1 = gx + (float)(col + 1) * c;
          float y0 = gy + (float)r * c;
          sl_draw_edge(renderer, x0, y0, x1, y0, ink_sol, thick * 0.55f);
        }
      }
    }
    for (int r = 0; r < hh; r++) {
      for (int col = 0; col <= w; col++) {
        size_t ix = (size_t)r * (size_t)(w + 1u) + (size_t)col;
        if (g->board.v_sol[ix]) {
          float x0 = gx + (float)col * c;
          float y0 = gy + (float)r * c;
          float y1 = gy + (float)(r + 1) * c;
          sl_draw_edge(renderer, x0, y0, x0, y1, ink_sol, thick * 0.55f);
        }
      }
    }
  }
}

static void sl_resize(void *state, int width_px, int height_px)
{
  SlitherlinkGame *g = (SlitherlinkGame *)state;
  sl_layout(g, width_px, height_px);
}

static const PgGameVtable kSlVt = {
    .id = "slitherlink",
    .create = sl_create,
    .destroy = sl_destroy,
    .reset = sl_reset_cb,
    .on_event = sl_on_event,
    .update = sl_update,
    .render = sl_render,
    .resize = sl_resize,
};

const PgGameVtable *slitherlink_game_vt(void)
{
  return &kSlVt;
}
