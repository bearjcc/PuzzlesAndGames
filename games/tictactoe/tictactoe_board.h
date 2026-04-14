#ifndef TICTACTOE_BOARD_H
#define TICTACTOE_BOARD_H

#include <stdbool.h>
#include <stdint.h>

#define TTT_CELL_COUNT 9

typedef int8_t TttCell;

enum {
  TTT_EMPTY = 0,
  TTT_X = 1,
  TTT_O = 2,
};

typedef enum {
  TTT_IN_PROGRESS = 0,
  TTT_X_WINS = 1,
  TTT_O_WINS = 2,
  TTT_DRAW = 3,
} TttOutcome;

typedef enum {
  TTT_AI_EASY = 0,
  TTT_AI_MEDIUM = 1,
  TTT_AI_IMPOSSIBLE = 2,
} TttAiDifficulty;

void ttt_board_clear(int8_t *cells);

TttOutcome ttt_outcome(const int8_t *cells);

bool ttt_cell_empty(const int8_t *cells, int idx);

/* X first: equal X and O counts means X to play; one more X than O means O to play. */
TttCell ttt_side_to_move(const int8_t *cells);

bool ttt_ai_choose_move(const int8_t *cells, TttAiDifficulty diff, uint32_t *rng_state, int *out_idx);

#endif
