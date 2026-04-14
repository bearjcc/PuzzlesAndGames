#ifndef SLITHERLINK_GAME_H
#define SLITHERLINK_GAME_H

#include "pg/game.h"

#include "slitherlink_board.h"

const PgGameVtable *slitherlink_game_vt(void);

/* Optional: call once after create() returns state, before first frame. */
void slitherlink_game_configure(void *state, int w, int h, SlShape shape, SlDifficulty diff, uint32_t seed);

#endif
