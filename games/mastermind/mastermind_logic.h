#ifndef MASTERMIND_LOGIC_H
#define MASTERMIND_LOGIC_H

#include <stdbool.h>
#include <stdint.h>

/* Tunable bounds (UI and stack allocation). STP Guess allows more; we cap for the SDL shell. */
#define MM_MAX_COLOURS 10
#define MM_MAX_PEGS 8
#define MM_MAX_GUESSES 16

typedef struct MmConfig {
  int ncolours;
  int npegs;
  int nguesses;
  /* Peg value 0 is blank (STP Guess). Secrets still use 1..ncolours only. */
  bool allow_blank;
  bool allow_duplicates;
} MmConfig;

typedef struct MmGuessRow {
  int pegs[MM_MAX_PEGS];
  int exact;
  int partial;
} MmGuessRow;

typedef struct MmState {
  MmConfig cfg;
  int secret[MM_MAX_PEGS];
  int draft[MM_MAX_PEGS];
  MmGuessRow history[MM_MAX_GUESSES];
  int n_committed;
  int cursor_slot;
  int selected_colour;
  /* 0 in play, 1 won, -1 lost */
  int outcome;
  uint32_t rng;
} MmState;

void mm_config_default(MmConfig *cfg);

/* Returns NULL if valid, otherwise a short static reason string. */
const char *mm_config_validate(const MmConfig *cfg);

uint32_t mm_rng_u32(uint32_t *rng);

void mm_state_reset(MmState *s, const MmConfig *cfg, uint32_t seed);

bool mm_draft_complete(const MmState *s);

/* Fills exact/partial using the same counting rule as STP Guess (`guess.c` mark_pegs). */
void mm_compute_feedback(
    const int *secret,
    const int *guess,
    int npegs,
    int ncolours,
    int *out_exact,
    int *out_partial);

/* Commits the current draft as the next guess. Returns false if draft invalid or game over. */
bool mm_try_submit(MmState *s);

#endif
