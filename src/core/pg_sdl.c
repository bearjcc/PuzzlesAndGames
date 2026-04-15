#include "pg/sdl.h"

#include <stddef.h>

void pg_sdl_window_to_logical(SDL_Renderer *renderer, int window_x, int window_y, float *out_lx, float *out_ly)
{
  if (renderer == NULL || out_lx == NULL || out_ly == NULL) {
    if (out_lx != NULL) {
      *out_lx = (float)window_x;
    }
    if (out_ly != NULL) {
      *out_ly = (float)window_y;
    }
    return;
  }
  SDL_RenderWindowToLogical(renderer, (float)window_x, (float)window_y, out_lx, out_ly);
}

bool pg_sdl_renderer_output_size(SDL_Renderer *renderer, int *out_w, int *out_h)
{
  if (renderer == NULL || out_w == NULL || out_h == NULL) {
    return false;
  }
  if (SDL_GetRendererOutputSize(renderer, out_w, out_h) != 0) {
    return false;
  }
  return true;
}

bool pg_sdl_event_pointer_logical(SDL_Renderer *renderer, const SDL_Event *event, float *out_x, float *out_y)
{
  if (event == NULL || out_x == NULL || out_y == NULL) {
    return false;
  }
  switch (event->type) {
  case SDL_MOUSEMOTION:
    pg_sdl_window_to_logical(renderer, event->motion.x, event->motion.y, out_x, out_y);
    return true;
  case SDL_MOUSEBUTTONDOWN:
  case SDL_MOUSEBUTTONUP:
    pg_sdl_window_to_logical(renderer, event->button.x, event->button.y, out_x, out_y);
    return true;
  case SDL_MOUSEWHEEL:
    pg_sdl_window_to_logical(renderer, event->wheel.x, event->wheel.y, out_x, out_y);
    return true;
  default:
    return false;
  }
}
