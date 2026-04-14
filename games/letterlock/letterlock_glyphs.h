#ifndef LETTERLOCK_GLYPHS_H
#define LETTERLOCK_GLYPHS_H

#include <SDL.h>

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 5 wide x 7 tall pixel font; step horizontally is 6 * pixel (same convention as pg_digits). */
void letterlock_draw_char(SDL_Renderer *renderer, float x, float y, float pixel, char ch, SDL_Color color);

/* Underscore slot for a hidden letter (not A-Z glyph). */
void letterlock_draw_hidden_slot(SDL_Renderer *renderer, float x, float y, float pixel, SDL_Color color);

#ifdef __cplusplus
}
#endif

#endif
