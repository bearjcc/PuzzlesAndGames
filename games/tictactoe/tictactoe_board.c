#include "tictactoe_board.h"

#include <string.h>

void ttt_board_clear(int8_t *cells)
{
  memset(cells, 0, (size_t)TTT_CELL_COUNT * sizeof(cells[0]));
}

static int ttt_count_marks(const int8_t *cells, TttCell m)
{
  int n = 0;
  for (int i = 0; i < TTT_CELL_COUNT; i++) {
    if (cells[i] == (int8_t)m) {
      n++;
    }
  }
  return n;
}

TttCell ttt_side_to_move(const int8_t *cells)
{
  int nx = ttt_count_marks(cells, TTT_X);
  int no = ttt_count_marks(cells, TTT_O);
  if (nx == no) {
    return TTT_X;
  }
  return TTT_O;
}

static bool ttt_line_win(const int8_t *cells, TttCell m, int a, int b, int c)
{
  return cells[a] == (int8_t)m && cells[b] == (int8_t)m && cells[c] == (int8_t)m;
}

TttOutcome ttt_outcome(const int8_t *cells)
{
  static const int lines[8][3] = {
      {0, 1, 2},
      {3, 4, 5},
      {6, 7, 8},
      {0, 3, 6},
      {1, 4, 7},
      {2, 5, 8},
      {0, 4, 8},
      {2, 4, 6},
  };
  for (int i = 0; i < 8; i++) {
    if (ttt_line_win(cells, TTT_X, lines[i][0], lines[i][1], lines[i][2])) {
      return TTT_X_WINS;
    }
    if (ttt_line_win(cells, TTT_O, lines[i][0], lines[i][1], lines[i][2])) {
      return TTT_O_WINS;
    }
  }
  for (int k = 0; k < TTT_CELL_COUNT; k++) {
    if (cells[k] == TTT_EMPTY) {
      return TTT_IN_PROGRESS;
    }
  }
  return TTT_DRAW;
}

bool ttt_cell_empty(const int8_t *cells, int idx)
{
  if (idx < 0 || idx >= TTT_CELL_COUNT) {
    return false;
  }
  return cells[idx] == TTT_EMPTY;
}

/* #region RNG */
static uint32_t ttt_rng_u32(uint32_t *s)
{
  uint32_t x = *s;
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  *s = x == 0u ? 0xA341316Cu : x;
  return *s;
}
/* #endregion */

/* #region Minimax (O maximizes, X minimizes) */
static int ttt_minimax_score(int8_t *b, TttCell player_to_move, int depth)
{
  TttOutcome o = ttt_outcome(b);
  if (o == TTT_O_WINS) {
    return 10 - depth;
  }
  if (o == TTT_X_WINS) {
    return depth - 10;
  }
  if (o == TTT_DRAW) {
    return 0;
  }

  int best = (player_to_move == TTT_O) ? -100 : 100;
  for (int i = 0; i < TTT_CELL_COUNT; i++) {
    if (b[i] != TTT_EMPTY) {
      continue;
    }
    b[i] = (int8_t)player_to_move;
    TttCell next = (player_to_move == TTT_X) ? TTT_O : TTT_X;
    int sc = ttt_minimax_score(b, next, depth + 1);
    b[i] = TTT_EMPTY;
    if (player_to_move == TTT_O) {
      if (sc > best) {
        best = sc;
      }
    } else {
      if (sc < best) {
        best = sc;
      }
    }
  }
  return best;
}

static int ttt_best_o_move(const int8_t *cells)
{
  int8_t b[TTT_CELL_COUNT];
  memcpy(b, cells, sizeof(b));
  int best_i = -1;
  int best_score = -100;
  for (int i = 0; i < TTT_CELL_COUNT; i++) {
    if (b[i] != TTT_EMPTY) {
      continue;
    }
    b[i] = TTT_O;
    int sc = ttt_minimax_score(b, TTT_X, 0);
    b[i] = TTT_EMPTY;
    if (sc > best_score) {
      best_score = sc;
      best_i = i;
    }
  }
  return best_i;
}
/* #endregion */

static bool ttt_collect_empty(const int8_t *cells, int *out, int max_out, int *out_count)
{
  int n = 0;
  for (int i = 0; i < TTT_CELL_COUNT; i++) {
    if (cells[i] == TTT_EMPTY) {
      if (n >= max_out) {
        return false;
      }
      out[n++] = i;
    }
  }
  *out_count = n;
  return true;
}

static int ttt_random_empty(const int8_t *cells, uint32_t *rng)
{
  int empties[TTT_CELL_COUNT];
  int ec = 0;
  if (!ttt_collect_empty(cells, empties, TTT_CELL_COUNT, &ec) || ec <= 0) {
    return -1;
  }
  uint32_t u = ttt_rng_u32(rng);
  return empties[(int)(u % (uint32_t)ec)];
}

bool ttt_ai_choose_move(const int8_t *cells, TttAiDifficulty diff, uint32_t *rng_state, int *out_idx)
{
  if (rng_state == NULL || out_idx == NULL) {
    return false;
  }
  if (ttt_outcome(cells) != TTT_IN_PROGRESS) {
    return false;
  }
  if (ttt_side_to_move(cells) != TTT_O) {
    return false;
  }

  int empties[TTT_CELL_COUNT];
  int ec = 0;
  if (!ttt_collect_empty(cells, empties, TTT_CELL_COUNT, &ec) || ec <= 0) {
    return false;
  }

  if (diff == TTT_AI_EASY) {
    *out_idx = ttt_random_empty(cells, rng_state);
    return *out_idx >= 0;
  }

  if (diff == TTT_AI_IMPOSSIBLE) {
    *out_idx = ttt_best_o_move(cells);
    return *out_idx >= 0;
  }

  /* Medium: sometimes random, otherwise optimal. */
  uint32_t u = ttt_rng_u32(rng_state);
  if ((u % 100u) < 40u) {
    *out_idx = ttt_random_empty(cells, rng_state);
    if (*out_idx < 0) {
      *out_idx = ttt_best_o_move(cells);
    }
    return *out_idx >= 0;
  }
  *out_idx = ttt_best_o_move(cells);
  return *out_idx >= 0;
}
