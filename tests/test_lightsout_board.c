#include "lightsout_board.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void test_rng_nonzero(void)
{
  uint32_t r = 1u;
  for (int i = 0; i < 8; i++) {
    uint32_t n = lightsout_rng_next(&r);
    assert(n != 0u);
  }
}

static void test_reset_moves_zero(void)
{
  LightsoutBoard b;
  lightsout_board_reset(&b, 0x12345678u);
  assert(b.moves == 0u);
}

static void test_toggle_center_neighbors(void)
{
  LightsoutBoard b;
  memset(&b, 0, sizeof(b));
  b.rng = 1u;
  lightsout_board_toggle_at(&b, 2, 2);
  assert(b.moves == 1u);
  assert(lightsout_board_is_on(&b, 2, 2));
  assert(lightsout_board_is_on(&b, 1, 2));
  assert(lightsout_board_is_on(&b, 3, 2));
  assert(lightsout_board_is_on(&b, 2, 1));
  assert(lightsout_board_is_on(&b, 2, 3));
  assert(!lightsout_board_is_on(&b, 0, 0));
  lightsout_board_toggle_at(&b, 2, 2);
  assert(b.moves == 2u);
  assert(lightsout_board_is_solved(&b));
}

static void test_corner_toggle(void)
{
  LightsoutBoard b;
  memset(&b, 0, sizeof(b));
  b.rng = 1u;
  lightsout_board_toggle_at(&b, 0, 0);
  assert(lightsout_board_is_on(&b, 0, 0));
  assert(lightsout_board_is_on(&b, 0, 1));
  assert(lightsout_board_is_on(&b, 1, 0));
  assert(!lightsout_board_is_on(&b, 1, 1));
}

int main(void)
{
  test_rng_nonzero();
  test_reset_moves_zero();
  test_toggle_center_neighbors();
  test_corner_toggle();
  puts("lightsout_board ok");
  return 0;
}
