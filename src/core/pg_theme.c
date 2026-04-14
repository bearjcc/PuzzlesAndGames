#include "pg/theme.h"

void pg_theme_clear_paper(SDL_Renderer *renderer)
{
  if (renderer == NULL) {
    return;
  }
  SDL_Color c = PG_COLOR_PAPER;
  SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);
  SDL_RenderClear(renderer);
}

float pg_theme_stroke_px(float reference_px, float ratio)
{
  float t = reference_px * ratio;
  if (t < 1.5f) {
    t = 1.5f;
  }
  if (t > 6.0f) {
    t = 6.0f;
  }
  return t;
}
