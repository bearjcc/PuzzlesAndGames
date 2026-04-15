#ifndef PG_SDL_H
#define PG_SDL_H

#include <SDL.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Map window-pixel coordinates to logical renderer coordinates (HiDPI / logical scaling). */
void pg_sdl_window_to_logical(SDL_Renderer *renderer, int window_x, int window_y, float *out_lx, float *out_ly);

/* Output size in renderer pixels (same units as layout in resize/render). Returns false if unavailable. */
bool pg_sdl_renderer_output_size(SDL_Renderer *renderer, int *out_w, int *out_h);

/**
 * Pointer position from a mouse SDL_Event in logical renderer coordinates.
 * Supports SDL_MOUSEMOTION, SDL_MOUSEBUTTONDOWN, SDL_MOUSEBUTTONUP, SDL_MOUSEWHEEL.
 * Returns false for other event types or null outputs.
 */
bool pg_sdl_event_pointer_logical(SDL_Renderer *renderer, const SDL_Event *event, float *out_x, float *out_y);

#ifdef __cplusplus
}
#endif

#endif
