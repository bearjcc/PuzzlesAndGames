#ifndef G2048_BOARD_H
#define G2048_BOARD_H

#include <stdbool.h>
#include <stdint.h>

#define G2048_SIZE 4
#define G2048_MAX_ANIM_SPRITES 24

typedef enum G2048Dir {
  G2048_DIR_LEFT = 0,
  G2048_DIR_RIGHT = 1,
  G2048_DIR_UP = 2,
  G2048_DIR_DOWN = 3,
} G2048Dir;

typedef struct G2048AnimSprite {
  int value;
  float fr;
  float fc;
  float tr;
  float tc;
  bool merge_pulse;
} G2048AnimSprite;

typedef struct G2048Board {
  int v[G2048_SIZE][G2048_SIZE];
  int id[G2048_SIZE][G2048_SIZE];
  int next_id;
  int score;
  bool won;
  uint32_t rng;
} G2048Board;

void g2048_board_init(G2048Board *b, uint32_t seed);
void g2048_board_reset(G2048Board *b, uint32_t seed);

bool g2048_board_grids_equal(const int a[G2048_SIZE][G2048_SIZE], const int b[G2048_SIZE][G2048_SIZE]);

bool g2048_board_prepare_move(
    const G2048Board *in,
    G2048Dir dir,
    G2048Board *out,
    G2048AnimSprite *sprites,
    int *sprite_count,
    int *score_delta,
    bool *won_delta);

bool g2048_board_has_empty(const G2048Board *b);
bool g2048_board_has_move(const G2048Board *b);
void g2048_board_spawn_random(G2048Board *b);

uint32_t g2048_rng_next(uint32_t *rng);

#endif
