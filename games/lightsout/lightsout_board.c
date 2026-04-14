#include "lightsout_board.h"

#include <string.h>

uint32_t lightsout_rng_next(uint32_t *rng)
{
  uint32_t x = *rng;
  if (x == 0u) {
    x = 0xA341316Cu;
  }
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  *rng = x == 0u ? 1u : x;
  return *rng;
}

static uint32_t lightsout_toggle_mask(int row, int col)
{
  uint32_t m = 0u;
  int idx = row * LIGHTSOUT_SIZE + col;
  m |= 1u << idx;
  if (row > 0) {
    m |= 1u << ((row - 1) * LIGHTSOUT_SIZE + col);
  }
  if (row + 1 < LIGHTSOUT_SIZE) {
    m |= 1u << ((row + 1) * LIGHTSOUT_SIZE + col);
  }
  if (col > 0) {
    m |= 1u << (row * LIGHTSOUT_SIZE + (col - 1));
  }
  if (col + 1 < LIGHTSOUT_SIZE) {
    m |= 1u << (row * LIGHTSOUT_SIZE + (col + 1));
  }
  return m;
}

void lightsout_board_init(LightsoutBoard *b, uint32_t seed)
{
  memset(b, 0, sizeof(*b));
  b->rng = seed == 0u ? 0xC0FFEEu : seed;
}

void lightsout_board_reset(LightsoutBoard *b, uint32_t seed)
{
  lightsout_board_init(b, seed);
  /* Start solved, then scramble with random presses so the state is always reachable. */
  for (int i = 0; i < 48; i++) {
    uint32_t r = lightsout_rng_next(&b->rng);
    int cell = (int)(r % (uint32_t)LIGHTSOUT_CELL_COUNT);
    int row = cell / LIGHTSOUT_SIZE;
    int col = cell % LIGHTSOUT_SIZE;
    b->on_bits ^= lightsout_toggle_mask(row, col);
  }
  if (b->on_bits == 0u) {
    uint32_t r = lightsout_rng_next(&b->rng);
    int cell = (int)(r % (uint32_t)LIGHTSOUT_CELL_COUNT);
    b->on_bits ^= lightsout_toggle_mask(cell / LIGHTSOUT_SIZE, cell % LIGHTSOUT_SIZE);
  }
  b->moves = 0u;
}

bool lightsout_board_is_on(const LightsoutBoard *b, int row, int col)
{
  if (row < 0 || col < 0 || row >= LIGHTSOUT_SIZE || col >= LIGHTSOUT_SIZE) {
    return false;
  }
  int idx = row * LIGHTSOUT_SIZE + col;
  return (b->on_bits & (1u << idx)) != 0u;
}

bool lightsout_board_is_solved(const LightsoutBoard *b)
{
  return b->on_bits == 0u;
}

void lightsout_board_toggle_at(LightsoutBoard *b, int row, int col)
{
  if (row < 0 || col < 0 || row >= LIGHTSOUT_SIZE || col >= LIGHTSOUT_SIZE) {
    return;
  }
  b->on_bits ^= lightsout_toggle_mask(row, col);
  b->moves++;
}
