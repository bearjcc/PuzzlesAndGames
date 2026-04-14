#include "tictactoe_board.h"

#include <assert.h>
#include <stdint.h>

static void test_empty_and_side(void)
{
  int8_t b[TTT_CELL_COUNT];
  ttt_board_clear(b);
  assert(ttt_outcome(b) == TTT_IN_PROGRESS);
  assert(ttt_side_to_move(b) == TTT_X);
  b[0] = TTT_X;
  assert(ttt_side_to_move(b) == TTT_O);
}

static void test_x_wins_row(void)
{
  int8_t b[TTT_CELL_COUNT] = {TTT_X, TTT_X, TTT_X, TTT_O, TTT_O, TTT_EMPTY, TTT_EMPTY, TTT_EMPTY, TTT_EMPTY};
  assert(ttt_outcome(b) == TTT_X_WINS);
}

static void test_draw(void)
{
  int8_t b[TTT_CELL_COUNT] = {
      TTT_X, TTT_X, TTT_O,
      TTT_O, TTT_O, TTT_X,
      TTT_X, TTT_O, TTT_X};
  assert(ttt_outcome(b) == TTT_DRAW);
}

static void test_ai_only_on_o_turn(void)
{
  int8_t b[TTT_CELL_COUNT];
  ttt_board_clear(b);
  uint32_t rng = 1u;
  int mv = -1;
  assert(!ttt_ai_choose_move(b, TTT_AI_IMPOSSIBLE, &rng, &mv));
  b[0] = TTT_X;
  assert(ttt_ai_choose_move(b, TTT_AI_IMPOSSIBLE, &rng, &mv));
  assert(mv >= 0 && mv < TTT_CELL_COUNT && b[mv] == TTT_EMPTY);
}

static void test_impossible_blocks_line(void)
{
  /* X threatens top row win at 2; O must block. */
  int8_t b[TTT_CELL_COUNT];
  ttt_board_clear(b);
  b[0] = TTT_X;
  b[4] = TTT_O;
  b[1] = TTT_X;
  uint32_t rng = 42u;
  int mv = -1;
  assert(ttt_ai_choose_move(b, TTT_AI_IMPOSSIBLE, &rng, &mv));
  assert(mv == 2);
}

static void test_impossible_takes_win(void)
{
  int8_t b[TTT_CELL_COUNT] = {
      TTT_O, TTT_X, TTT_X,
      TTT_EMPTY, TTT_O, TTT_X,
      TTT_EMPTY, TTT_EMPTY, TTT_EMPTY};
  uint32_t rng = 1u;
  int mv = -1;
  assert(ttt_ai_choose_move(b, TTT_AI_IMPOSSIBLE, &rng, &mv));
  /* O at 0 and 4 completes diagonal 0-4-8. */
  assert(mv == 8);
}

int main(void)
{
  test_empty_and_side();
  test_x_wins_row();
  test_draw();
  test_ai_only_on_o_turn();
  test_impossible_blocks_line();
  test_impossible_takes_win();
  return 0;
}
