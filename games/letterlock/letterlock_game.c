#include "letterlock_game.h"

#include "letterlock_puzzle.h"
#include "pg/text.h"
#include "pg/theme.h"

#include <SDL.h>

#include <string.h>

typedef struct LetterlockGame {
  LetterlockPuzzle puzzle;
  int win_w;
  int win_h;
  float phrase_cap_px;
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

static float letterlock_char_advance(const char *one_utf8, float cap_px, float *out_line_h)
{
  float w = 0.0f;
  float h = 0.0f;
  if (!pg_text_measure_nominal(one_utf8, cap_px, &w, &h)) {
    if (out_line_h != NULL) {
      *out_line_h = cap_px * 1.2f;
    }
    return cap_px * 0.62f;
  }
  if (out_line_h != NULL) {
    *out_line_h = fmaxf(h * 1.12f, cap_px * 1.05f);
  }
  return w + cap_px * 0.06f;
}

static void letterlock_layout_phrase(LetterlockGame *g)
{
  float margin = 24.0f;
  float max_w = (float)g->win_w - margin * 2.0f;
  if (max_w < 120.0f) {
    max_w = 120.0f;
  }

  float cap = 26.0f;
  WordSpan words[LETTERLOCK_MAX_WORDS];
  int wc = letterlock_collect_words(g->puzzle.phrase, words, LETTERLOCK_MAX_WORDS);

  for (int guard = 0; guard < 28; guard++) {
    float line_h = 0.0f;
    (void)letterlock_char_advance("M", cap, &line_h);
    float space_adv = cap * 0.42f;
    float x = margin;
    float y = 118.0f;
    bool ok = true;
    for (int wi = 0; wi < wc; wi++) {
      float word_w = 0.0f;
      for (int k = 0; k < words[wi].len; k++) {
        char ch = g->puzzle.phrase[words[wi].start + k];
        char buf[2] = {ch, '\0'};
        if (is_az_upper(ch)) {
          word_w += letterlock_char_advance(buf, cap, NULL);
        }
      }
      if (x > margin + 0.5f && x + word_w > margin + max_w) {
        x = margin;
        y += line_h;
      }
      if (x + word_w > margin + max_w) {
        ok = false;
        break;
      }
      for (int k = 0; k < words[wi].len; k++) {
        char ch = g->puzzle.phrase[words[wi].start + k];
        if (is_az_upper(ch)) {
          char buf[2] = {ch, '\0'};
          float adv = letterlock_char_advance(buf, cap, NULL);
          if (x > margin + 0.5f && x + adv > margin + max_w) {
            x = margin;
            y += line_h;
          }
          if (x + adv > margin + max_w) {
            ok = false;
            break;
          }
          x += adv;
        }
        if (!ok) {
          break;
        }
      }
      if (!ok) {
        break;
      }
      x += space_adv;
    }
    if (ok) {
      g->phrase_cap_px = cap;
      g->phrase_x = margin;
      g->phrase_y = y;
      g->phrase_line_step = line_h;
      return;
    }
    cap *= 0.92f;
    if (cap < 11.0f) {
      cap = 11.0f;
    }
  }
  g->phrase_cap_px = cap;
  g->phrase_x = margin;
  g->phrase_y = 118.0f;
  float lh = 0.0f;
  (void)letterlock_char_advance("M", cap, &lh);
  g->phrase_line_step = lh;
}
/* #endregion */

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
    SDL_Color c = lost_segment ? PG_COLOR_STATE_NEGATIVE : PG_COLOR_SURFACE_STRONG;
    SDL_SetRenderDrawColor(r, c.r, c.g, c.b, c.a);
    SDL_RenderFillRectF(r, &box);
  }
}

static void letterlock_draw_phrase(SDL_Renderer *r, LetterlockGame *g)
{
  float cap = g->phrase_cap_px;
  float margin = g->phrase_x;
  float max_w = (float)g->win_w - margin * 2.0f;
  if (max_w < 80.0f) {
    max_w = 80.0f;
  }

  SDL_Color ink = PG_COLOR_INK;
  SDL_Color slot = PG_COLOR_INK_FAINT;

  WordSpan words[LETTERLOCK_MAX_WORDS];
  int wc = letterlock_collect_words(g->puzzle.phrase, words, LETTERLOCK_MAX_WORDS);
  float x = margin;
  float y = g->phrase_y;
  float line_h = g->phrase_line_step;
  float space_adv = cap * 0.42f;

  for (int wi = 0; wi < wc; wi++) {
    float word_w = 0.0f;
    for (int k = 0; k < words[wi].len; k++) {
      char ch = g->puzzle.phrase[words[wi].start + k];
      char buf[2] = {ch, '\0'};
      if (is_az_upper(ch)) {
        word_w += letterlock_char_advance(buf, cap, NULL);
      }
    }
    if (x > margin + 0.5f && x + word_w > margin + max_w) {
      x = margin;
      y += line_h;
    }
    for (int k = 0; k < words[wi].len; k++) {
      char ch = g->puzzle.phrase[words[wi].start + k];
      if (is_az_upper(ch)) {
        char buf[2] = {ch, '\0'};
        float adv = letterlock_char_advance(buf, cap, NULL);
        if (x > margin + 0.5f && x + adv > margin + max_w) {
          x = margin;
          y += line_h;
        }
        if (g->puzzle.guessed[(size_t)(ch - 'A')]) {
          pg_text_draw_utf8(r, x, y, cap, buf, ink);
        } else {
          float sw = 0.0f;
          float sh = 0.0f;
          if (pg_text_measure_nominal(buf, cap, &sw, &sh)) {
            float yy = y + sh - cap * 0.12f;
            SDL_FRect rule = {x, yy, sw, fmaxf(1.5f, cap * 0.06f)};
            SDL_SetRenderDrawColor(r, slot.r, slot.g, slot.b, slot.a);
            SDL_RenderFillRectF(r, &rule);
          }
        }
        x += adv;
      }
    }
    x += space_adv;
  }
}

static void letterlock_draw_overlay(SDL_Renderer *r, const LetterlockGame *g, bool win)
{
  int ww = g->win_w;
  int hh = g->win_h;
  SDL_FRect dim = {0.0f, 0.0f, (float)ww, (float)hh};
  SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
  SDL_Color scrim = PG_COLOR_OVERLAY_SCRIM_LIGHT;
  SDL_SetRenderDrawColor(r, scrim.r, scrim.g, scrim.b, scrim.a);
  SDL_RenderFillRectF(r, &dim);

  const char *msg = win ? "Unlocked" : "Out of guesses";
  SDL_Color ink = PG_COLOR_INK;
  SDL_FRect title_box = {24.0f, (float)hh * 0.38f, (float)ww - 48.0f, 48.0f};
  pg_text_draw_utf8_centered(r, &title_box, msg, ink);

  const char *hint = "Enter or space for a new phrase";
  SDL_FRect hint_box = {40.0f, title_box.y + 52.0f, (float)ww - 80.0f, 32.0f};
  pg_text_draw_utf8_centered(r, &hint_box, hint, PG_COLOR_INK_MUTED);

  if (!win) {
    SDL_FRect lab = {40.0f, hint_box.y + 40.0f, (float)ww - 80.0f, 22.0f};
    pg_text_draw_utf8_centered(r, &lab, "Phrase", PG_COLOR_INK_MUTED);
    SDL_FRect ans = {32.0f, lab.y + 28.0f, (float)ww - 64.0f, (float)hh * 0.22f};
    pg_text_draw_utf8_centered(r, &ans, g->puzzle.phrase, ink);
  }
}

static void *letterlock_create(SDL_Renderer *renderer)
{
  LetterlockGame *g = (LetterlockGame *)SDL_calloc(1, sizeof(LetterlockGame));
  if (g == NULL) {
    return NULL;
  }
  if (!pg_text_ref()) {
    SDL_free(g);
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
  pg_text_unref();
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
  pg_theme_clear_paper(renderer);

  pg_text_draw_utf8(renderer, 24.0f, 20.0f, 26.0f, "Letterlock", PG_COLOR_INK);
  pg_text_draw_utf8(renderer, 24.0f, 54.0f, 15.0f, "Guess the phrase one letter at a time", PG_COLOR_INK_MUTED);

  letterlock_draw_try_meter(renderer, g);
  letterlock_draw_phrase(renderer, g);

  pg_text_draw_utf8(renderer, 24.0f, (float)g->win_h - 34.0f, 14.0f, "A Z to guess   R new phrase", PG_COLOR_INK_FAINT);

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
