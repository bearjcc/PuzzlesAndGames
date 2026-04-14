#ifndef SLITHERLINK_BOARD_H
#define SLITHERLINK_BOARD_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum SlShape {
  SL_SHAPE_RECT = 0,
  SL_SHAPE_BLOB = 1,
} SlShape;

typedef enum SlDifficulty {
  SL_DIFF_EASY = 0,
  SL_DIFF_NORMAL = 1,
  SL_DIFF_HARD = 2,
} SlDifficulty;

typedef struct SlitherlinkBoard {
  int w;
  int h;
  SlShape shape;
  SlDifficulty difficulty;
  uint32_t rng;

  /* Horizontal edges: h[ri][ci], ri in [0,h], ci in [0,w-1] */
  bool *h_user;
  bool *h_sol;
  /* Vertical edges: v[ri][ci], ri in [0,h-1], ci in [0,w] */
  bool *v_user;
  bool *v_sol;

  /* Per cell: -1 = unknown, 0..3 = clue (only when shape==RECT or cell active for BLOB) */
  int8_t *clue;
  /* For SL_SHAPE_BLOB: cell is part of polyomino (interior+boundary); exterior cells have no clues. */
  bool *cell_active;

  bool solved;
} SlitherlinkBoard;

void sl_board_init_defaults(SlitherlinkBoard *b);
void sl_board_free(SlitherlinkBoard *b);
bool sl_board_alloc(SlitherlinkBoard *b, int w, int h);

/* Fills solution, clues, cell_active from rng and settings. Returns false on failure (retry with new seed). */
bool sl_board_generate(SlitherlinkBoard *b);

/* Toggle user edge at pixel coords; edge_kind 0=horizontal 1=vertical, indices from sl_board_hit_test */
void sl_board_toggle_h(SlitherlinkBoard *b, int ri, int ci);
void sl_board_toggle_v(SlitherlinkBoard *b, int ri, int ci);

void sl_board_clear_user(SlitherlinkBoard *b);
void sl_board_check_solved(SlitherlinkBoard *b);

/* Count solutions with current partial user assignment (used after generation). */
int sl_count_solutions(const SlitherlinkBoard *b, int limit);

#endif
