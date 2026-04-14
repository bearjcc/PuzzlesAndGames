#include "pg/text.h"

#include <SDL.h>
#include <SDL_ttf.h>

#include <stdbool.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#ifndef PG_ASSET_ROOT
#define PG_ASSET_ROOT "."
#endif

static int s_refcount;
static TTF_Font *s_font;

static bool pg_text_open_font(void)
{
  char path[1024];
  SDL_snprintf(path, sizeof(path), "%s/assets/fonts/SourceSans3-Regular.ttf", PG_ASSET_ROOT);
  s_font = TTF_OpenFont(path, 32);
  if (s_font != NULL) {
    return true;
  }
  const char *base = SDL_GetBasePath();
  if (base != NULL) {
    SDL_snprintf(path, sizeof(path), "%sassets/fonts/SourceSans3-Regular.ttf", base);
    s_font = TTF_OpenFont(path, 32);
  }
  return s_font != NULL;
}

bool pg_text_ref(void)
{
  if (s_refcount == 0) {
    if (TTF_WasInit() == 0 && TTF_Init() != 0) {
      fprintf(stderr, "TTF_Init failed: %s\n", TTF_GetError());
      return false;
    }
    if (!pg_text_open_font()) {
      fprintf(stderr, "TTF_OpenFont failed (%s/assets/fonts/SourceSans3-Regular.ttf): %s\n", PG_ASSET_ROOT, TTF_GetError());
      if (TTF_WasInit() != 0) {
        TTF_Quit();
      }
      return false;
    }
  }
  s_refcount++;
  return true;
}

void pg_text_unref(void)
{
  if (s_refcount <= 0) {
    return;
  }
  s_refcount--;
  if (s_refcount == 0) {
    if (s_font != NULL) {
      TTF_CloseFont(s_font);
      s_font = NULL;
    }
    if (TTF_WasInit() != 0) {
      TTF_Quit();
    }
  }
}

static int pg_text_utf8_len(const char *s)
{
  int n = 0;
  for (const unsigned char *p = (const unsigned char *)s; *p != 0; p++) {
    if ((*p & 0xC0) != 0x80) {
      n++;
    }
  }
  return n;
}

static bool pg_text_fit_size(
    const char *text, float box_w, float box_h, int *out_pt, int *out_w, int *out_h)
{
  if (s_font == NULL || text[0] == '\0') {
    return false;
  }
  float margin = fmaxf(4.0f, box_h * 0.08f);
  float inner_w = box_w - margin * 2.0f;
  float inner_h = box_h - margin * 2.0f;
  if (inner_w < 4.0f || inner_h < 4.0f) {
    return false;
  }
  int lo = 8;
  int hi = (int)fmaxf(12.0f, floorf(inner_h * 1.1f));
  if (hi > 256) {
    hi = 256;
  }
  int best = lo;
  int best_w = 0;
  int best_h = 0;
  while (lo <= hi) {
    int mid = (lo + hi) / 2;
    if (TTF_SetFontSize(s_font, mid) != 0) {
      break;
    }
    int w = 0;
    int h = 0;
    if (TTF_SizeUTF8(s_font, text, &w, &h) != 0) {
      break;
    }
    if (w <= (int)inner_w && h <= (int)inner_h) {
      best = mid;
      best_w = w;
      best_h = h;
      lo = mid + 1;
    } else {
      hi = mid - 1;
    }
  }
  if (best_w == 0) {
    (void)TTF_SetFontSize(s_font, best);
    if (TTF_SizeUTF8(s_font, text, &best_w, &best_h) != 0) {
      return false;
    }
  } else {
    (void)TTF_SetFontSize(s_font, best);
    if (TTF_SizeUTF8(s_font, text, &best_w, &best_h) != 0) {
      return false;
    }
  }
  *out_pt = best;
  *out_w = best_w;
  *out_h = best_h;
  return true;
}

void pg_text_draw_uint_centered(SDL_Renderer *renderer, const SDL_FRect *box, uint32_t value, SDL_Color color)
{
  if (s_font == NULL || renderer == NULL || box == NULL) {
    return;
  }
  char buf[32];
  SDL_snprintf(buf, sizeof(buf), "%u", value);
  if (buf[0] == '\0') {
    return;
  }
  int pt = 0;
  int tw = 0;
  int th = 0;
  if (!pg_text_fit_size(buf, box->w, box->h, &pt, &tw, &th)) {
    return;
  }
  (void)TTF_SetFontSize(s_font, pt);
  SDL_Surface *surf = TTF_RenderUTF8_Blended(s_font, buf, color);
  if (surf == NULL) {
    return;
  }
  SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
  SDL_FreeSurface(surf);
  if (tex == NULL) {
    return;
  }
  SDL_SetTextureScaleMode(tex, SDL_ScaleModeLinear);
  SDL_FRect dst;
  dst.w = (float)tw;
  dst.h = (float)th;
  dst.x = box->x + (box->w - dst.w) * 0.5f;
  dst.y = box->y + (box->h - dst.h) * 0.5f;
  SDL_RenderCopyF(renderer, tex, NULL, &dst);
  SDL_DestroyTexture(tex);
}

static void pg_text_draw_utf8_sized(
    SDL_Renderer *renderer, float x, float y, float height_px, const char *text, SDL_Color color)
{
  if (s_font == NULL || renderer == NULL || text == NULL || text[0] == '\0') {
    return;
  }
  int n = pg_text_utf8_len(text);
  if (n <= 0) {
    n = 1;
  }
  float per = height_px / 1.35f;
  float approx_w = per * (float)n * 0.62f + height_px * 0.5f;
  SDL_FRect box;
  box.x = x;
  box.y = y;
  box.w = fmaxf(approx_w, height_px * 0.8f);
  box.h = fmaxf(height_px * 1.2f, 12.0f);
  int pt = 0;
  int tw = 0;
  int th = 0;
  if (!pg_text_fit_size(text, box.w, box.h, &pt, &tw, &th)) {
    return;
  }
  (void)TTF_SetFontSize(s_font, pt);
  SDL_Surface *surf = TTF_RenderUTF8_Blended(s_font, text, color);
  if (surf == NULL) {
    return;
  }
  SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
  SDL_FreeSurface(surf);
  if (tex == NULL) {
    return;
  }
  SDL_SetTextureScaleMode(tex, SDL_ScaleModeLinear);
  SDL_FRect dst;
  dst.w = (float)tw;
  dst.h = (float)th;
  dst.x = x;
  dst.y = y + (box.h - dst.h) * 0.5f;
  SDL_RenderCopyF(renderer, tex, NULL, &dst);
  SDL_DestroyTexture(tex);
}

void pg_text_draw_uint(SDL_Renderer *renderer, float x, float y, float height_px, uint32_t value, SDL_Color color)
{
  char buf[32];
  SDL_snprintf(buf, sizeof(buf), "%u", value);
  pg_text_draw_utf8_sized(renderer, x, y, height_px, buf, color);
}

void pg_text_draw_utf8(SDL_Renderer *renderer, float x, float y, float height_px, const char *utf8, SDL_Color color)
{
  pg_text_draw_utf8_sized(renderer, x, y, height_px, utf8, color);
}
