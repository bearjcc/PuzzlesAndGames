#include "letterlock_game.h"

#include "letterlock_glyphs.h"
#include "letterlock_puzzle.h"

#include <SDL.h>

#include <string.h>

typedef struct LetterlockGame {
  LetterlockPuzzle puzzle;
  int win_w;
  int win_h;
  float letter_px;
  float phrase_x;
  float phrase_y;
  float phrase_line_step;
} LetterlockGame;

static bool is_az_upper(char c)
{
  return c >= 'A' && c <= 'Z';
}

/* #region Layout helpers */
#define LETTERLOCK_MAX_WORDS 32

typedef struct WordSpan {
  int start;
  int len;
} WordSpan;

static int letterlock_collect_words(const char *phrase, WordSpan *out, int max_out)
{
  int n = 0;
  int i = 0;
  size_t plen = strlen(phrase);
  while (i < (int)plen && n < max_out) {
    while (i < (int)plen && phrase[i] == ' ') {
      i++;
    }
    if (i >= (int)plen) {
      break;
    }
    int s = i;
    while (i < (int)plen && phrase[i] != ' ') {
      i++;
    }
    out[n].start = s;
    out[n].len = i - s;
    n++;
  }
  return n;
}

static void letterlock_layout_phrase(LetterlockGame *g)
{
  float margin = 24.0f;
  float max_w = (float)g->win_w - margin * 2.0f;
  if (max_w < 120.0f) {
    max_w = 120.0f;
  }

  float px = 4.2f;
  float char_step = 6.0f * px;
  float space_step = 3.2f * px;

  WordSpan words[LETTERLOCK_MAX_WORDS];
  int wc = letterlock_collect_words(g->puzzle.phrase, words, LETTERLOCK_MAX_WORDS);

  for (int guard = 0; guard < 24; guard++) {
    float line_h = 8.0f * px;
    float x = margin;
    float y = 120.0f;
    bool ok = true;
    for (int wi = 0; wi < wc; wi++) {
      float w_px = (float)words[wi].len * char_step;
      if (x > margin + 0.5f && x + w_px > margin + max_w) {
        x = margin;
        y += line_h;
      }
      if (x + w_px > margin + max_w) {
        ok = false;
        break;
      }
      x += w_px + space_step;
    }
    if (ok) {
      g->letter_px = px;
      g->phrase_x = margin;
      g->phrase_y = y;
      g->phrase_line_step = line_h;
      return;
    }
    px *= 0.92f;
    if (px < 2.2f) {
      px = 2.2f;
    }
    char_step = 6.0f * px;
    space_step = 3.2f * px;
  }
  g->letter_px = px;
  g->phrase_x = margin;
  g->phrase_y = 120.0f;
  g->phrase_line_step = 8.0f * px;
}
/* #endregion */

static void letterlock_draw_title(SDL_Renderer *r)
{
  const char *title = "LETTERLOCK";
  SDL_Color ink = {0x2C, 0x3E, 0x50, 255};
  float px = 3.2f;
  float x = 24.0f;
  float y = 22.0f;
  float step = 6.0f * px;
  for (const char *p = title; *p != '\0'; p++) {
    letterlock_draw_char(r, x, y, px, *p, ink);
    x += step;
  }
}

static void letterlock_draw_subtitle(SDL_Renderer *r, float y)
{
  SDL_Color muted = {0x5D, 0x6D, 0x7E, 255};
  const char *s = "GUESS THE PHRASE ONE LETTER AT A TIME";
  float px = 2.35f;
  float x = 24.0f;
  float step = 6.0f * px;
  for (const char *p = s; *p != '\0'; p++) {
    if (*p == ' ') {
      x += step * 0.45f;
      continue;
    }
    letterlock_draw_char(r, x, y, px, *p, muted);
    x += step;
  }
}

static void letterlock_draw_try_meter(SDL_Renderer *r, const LetterlockGame *g)
{
  int total = g->puzzle.max_wrong;
  if (total <= 0) {
    return;
  }
  float seg = 14.0f;
  float gap = 5.0f;
  float w = (float)g->win_w;
  float x0 = w - 24.0f - (float)total * seg - (float)(total - 1) * gap;
  float y0 = 26.0f;
  for (int i = 0; i < total; i++) {
    SDL_FRect box;
    box.x = x0 + (float)i * (seg + gap);
    box.y = y0;
    box.w = seg;
    box.h = seg * 0.55f;
    bool lost_segment = i < g->puzzle.wrong_count;
    if (lost_segment) {
      SDL_SetRenderDrawColor(r, 0xE7, 0x4C, 0x3C, 255);
    } else {
      SDL_SetRenderDrawColor(r, 0x27, 0xAE, 0x60, 255);
    }
    SDL_RenderFillRectF(r, &box);
  }
}

static void letterlock_draw_phrase(SDL_Renderer *r, LetterlockGame *g)
{
  float px = g->letter_px;
  float char_step = 6.0f * px;
  float space_step = 3.2f * px;
  float margin = g->phrase_x;
  float max_w = (float)g->win_w - margin * 2.0f;
  if (max_w < 80.0f) {
    max_w = 80.0f;
  }

  SDL_Color ink = {0x2C, 0x3E, 0x50, 255};
  SDL_Color slot = {0x95, 0xA5, 0xA6, 255};

  WordSpan words[LETTERLOCK_MAX_WORDS];
  int wc = letterlock_collect_words(g->puzzle.phrase, words, LETTERLOCK_MAX_WORDS);
  float x = margin;
  float y = g->phrase_y;
  float line_step = g->phrase_line_step;

  for (int wi = 0; wi < wc; wi++) {
    float w_px = (float)words[wi].len * char_step;
    if (x > margin + 0.5f && x + w_px > margin + max_w) {
      x = margin;
      y += line_step;
    }
    for (int k = 0; k < words[wi].len; k++) {
      char ch = g->puzzle.phrase[words[wi].start + k];
      if (is_az_upper(ch)) {
        if (g->puzzle.guessed[(size_t)(ch - 'A')]) {
          letterlock_draw_char(r, x, y, px, ch, ink);
        } else {
          letterlock_draw_hidden_slot(r, x, y, px, slot);
        }
      }
      x += char_step;
    }
    x += space_step;
  }
}

static void letterlock_draw_overlay(SDL_Renderer *r, const LetterlockGame *g, bool win)
{
  int ww = g->win_w;
  int hh = g->win_h;
  SDL_FRect dim = {0.0f, 0.0f, (float)ww, (float)hh};
  SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
  SDL_SetRenderDrawColor(r, 0xEC, 0xF0, 0xF1, 210);
  SDL_RenderFillRectF(r, &dim);

  const char *msg = win ? "UNLOCKED" : "OUT OF GUESSES";
  SDL_Color ink = {0x2C, 0x3E, 0x50, 255};
  float px = 3.6f;
  float step = 6.0f * px;
  float text_w = 0.0f;
  for (const char *p = msg; *p != '\0'; p++) {
    if (*p != ' ') {
      text_w += step;
    } else {
      text_w += step * 0.5f;
    }
  }
  float cx = ((float)ww - text_w) * 0.5f;
  float cy = (float)hh * 0.42f;
  float x = cx;
  for (const char *p = msg; *p != '\0'; p++) {
    if (*p == ' ') {
      x += step * 0.5f;
      continue;
    }
    letterlock_draw_char(r, x, cy, px, *p, ink);
    x += step;
  }

  const char *hint = "ENTER OR SPACE FOR A NEW PHRASE";
  SDL_Color muted = {0x5D, 0x6D, 0x7E, 255};
  float hpx = 2.2f;
  float hstep = 6.0f * hpx;
  float hw = 0.0f;
  for (const char *p = hint; *p != '\0'; p++) {
    if (*p == ' ') {
      hw += hstep * 0.45f;
    } else {
      hw += hstep;
    }
  }
  float hx = ((float)ww - hw) * 0.5f;
  float hy = cy + step * 2.2f;
  for (const char *p = hint; *p != '\0'; p++) {
    if (*p == ' ') {
      hx += hstep * 0.45f;
      continue;
    }
    letterlock_draw_char(r, hx, hy, hpx, *p, muted);
    hx += hstep;
  }

  if (!win) {
    const char *answer_label = "PHRASE";
    float apx = 2.0f;
    float astep = 6.0f * apx;
    float ax = ((float)ww - astep * (float)strlen(answer_label)) * 0.5f;
    float ay = hy + hstep * 2.0f;
    for (const char *p = answer_label; *p != '\0'; p++) {
      letterlock_draw_char(r, ax, ay, apx, *p, muted);
      ax += astep;
    }
    ax = 24.0f;
    ay += astep * 1.2f;
    float max_w = (float)ww - 48.0f;
    float lx = ax;
    float ly = ay;
    for (const char *p = g->puzzle.phrase; *p != '\0'; p++) {
      char c = *p;
      if (c == ' ') {
        lx += astep * 0.55f;
        if (lx > (float)ww - 48.0f - astep * 3.0f) {
          lx = ax;
          ly += astep * 1.25f;
        }
        continue;
      }
      if (lx + astep > ax + max_w) {
        lx = ax;
        ly += astep * 1.25f;
      }
      letterlock_draw_char(r, lx, ly, apx, c, ink);
      lx += astep;
    }
  }
}

static void *letterlock_create(SDL_Renderer *renderer)
{
  LetterlockGame *g = (LetterlockGame *)SDL_calloc(1, sizeof(LetterlockGame));
  if (g == NULL) {
    return NULL;
  }
  int w = 0;
  int h = 0;
  SDL_GetRendererOutputSize(renderer, &w, &h);
  g->win_w = w;
  g->win_h = h;
  uint32_t seed = (uint32_t)SDL_GetTicks();
  letterlock_puzzle_reset(&g->puzzle, seed == 0u ? 1u : seed);
  letterlock_layout_phrase(g);
  return g;
}

static void letterlock_destroy(void *state)
{
  SDL_free(state);
}

static void letterlock_reset_cb(void *state)
{
  LetterlockGame *g = (LetterlockGame *)state;
  uint32_t seed = (uint32_t)SDL_GetTicks();
  letterlock_puzzle_reset(&g->puzzle, seed == 0u ? 1u : seed);
  letterlock_layout_phrase(g);
}

static void letterlock_on_event(void *state, const SDL_Event *event)
{
  LetterlockGame *g = (LetterlockGame *)state;
  if (event->type != SDL_KEYDOWN) {
    return;
  }
  const SDL_Keycode k = event->key.keysym.sym;
  if (k == SDLK_r) {
    letterlock_reset_cb(g);
    return;
  }
  if (g->puzzle.won || g->puzzle.lost) {
    if (k == SDLK_RETURN || k == SDLK_SPACE) {
      letterlock_reset_cb(g);
    }
    return;
  }
  if (k >= SDLK_a && k <= SDLK_z) {
    char ch = (char)('A' + (k - SDLK_a));
    letterlock_puzzle_guess(&g->puzzle, ch);
  }
}

static void letterlock_update(void *state, float dt_seconds)
{
  (void)state;
  (void)dt_seconds;
}

static void letterlock_render(void *state, SDL_Renderer *renderer)
{
  LetterlockGame *g = (LetterlockGame *)state;
  SDL_SetRenderDrawColor(renderer, 0xEC, 0xF0, 0xF1, 255);
  SDL_RenderClear(renderer);

  letterlock_draw_title(renderer);
  letterlock_draw_subtitle(renderer, 58.0f);
  letterlock_draw_try_meter(renderer, g);
  letterlock_draw_phrase(renderer, g);

  const char *help = "A Z TO GUESS R NEW PHRASE";
  SDL_Color help_c = {0x7F, 0x8C, 0x8D, 255};
  float hx = 24.0f;
  float hy = (float)g->win_h - 36.0f;
  float hpx = 2.0f;
  float hstep = 6.0f * hpx;
  for (const char *p = help; *p != '\0'; p++) {
    if (*p == ' ') {
      hx += hstep * 0.45f;
      continue;
    }
    letterlock_draw_char(renderer, hx, hy, hpx, *p, help_c);
    hx += hstep;
  }

  if (g->puzzle.won) {
    letterlock_draw_overlay(renderer, g, true);
  } else if (g->puzzle.lost) {
    letterlock_draw_overlay(renderer, g, false);
  }
}

static void letterlock_resize(void *state, int width_px, int height_px)
{
  LetterlockGame *g = (LetterlockGame *)state;
  g->win_w = width_px;
  g->win_h = height_px;
  letterlock_layout_phrase(g);
}

static const PgGameVtable kLetterlockVt = {
    .id = "letterlock",
    .create = letterlock_create,
    .destroy = letterlock_destroy,
    .reset = letterlock_reset_cb,
    .on_event = letterlock_on_event,
    .update = letterlock_update,
    .render = letterlock_render,
    .resize = letterlock_resize,
};

const PgGameVtable *letterlock_game_vt(void)
{
  return &kLetterlockVt;
}
