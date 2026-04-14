#include "stp_sdl_game.h"

#include "puzzles.h"

#include <SDL.h>
#include <SDL_ttf.h>

#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

#ifndef PG_STP_FONT_FILENAME
#define PG_STP_FONT_FILENAME "SourceSans3-wght.ttf"
#endif

/* #region midend / game selection */
extern const game *gamelist[];
extern const int gamecount;

static const game *stp_pick_game(const char *id)
{
  if (id == NULL || id[0] == '\0') {
    return gamelist[0];
  }
  for (int i = 0; i < gamecount; i++) {
    if (strcmp(gamelist[i]->name, id) == 0) {
      return gamelist[i];
    }
  }
  return gamelist[0];
}
/* #endregion */

/* #region fatal / random seed */
void fatal(char *fmt, ...)
{
  va_list ap;
  fprintf(stderr, "fatal: ");
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);
  fprintf(stderr, "\n");
  exit(1);
}

void get_random_seed(void **randseed, int *randseedsize)
{
  struct timeval *tvp = snew(struct timeval);
  gettimeofday(tvp, NULL);
  *randseed = (void *)tvp;
  *randseedsize = (int)sizeof(struct timeval);
}
/* #endregion */

/* #region SDL drawing state */
typedef struct StpBlitter {
  SDL_Texture *tex;
  int w;
  int h;
  int saved_x;
  int saved_y;
} StpBlitter;

typedef struct StpSdlFrontend {
  SDL_Renderer *renderer;
  TTF_Font *font;
  char *font_path_owned;
  midend *me;
  float *colours;
  int ncolours;

  SDL_Texture *puzzle_tex;
  int puzzle_w;
  int puzzle_h;
  int ox;
  int oy;
  int win_w;
  int win_h;

  SDL_Rect clip;
  bool clip_active;

  float line_width;
  int line_dotted;

  char status[512];

  bool in_frame;
} StpSdlFrontend;

typedef struct StpSdlGame {
  StpSdlFrontend fe;
  bool timer_armed;
  float timer_accum;
} StpSdlGame;

static void stp_colour_rgba(const StpSdlFrontend *fe, int index, Uint8 *r, Uint8 *g, Uint8 *b, Uint8 *a)
{
  if (fe->colours == NULL || index < 0 || index >= fe->ncolours) {
    *r = *g = *b = 200;
    *a = 255;
    return;
  }
  float rf = fe->colours[index * 3 + 0];
  float gf = fe->colours[index * 3 + 1];
  float bf = fe->colours[index * 3 + 2];
  if (rf < 0.0f) {
    rf = 0.0f;
  }
  if (gf < 0.0f) {
    gf = 0.0f;
  }
  if (bf < 0.0f) {
    bf = 0.0f;
  }
  if (rf > 1.0f) {
    rf = 1.0f;
  }
  if (gf > 1.0f) {
    gf = 1.0f;
  }
  if (bf > 1.0f) {
    bf = 1.0f;
  }
  *r = (Uint8)(rf * 255.0f + 0.5f);
  *g = (Uint8)(gf * 255.0f + 0.5f);
  *b = (Uint8)(bf * 255.0f + 0.5f);
  *a = 255;
}

static void stp_set_draw_colour(StpSdlFrontend *fe, int colour)
{
  Uint8 r, g, b, a;
  stp_colour_rgba(fe, colour, &r, &g, &b, &a);
  SDL_SetRenderDrawColor(fe->renderer, r, g, b, a);
}

void frontend_default_colour(frontend *unused_fe, float *output)
{
  (void)unused_fe;
  output[0] = 0.9f;
  output[1] = 0.9f;
  output[2] = 0.9f;
}

static char *stp_resolve_font_path(const char *argv0)
{
  const char *env = getenv("PG_STP_FONT");
  if (env != NULL && env[0] != '\0') {
    return dupstr(env);
  }
  char *base = SDL_GetBasePath();
  if (base != NULL) {
    size_t n = strlen(base) + strlen("assets/fonts/") + strlen(PG_STP_FONT_FILENAME) + 1u;
    char *p = (char *)malloc(n);
    if (p != NULL) {
      snprintf(p, n, "%sassets/fonts/%s", base, PG_STP_FONT_FILENAME);
      FILE *fp = fopen(p, "rb");
      if (fp != NULL) {
        fclose(fp);
        SDL_free(base);
        return p;
      }
      free(p);
    }
    SDL_free(base);
  }
  (void)argv0;
  return dupstr("assets/fonts/" PG_STP_FONT_FILENAME);
}

static void stp_refresh_colours(StpSdlFrontend *fe)
{
  sfree(fe->colours);
  fe->colours = midend_colours(fe->me, &fe->ncolours);
}

static void stp_layout_puzzle(StpSdlGame *g, int win_w, int win_h)
{
  StpSdlFrontend *fe = &g->fe;
  fe->win_w = win_w;
  fe->win_h = win_h;
  int x = win_w;
  int y = win_h;
  midend_size(fe->me, &x, &y, 0);
  fe->puzzle_w = x;
  fe->puzzle_h = y;
  fe->ox = (win_w - x) / 2;
  fe->oy = (win_h - y) / 2;
  if (fe->ox < 0) {
    fe->ox = 0;
  }
  if (fe->oy < 0) {
    fe->oy = 0;
  }

  if (fe->puzzle_tex != NULL) {
    SDL_DestroyTexture(fe->puzzle_tex);
    fe->puzzle_tex = NULL;
  }
  fe->puzzle_tex = SDL_CreateTexture(fe->renderer, SDL_PIXELFORMAT_RGBA8888,
      SDL_TEXTUREACCESS_TARGET, fe->puzzle_w, fe->puzzle_h);
  if (fe->puzzle_tex == NULL) {
    fprintf(stderr, "stp: SDL_CreateTexture failed: %s\n", SDL_GetError());
  }
  stp_refresh_colours(fe);
  midend_force_redraw(fe->me);
}

void deactivate_timer(frontend *fe_in)
{
  StpSdlGame *g = (StpSdlGame *)fe_in;
  g->timer_armed = false;
  g->timer_accum = 0.0f;
}

void activate_timer(frontend *fe_in)
{
  StpSdlGame *g = (StpSdlGame *)fe_in;
  if (!g->timer_armed) {
    g->timer_armed = true;
    g->timer_accum = 0.0f;
  }
}

/* #endregion */

/* #region drawing_api */
static void stp_start_draw(void *handle)
{
  StpSdlFrontend *fe = (StpSdlFrontend *)handle;
  fe->in_frame = true;
  fe->clip_active = false;
  fe->line_width = 1.0f;
  fe->line_dotted = 0;
  if (fe->puzzle_tex == NULL) {
    return;
  }
  SDL_SetRenderTarget(fe->renderer, fe->puzzle_tex);
  stp_set_draw_colour(fe, 0);
  SDL_RenderClear(fe->renderer);
}

static void stp_end_draw(void *handle)
{
  StpSdlFrontend *fe = (StpSdlFrontend *)handle;
  fe->in_frame = false;
  SDL_SetRenderTarget(fe->renderer, NULL);
  SDL_RenderSetClipRect(fe->renderer, NULL);
}

static void stp_draw_update(void *handle, int x, int y, int w, int h)
{
  (void)handle;
  (void)x;
  (void)y;
  (void)w;
  (void)h;
}

static void stp_clip(void *handle, int x, int y, int w, int h)
{
  StpSdlFrontend *fe = (StpSdlFrontend *)handle;
  fe->clip_active = true;
  fe->clip.x = x;
  fe->clip.y = y;
  fe->clip.w = w;
  fe->clip.h = h;
  SDL_RenderSetClipRect(fe->renderer, &fe->clip);
}

static void stp_unclip(void *handle)
{
  StpSdlFrontend *fe = (StpSdlFrontend *)handle;
  fe->clip_active = false;
  SDL_RenderSetClipRect(fe->renderer, NULL);
}

static void stp_status_bar(void *handle, char *text)
{
  StpSdlFrontend *fe = (StpSdlFrontend *)handle;
  if (text == NULL) {
    fe->status[0] = '\0';
    return;
  }
  snprintf(fe->status, sizeof(fe->status), "%s", text);
}

static char *stp_text_fallback(void *handle, const char *const *strings, int nstrings)
{
  (void)handle;
  if (nstrings <= 0 || strings == NULL) {
    return dupstr("");
  }
  return dupstr(strings[0]);
}

static void stp_draw_rect(void *handle, int x, int y, int w, int h, int colour)
{
  StpSdlFrontend *fe = (StpSdlFrontend *)handle;
  stp_set_draw_colour(fe, colour);
  SDL_Rect r = {x, y, w, h};
  SDL_RenderFillRect(fe->renderer, &r);
}

static void stp_draw_line(void *handle, int x1, int y1, int x2, int y2, int colour)
{
  StpSdlFrontend *fe = (StpSdlFrontend *)handle;
  stp_set_draw_colour(fe, colour);
  SDL_RenderDrawLine(fe->renderer, x1, y1, x2, y2);
}

static void stp_draw_thick_line(void *handle, float thickness,
    float x1, float y1, float x2, float y2, int colour)
{
  StpSdlFrontend *fe = (StpSdlFrontend *)handle;
  float dx = x2 - x1;
  float dy = y2 - y1;
  float len = sqrtf(dx * dx + dy * dy);
  if (len < 0.001f) {
    return;
  }
  float nx = -dy / len * thickness * 0.5f;
  float ny = dx / len * thickness * 0.5f;
  SDL_Vertex v[4];
  Uint8 r, g, b, a;
  stp_colour_rgba(fe, colour, &r, &g, &b, &a);
  v[0].position.x = x1 + nx;
  v[0].position.y = y1 + ny;
  v[1].position.x = x2 + nx;
  v[1].position.y = y2 + ny;
  v[2].position.x = x2 - nx;
  v[2].position.y = y2 - ny;
  v[3].position.x = x1 - nx;
  v[3].position.y = y1 - ny;
  for (int i = 0; i < 4; i++) {
    v[i].color.r = r;
    v[i].color.g = g;
    v[i].color.b = b;
    v[i].color.a = a;
    v[i].tex_coord.x = 0.0f;
    v[i].tex_coord.y = 0.0f;
  }
  int indices[6] = {0, 1, 2, 0, 2, 3};
  SDL_RenderGeometry(fe->renderer, NULL, v, 4, indices, 6);
}

static void stp_draw_circle(void *handle, int cx, int cy, int radius,
    int fillcolour, int outlinecolour)
{
  StpSdlFrontend *fe = (StpSdlFrontend *)handle;
  const int n = 36;
  if (radius < 1) {
    return;
  }
  if (fillcolour >= 0) {
    SDL_Vertex *v = (SDL_Vertex *)malloc(sizeof(SDL_Vertex) * (size_t)(n + 1));
    int *idx = (int *)malloc(sizeof(int) * (size_t)(n * 3));
    if (v != NULL && idx != NULL) {
      Uint8 r, g, b, a;
      stp_colour_rgba(fe, fillcolour, &r, &g, &b, &a);
      v[0].position.x = (float)cx;
      v[0].position.y = (float)cy;
      v[0].color.r = r;
      v[0].color.g = g;
      v[0].color.b = b;
      v[0].color.a = a;
      v[0].tex_coord.x = 0.0f;
      v[0].tex_coord.y = 0.0f;
      for (int i = 0; i < n; i++) {
        float ang = 2.0f * 3.14159265f * (float)i / (float)n;
        v[i + 1].position.x = (float)cx + cosf(ang) * (float)radius;
        v[i + 1].position.y = (float)cy + sinf(ang) * (float)radius;
        v[i + 1].color = v[0].color;
        v[i + 1].tex_coord.x = 0.0f;
        v[i + 1].tex_coord.y = 0.0f;
      }
      int p = 0;
      for (int i = 0; i < n; i++) {
        idx[p++] = 0;
        idx[p++] = 1 + i;
        idx[p++] = 1 + ((i + 1) % n);
      }
      SDL_RenderGeometry(fe->renderer, NULL, v, n + 1, idx, p);
    }
    free(v);
    free(idx);
  }
  if (outlinecolour >= 0) {
    stp_set_draw_colour(fe, outlinecolour);
    for (int i = 0; i < n; i++) {
      float a0 = 2.0f * 3.14159265f * (float)i / (float)n;
      float a1 = 2.0f * 3.14159265f * (float)(i + 1) / (float)n;
      int x0 = cx + (int)(cosf(a0) * (float)radius + 0.5f);
      int y0 = cy + (int)(sinf(a0) * (float)radius + 0.5f);
      int x1 = cx + (int)(cosf(a1) * (float)radius + 0.5f);
      int y1 = cy + (int)(sinf(a1) * (float)radius + 0.5f);
      SDL_RenderDrawLine(fe->renderer, x0, y0, x1, y1);
    }
  }
}

static void stp_draw_poly(void *handle, int *coords, int npoints,
    int fillcolour, int outlinecolour)
{
  StpSdlFrontend *fe = (StpSdlFrontend *)handle;
  if (npoints < 2) {
    return;
  }
  if (fillcolour >= 0 && npoints >= 3) {
    SDL_Vertex *v = (SDL_Vertex *)malloc(sizeof(SDL_Vertex) * (size_t)npoints);
    int *idx = (int *)malloc(sizeof(int) * (size_t)((npoints - 2) * 3));
    if (v != NULL && idx != NULL) {
      Uint8 r, g, b, a;
      stp_colour_rgba(fe, fillcolour, &r, &g, &b, &a);
      for (int i = 0; i < npoints; i++) {
        v[i].position.x = (float)coords[i * 2];
        v[i].position.y = (float)coords[i * 2 + 1];
        v[i].color.r = r;
        v[i].color.g = g;
        v[i].color.b = b;
        v[i].color.a = a;
        v[i].tex_coord.x = 0.0f;
        v[i].tex_coord.y = 0.0f;
      }
      int p = 0;
      for (int i = 1; i < npoints - 1; i++) {
        idx[p++] = 0;
        idx[p++] = i;
        idx[p++] = i + 1;
      }
      SDL_RenderGeometry(fe->renderer, NULL, v, npoints, idx, p);
    }
    free(v);
    free(idx);
  }
  if (outlinecolour >= 0) {
    stp_set_draw_colour(fe, outlinecolour);
    for (int i = 0; i < npoints; i++) {
      int j = (i + 1) % npoints;
      SDL_RenderDrawLine(fe->renderer, coords[i * 2], coords[i * 2 + 1],
          coords[j * 2], coords[j * 2 + 1]);
    }
  }
}

static void stp_draw_text(void *handle, int x, int y, int fonttype, int fontsize,
    int align, int colour, char *text)
{
  StpSdlFrontend *fe = (StpSdlFrontend *)handle;
  if (fe->font == NULL || text == NULL) {
    return;
  }
  if (fontsize < 4) {
    fontsize = 4;
  }
  TTF_SetFontSize(fe->font, fontsize);
  SDL_Color fc;
  stp_colour_rgba(fe, colour, &fc.r, &fc.g, &fc.b, &fc.a);
  SDL_Surface *surf = TTF_RenderUTF8_Blended(fe->font, text, fc);
  if (surf == NULL) {
    return;
  }
  SDL_Texture *tex = SDL_CreateTextureFromSurface(fe->renderer, surf);
  SDL_FreeSurface(surf);
  if (tex == NULL) {
    return;
  }
  int tw = 0;
  int th = 0;
  SDL_QueryTexture(tex, NULL, NULL, &tw, &th);
  int dx = x;
  int dy = y;
  if (align & ALIGN_VCENTRE) {
    dy -= th / 2;
  } else {
    dy -= th;
  }
  if (align & ALIGN_HCENTRE) {
    dx -= tw / 2;
  } else if (align & ALIGN_HRIGHT) {
    dx -= tw;
  }
  SDL_Rect dst = {dx, dy, tw, th};
  SDL_RenderCopy(fe->renderer, tex, NULL, &dst);
  SDL_DestroyTexture(tex);
  (void)fonttype;
}

static blitter *stp_blitter_new(void *handle, int w, int h)
{
  StpSdlFrontend *fe = (StpSdlFrontend *)handle;
  StpBlitter *bl = snew(StpBlitter);
  bl->w = w;
  bl->h = h;
  bl->saved_x = 0;
  bl->saved_y = 0;
  bl->tex = SDL_CreateTexture(fe->renderer, SDL_PIXELFORMAT_RGBA8888,
      SDL_TEXTUREACCESS_TARGET, w, h);
  if (bl->tex == NULL) {
    sfree(bl);
    return NULL;
  }
  SDL_SetTextureBlendMode(bl->tex, SDL_BLENDMODE_BLEND);
  return (blitter *)bl;
}

static void stp_blitter_free(void *handle, blitter *bl_in)
{
  StpBlitter *bl = (StpBlitter *)bl_in;
  (void)handle;
  if (bl != NULL) {
    if (bl->tex != NULL) {
      SDL_DestroyTexture(bl->tex);
    }
    sfree(bl);
  }
}

static void stp_blitter_save(void *handle, blitter *bl_in, int x, int y)
{
  StpSdlFrontend *fe = (StpSdlFrontend *)handle;
  StpBlitter *bl = (StpBlitter *)bl_in;
  bl->saved_x = x;
  bl->saved_y = y;
  SDL_Texture *prev = SDL_GetRenderTarget(fe->renderer);
  SDL_SetRenderTarget(fe->renderer, bl->tex);
  SDL_SetRenderDrawBlendMode(fe->renderer, SDL_BLENDMODE_NONE);
  SDL_SetRenderDrawColor(fe->renderer, 0, 0, 0, 0);
  SDL_RenderClear(fe->renderer);
  SDL_SetRenderDrawBlendMode(fe->renderer, SDL_BLENDMODE_BLEND);
  SDL_Rect sr = {x, y, bl->w, bl->h};
  SDL_RenderCopy(fe->renderer, fe->puzzle_tex, &sr, NULL);
  SDL_SetRenderTarget(fe->renderer, prev);
}

static void stp_blitter_load(void *handle, blitter *bl_in, int x, int y)
{
  StpSdlFrontend *fe = (StpSdlFrontend *)handle;
  StpBlitter *bl = (StpBlitter *)bl_in;
  int dx = x;
  int dy = y;
  if (x == BLITTER_FROMSAVED && y == BLITTER_FROMSAVED) {
    dx = bl->saved_x;
    dy = bl->saved_y;
  }
  SDL_Rect dr = {dx, dy, bl->w, bl->h};
  SDL_RenderCopy(fe->renderer, bl->tex, NULL, &dr);
}

static const drawing_api stp_sdl_drawing = {
  stp_draw_text,
  stp_draw_rect,
  stp_draw_line,
  stp_draw_poly,
  stp_draw_circle,
  stp_draw_update,
  stp_clip,
  stp_unclip,
  stp_start_draw,
  stp_end_draw,
  stp_status_bar,
  stp_blitter_new,
  stp_blitter_free,
  stp_blitter_save,
  stp_blitter_load,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  stp_text_fallback,
  stp_draw_thick_line,
};

/* #endregion */

/* #region PgGame vtable */
static int stp_sdl_key_to_button(const SDL_KeyboardEvent *key)
{
  SDL_Keycode sym = key->keysym.sym;
  Uint16 mod = key->keysym.mod;
  int shift = (mod & KMOD_SHIFT) ? MOD_SHFT : 0;
  int ctrl = (mod & KMOD_CTRL) ? MOD_CTRL : 0;

  if (sym == SDLK_UP) {
    return shift | ctrl | CURSOR_UP;
  }
  if (sym == SDLK_DOWN) {
    return shift | ctrl | CURSOR_DOWN;
  }
  if (sym == SDLK_LEFT) {
    return shift | ctrl | CURSOR_LEFT;
  }
  if (sym == SDLK_RIGHT) {
    return shift | ctrl | CURSOR_RIGHT;
  }
  if (sym == SDLK_KP_8) {
    return MOD_NUM_KEYPAD | '8';
  }
  if (sym == SDLK_KP_2) {
    return MOD_NUM_KEYPAD | '2';
  }
  if (sym == SDLK_KP_4) {
    return MOD_NUM_KEYPAD | '4';
  }
  if (sym == SDLK_KP_6) {
    return MOD_NUM_KEYPAD | '6';
  }
  if (sym == SDLK_KP_7) {
    return MOD_NUM_KEYPAD | '7';
  }
  if (sym == SDLK_KP_1) {
    return MOD_NUM_KEYPAD | '1';
  }
  if (sym == SDLK_KP_9) {
    return MOD_NUM_KEYPAD | '9';
  }
  if (sym == SDLK_KP_3) {
    return MOD_NUM_KEYPAD | '3';
  }
  if (sym == SDLK_KP_0) {
    return MOD_NUM_KEYPAD | '0';
  }
  if (sym == SDLK_KP_5) {
    return MOD_NUM_KEYPAD | '5';
  }
  if (sym == SDLK_BACKSPACE || sym == SDLK_DELETE) {
    return '\177';
  }
  if (sym >= SDLK_SPACE && sym <= SDLK_z && (sym < 128)) {
    return (int)sym;
  }
  return -1;
}

static void *stp_sdl_create(SDL_Renderer *renderer)
{
  if (TTF_Init() != 0) {
    fprintf(stderr, "stp: TTF_Init failed: %s\n", TTF_GetError());
    return NULL;
  }
  StpSdlGame *g = (StpSdlGame *)SDL_calloc(1, sizeof(StpSdlGame));
  if (g == NULL) {
    TTF_Quit();
    return NULL;
  }
  g->fe.renderer = renderer;
  g->fe.font_path_owned = stp_resolve_font_path(NULL);
  g->fe.font = TTF_OpenFont(g->fe.font_path_owned, 16);
  if (g->fe.font == NULL) {
    fprintf(stderr, "stp: TTF_OpenFont failed (%s): %s\n", g->fe.font_path_owned, TTF_GetError());
  }

  const game *game = stp_pick_game(getenv("PG_STP_GAME"));
  g->fe.me = midend_new((frontend *)g, game, &stp_sdl_drawing, &g->fe);
  if (g->fe.me == NULL) {
    if (g->fe.font != NULL) {
      TTF_CloseFont(g->fe.font);
    }
    free(g->fe.font_path_owned);
    SDL_free(g);
    TTF_Quit();
    return NULL;
  }

  int w = 0;
  int h = 0;
  SDL_GetRendererOutputSize(renderer, &w, &h);
  stp_layout_puzzle(g, w, h);
  midend_new_game(g->fe.me);
  stp_layout_puzzle(g, w, h);
  return g;
}

static void stp_sdl_destroy(void *state)
{
  StpSdlGame *g = (StpSdlGame *)state;
  deactivate_timer((frontend *)g);
  if (g->fe.me != NULL) {
    midend_free(g->fe.me);
    g->fe.me = NULL;
  }
  sfree(g->fe.colours);
  if (g->fe.puzzle_tex != NULL) {
    SDL_DestroyTexture(g->fe.puzzle_tex);
    g->fe.puzzle_tex = NULL;
  }
  if (g->fe.font != NULL) {
    TTF_CloseFont(g->fe.font);
  }
  free(g->fe.font_path_owned);
  SDL_free(g);
  TTF_Quit();
}

static void stp_sdl_reset(void *state)
{
  StpSdlGame *g = (StpSdlGame *)state;
  midend_restart_game(g->fe.me);
  stp_refresh_colours(&g->fe);
  midend_force_redraw(g->fe.me);
}

static void stp_sdl_on_event(void *state, const SDL_Event *event)
{
  StpSdlGame *g = (StpSdlGame *)state;
  StpSdlFrontend *fe = &g->fe;
  if (fe->me == NULL) {
    return;
  }
  if (event->type == SDL_MOUSEBUTTONDOWN || event->type == SDL_MOUSEBUTTONUP) {
    int button = -1;
    Uint32 mods = SDL_GetModState();
    if (event->button.button == SDL_BUTTON_MIDDLE ||
        (event->button.button == SDL_BUTTON_LEFT && (mods & KMOD_SHIFT))) {
      button = MIDDLE_BUTTON;
    } else if (event->button.button == SDL_BUTTON_RIGHT ||
        (event->button.button == SDL_BUTTON_LEFT && (mods & KMOD_ALT))) {
      button = RIGHT_BUTTON;
    } else if (event->button.button == SDL_BUTTON_LEFT) {
      button = LEFT_BUTTON;
    }
    if (button < 0) {
      return;
    }
    if (event->type == SDL_MOUSEBUTTONUP) {
      button += LEFT_RELEASE - LEFT_BUTTON;
    }
    int mx = event->button.x - fe->ox;
    int my = event->button.y - fe->oy;
    midend_process_key(fe->me, mx, my, button);
  } else if (event->type == SDL_MOUSEMOTION) {
    if ((event->motion.state & SDL_BUTTON_LMASK) == 0 &&
        (event->motion.state & SDL_BUTTON_MMASK) == 0 &&
        (event->motion.state & SDL_BUTTON_RMASK) == 0) {
      return;
    }
    int button = LEFT_DRAG;
    if (event->motion.state & SDL_BUTTON_MMASK) {
      button = MIDDLE_DRAG;
    } else if (event->motion.state & SDL_BUTTON_RMASK) {
      button = RIGHT_DRAG;
    }
    int mx = event->motion.x - fe->ox;
    int my = event->motion.y - fe->oy;
    midend_process_key(fe->me, mx, my, button);
  } else if (event->type == SDL_KEYDOWN) {
    if (event->key.keysym.sym == SDLK_F2) {
      midend_new_game(fe->me);
      stp_layout_puzzle(g, fe->win_w, fe->win_h);
      return;
    }
    int kv = stp_sdl_key_to_button(&event->key);
    if (kv >= 0) {
      midend_process_key(fe->me, 0, 0, kv);
    }
  }
}

static void stp_sdl_update(void *state, float dt)
{
  StpSdlGame *g = (StpSdlGame *)state;
  if (g->fe.me != NULL && g->timer_armed) {
    g->timer_accum += dt;
    while (g->timer_accum >= 0.02f) {
      midend_timer(g->fe.me, 0.02f);
      g->timer_accum -= 0.02f;
    }
  }
}

static void stp_sdl_render(void *state, SDL_Renderer *renderer)
{
  StpSdlGame *g = (StpSdlGame *)state;
  StpSdlFrontend *fe = &g->fe;
  SDL_SetRenderDrawColor(renderer, 40, 44, 52, 255);
  SDL_RenderClear(renderer);
  if (fe->puzzle_tex != NULL) {
    SDL_Rect dst = {fe->ox, fe->oy, fe->puzzle_w, fe->puzzle_h};
    SDL_RenderCopy(renderer, fe->puzzle_tex, NULL, &dst);
  }
  if (fe->status[0] != '\0' && fe->font != NULL) {
    TTF_SetFontSize(fe->font, 14);
    SDL_Color white = {240, 240, 240, 255};
    SDL_Surface *surf = TTF_RenderUTF8_Blended(fe->font, fe->status, white);
    if (surf != NULL) {
      SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);
      SDL_FreeSurface(surf);
      if (tex != NULL) {
        int tw = 0;
        int th = 0;
        SDL_QueryTexture(tex, NULL, NULL, &tw, &th);
        SDL_Rect bar = {0, fe->win_h - th - 8, fe->win_w, th + 8};
        SDL_SetRenderDrawColor(renderer, 30, 30, 36, 240);
        SDL_RenderFillRect(renderer, &bar);
        SDL_Rect tdst = {6, fe->win_h - th - 4, tw, th};
        SDL_RenderCopy(renderer, tex, NULL, &tdst);
        SDL_DestroyTexture(tex);
      }
    }
  }
}

static void stp_sdl_resize(void *state, int width_px, int height_px)
{
  StpSdlGame *g = (StpSdlGame *)state;
  stp_layout_puzzle(g, width_px, height_px);
}

static const PgGameVtable stp_sdl_vtable = {
  "stp",
  stp_sdl_create,
  stp_sdl_destroy,
  stp_sdl_reset,
  stp_sdl_on_event,
  stp_sdl_update,
  stp_sdl_render,
  stp_sdl_resize,
};

const PgGameVtable *stp_sdl_game_vt(void)
{
  return &stp_sdl_vtable;
}

/* #endregion */
