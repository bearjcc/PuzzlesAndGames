#ifndef LETTERLOCK_PUZZLE_H
#define LETTERLOCK_PUZZLE_H

#include <stdbool.h>
#include <stdint.h>

#define LETTERLOCK_MAX_PHRASE 96
#define LETTERLOCK_MAX_WRONG 8

typedef struct LetterlockPuzzle {
  char phrase[LETTERLOCK_MAX_PHRASE];
  bool guessed[26];
  int wrong_count;
  int max_wrong;
  bool won;
  bool lost;
} LetterlockPuzzle;

void letterlock_puzzle_reset(LetterlockPuzzle *p, uint32_t seed);

const char *letterlock_puzzle_phrase(const LetterlockPuzzle *p);

/* Letter A-Z (any case). Returns false if ignored (invalid, duplicate, or terminal state). */
bool letterlock_puzzle_guess(LetterlockPuzzle *p, char letter);

#endif
