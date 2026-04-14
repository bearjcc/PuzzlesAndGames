#ifndef PG_TEXT_H
#define PG_TEXT_H

#include <SDL.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Call once per module that needs text rendering; paired with pg_text_unref. */
bool pg_text_ref(void);
void pg_text_unref(void);

/**
 * Draw decimal digits using the bundled font, centered in `box`.
 * Font size is chosen so the string fits inside the box (with a small margin).
 */
void pg_text_draw_uint_centered(SDL_Renderer *renderer, const SDL_FRect *box, uint32_t value, SDL_Color color);

/**
 * Draw a decimal number with nominal cap height `height_px` (clamped), left-aligned from (x, y).
 */
void pg_text_draw_uint(SDL_Renderer *renderer, float x, float y, float height_px, uint32_t value, SDL_Color color);

/** Same sizing rules as pg_text_draw_uint, for short ASCII or UTF-8 labels. */
void pg_text_draw_utf8(SDL_Renderer *renderer, float x, float y, float height_px, const char *utf8, SDL_Color color);

#ifdef __cplusplus
}
#endif

#endif
