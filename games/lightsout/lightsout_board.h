#ifndef LIGHTSOUT_BOARD_H
#define LIGHTSOUT_BOARD_H

#include <stdbool.h>
#include <stdint.h>

#define LIGHTSOUT_SIZE 5
#define LIGHTSOUT_CELL_COUNT (LIGHTSOUT_SIZE * LIGHTSOUT_SIZE)

typedef struct LightsoutBoard {
  uint32_t on_bits;
  uint32_t rng;
  uint32_t moves;
} LightsoutBoard;

void lightsout_board_init(LightsoutBoard *b, uint32_t seed);
void lightsout_board_reset(LightsoutBoard *b, uint32_t seed);

uint32_t lightsout_rng_next(uint32_t *rng);

bool lightsout_board_is_on(const LightsoutBoard *b, int row, int col);
bool lightsout_board_is_solved(const LightsoutBoard *b);

void lightsout_board_toggle_at(LightsoutBoard *b, int row, int col);

#endif
