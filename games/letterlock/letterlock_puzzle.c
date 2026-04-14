#include "letterlock_puzzle.h"

#include <string.h>

/* #region Phrase bank (short ASCII idioms; no trivia-show wording) */
static const char *const kPhrases[] = {
    "A BLESSING IN DISGUISE",
    "BREAK THE ICE",
    "PIECE OF CAKE",
    "UNDER THE WEATHER",
    "ONCE IN A BLUE MOON",
    "THE BALL IS IN YOUR COURT",
    "BETTER LATE THAN NEVER",
    "CURIOSITY KILLED THE CAT",
    "EVERY CLOUD HAS A SILVER LINING",
    "HIT THE NAIL ON THE HEAD",
    "IT TAKES TWO TO TANGO",
    "LET SLEEPING DOGS LIE",
    "NO PAIN NO GAIN",
    "THE EARLY BIRD CATCHES THE WORM",
    "WHEN IT RAINS IT POURS",
    "YOU CAN NOT HAVE YOUR CAKE AND EAT IT TOO",
    "ACTIONS SPEAK LOUDER THAN WORDS",
    "BURNING THE MIDNIGHT OIL",
    "COST AN ARM AND A LEG",
    "DO NOT COUNT YOUR CHICKENS BEFORE THEY HATCH",
};
/* #endregion */

static bool is_az_upper(char c)
{
  return c >= 'A' && c <= 'Z';
}

static void phrase_normalize(char *dst, size_t dst_cap, const char *src)
{
  size_t o = 0;
  for (const char *p = src; *p != '\0' && o + 1 < dst_cap; p++) {
    char c = *p;
    if (c >= 'a' && c <= 'z') {
      c = (char)(c - 'a' + 'A');
    }
    if (c == '\t') {
      c = ' ';
    }
    dst[o++] = c;
  }
  dst[o] = '\0';
}

static bool phrase_valid(const char *s)
{
  if (s == NULL || s[0] == '\0') {
    return false;
  }
  size_t n = strlen(s);
  if (n >= LETTERLOCK_MAX_PHRASE) {
    return false;
  }
  for (size_t i = 0; i < n; i++) {
    char c = s[i];
    if (is_az_upper(c) || c == ' ' || c == '\'' || c == '-' || c == ',') {
      continue;
    }
    return false;
  }
  return true;
}

static uint32_t rng_next(uint32_t *s)
{
  /* LCG (Numerical Recipes style); non-crypto PRNG for phrase pick only */
  *s = *s * 1664525u + 1013904223u;
  return *s;
}

const char *letterlock_puzzle_phrase(const LetterlockPuzzle *p)
{
  return p->phrase;
}

void letterlock_puzzle_reset(LetterlockPuzzle *p, uint32_t seed)
{
  memset(p, 0, sizeof(*p));
  p->max_wrong = LETTERLOCK_MAX_WRONG;
  uint32_t s = seed == 0u ? 1u : seed;
  uint32_t r = rng_next(&s);
  size_t count = sizeof(kPhrases) / sizeof(kPhrases[0]);
  size_t idx = (size_t)(r % (uint32_t)count);
  const char *pick = kPhrases[idx];
  if (!phrase_valid(pick)) {
    pick = kPhrases[0];
  }
  phrase_normalize(p->phrase, sizeof(p->phrase), pick);
}

static bool puzzle_terminal(const LetterlockPuzzle *p)
{
  return p->won || p->lost;
}

static bool all_letters_found(const LetterlockPuzzle *p)
{
  for (const char *q = p->phrase; *q != '\0'; q++) {
    char c = *q;
    if (is_az_upper(c)) {
      if (!p->guessed[(size_t)(c - 'A')]) {
        return false;
      }
    }
  }
  return true;
}

bool letterlock_puzzle_guess(LetterlockPuzzle *p, char letter)
{
  if (puzzle_terminal(p)) {
    return false;
  }
  if (letter >= 'a' && letter <= 'z') {
    letter = (char)(letter - 'a' + 'A');
  }
  if (!is_az_upper(letter)) {
    return false;
  }
  size_t li = (size_t)(letter - 'A');
  if (p->guessed[li]) {
    return false;
  }

  bool in_phrase = false;
  for (const char *q = p->phrase; *q != '\0'; q++) {
    if (*q == letter) {
      in_phrase = true;
      break;
    }
  }

  p->guessed[li] = true;
  if (in_phrase) {
    if (all_letters_found(p)) {
      p->won = true;
    }
  } else {
    p->wrong_count++;
    if (p->wrong_count >= p->max_wrong) {
      p->lost = true;
    }
  }
  return true;
}
