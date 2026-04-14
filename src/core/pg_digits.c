#include "pg/digits.h"

#include <SDL.h>

/* 7 rows, 5 columns; each row uses low 5 bits (MSB = left pixel). */
static const uint8_t kDigit5x7[10][7] = {
    {0x1F, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1F}, /* 0 */
    {0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E}, /* 1 */
    {0x1F, 0x01, 0x01, 0x1F, 0x10, 0x10, 0x1F}, /* 2 */
    {0x1F, 0x01, 0x01, 0x0F, 0x01, 0x01, 0x1F}, /* 3 */
    {0x11, 0x11, 0x11, 0x1F, 0x01, 0x01, 0x01}, /* 4 */
    {0x1F, 0x10, 0x10, 0x1F, 0x01, 0x01, 0x1F}, /* 5 */
    {0x0E, 0x10, 0x10, 0x1F, 0x11, 0x11, 0x1F}, /* 6 */
    {0x1F, 0x01, 0x02, 0x04, 0x04, 0x04, 0x04}, /* 7 */
    {0x1F, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x1F}, /* 8 */
    {0x1F, 0x11, 0x11, 0x1F, 0x01, 0x01, 0x1F}, /* 9 */
};

static void pg_digit_draw(SDL_Renderer *renderer, float x, float y, float pixel, int digit, SDL_Color color)
{
  if (digit < 0 || digit > 9) {
    return;
  }
  SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
  for (int row = 0; row < 7; row++) {
    uint8_t bits = kDigit5x7[digit][row];
    for (int col = 0; col < 5; col++) {
      if ((bits >> (4 - col)) & 1) {
        SDL_FRect r;
        r.x = x + (float)col * pixel;
        r.y = y + (float)row * pixel;
        r.w = pixel;
        r.h = pixel;
        SDL_RenderFillRectF(renderer, &r);
      }
    }
  }
}

void pg_digits_draw_uint(SDL_Renderer *renderer, float x, float y, float pixel, uint32_t value, SDL_Color color)
{
  char buf[16];
  int len = SDL_snprintf(buf, sizeof(buf), "%u", value);
  if (len <= 0) {
    return;
  }
  float step = 6.0f * pixel;
  for (int i = 0; i < len; i++) {
    char c = buf[i];
    if (c < '0' || c > '9') {
      continue;
    }
    int d = c - '0';
    pg_digit_draw(renderer, x + (float)i * step, y, pixel, d, color);
  }
}
