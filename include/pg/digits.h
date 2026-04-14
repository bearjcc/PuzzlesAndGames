#ifndef PG_DIGITS_H
#define PG_DIGITS_H

#include <SDL.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void pg_digits_draw_uint(SDL_Renderer *renderer, float x, float y, float pixel, uint32_t value, SDL_Color color);

#ifdef __cplusplus
}
#endif

#endif
