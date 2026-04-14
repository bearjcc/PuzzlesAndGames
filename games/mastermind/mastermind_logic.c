#include "mastermind_logic.h"

#include <string.h>

void mm_config_default(MmConfig *cfg)
{
  cfg->ncolours = 6;
  cfg->npegs = 4;
  cfg->nguesses = 10;
  cfg->allow_blank = false;
  cfg->allow_duplicates = true;
}

const char *mm_config_validate(const MmConfig *cfg)
{
  if (cfg->ncolours < 2 || cfg->npegs < 2) {
    return "need at least 2 colours and 2 pegs";
  }
  if (cfg->ncolours > MM_MAX_COLOURS) {
    return "too many colours";
  }
  if (cfg->npegs > MM_MAX_PEGS) {
    return "too many pegs per row";
  }
  if (cfg->nguesses < 1 || cfg->nguesses > MM_MAX_GUESSES) {
    return "guess count out of range";
  }
  if (!cfg->allow_duplicates && cfg->ncolours < cfg->npegs) {
    return "unique colours need at least as many colours as pegs";
  }
  if (cfg->allow_blank) {
    return "blank pegs are not implemented yet";
  }
  return NULL;
}

uint32_t mm_rng_u32(uint32_t *rng)
{
  uint32_t x = *rng;
  if (x == 0u) {
    x = 0xA341316Cu;
  }
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  *rng = x == 0u ? 1u : x;
  return *rng;
}

static void mm_gen_secret(MmState *s)
{
  int used[MM_MAX_COLOURS];
  memset(used, 0, sizeof(used));
  for (int i = 0; i < s->cfg.npegs; i++) {
    int placed = 0;
    for (int guard = 0; guard < 65536; guard++) {
      int c = (int)(mm_rng_u32(&s->rng) % (uint32_t)s->cfg.ncolours) + 1;
      if (!s->cfg.allow_duplicates && used[c - 1]) {
        continue;
      }
      s->secret[i] = c;
      if (!s->cfg.allow_duplicates) {
        used[c - 1] = 1;
      }
      placed = 1;
      break;
    }
    if (!placed) {
      /* Should be unreachable when cfg is validated. */
      s->secret[i] = 1;
    }
  }
}

void mm_state_reset(MmState *s, const MmConfig *cfg, uint32_t seed)
{
  s->cfg = *cfg;
  s->rng = seed == 0u ? 0xC0FFEEu : seed;
  memset(s->secret, 0, sizeof(s->secret));
  memset(s->draft, 0, sizeof(s->draft));
  memset(s->history, 0, sizeof(s->history));
  s->n_committed = 0;
  s->cursor_slot = 0;
  s->selected_colour = 1;
  s->outcome = 0;
  mm_gen_secret(s);
}

void mm_compute_feedback(
    const int *secret,
    const int *guess,
    int npegs,
    int ncolours,
    int *out_exact,
    int *out_partial)
{
  int nc_place = 0;
  int nc_colour = 0;
  int i;
  int j;

  for (i = 0; i < npegs; i++) {
    if (guess[i] == secret[i]) {
      nc_place++;
    }
  }

  /*
   * Scoring matches Simon Tatham's Guess (`third_party/stp-guess/guess.c`, mark_pegs):
   * sum over colours of min(count in guess, count in secret) minus exact placements.
   */
  for (i = 1; i <= ncolours; i++) {
    int n_guess = 0;
    int n_solution = 0;
    for (j = 0; j < npegs; j++) {
      if (guess[j] == i) {
        n_guess++;
      }
      if (secret[j] == i) {
        n_solution++;
      }
    }
    if (n_guess < n_solution) {
      nc_colour += n_guess;
    } else {
      nc_colour += n_solution;
    }
  }
  nc_colour -= nc_place;

  *out_exact = nc_place;
  *out_partial = nc_colour;
}

bool mm_draft_complete(const MmState *s)
{
  int used[MM_MAX_COLOURS];
  memset(used, 0, sizeof(used));
  for (int i = 0; i < s->cfg.npegs; i++) {
    int c = s->draft[i];
    if (c < 1 || c > s->cfg.ncolours) {
      return false;
    }
    if (!s->cfg.allow_duplicates) {
      if (used[c - 1]) {
        return false;
      }
      used[c - 1] = 1;
    }
  }
  return true;
}

bool mm_try_submit(MmState *s)
{
  if (s->outcome != 0) {
    return false;
  }
  if (s->n_committed >= s->cfg.nguesses) {
    return false;
  }
  if (!mm_draft_complete(s)) {
    return false;
  }

  int exact = 0;
  int partial = 0;
  mm_compute_feedback(s->secret, s->draft, s->cfg.npegs, s->cfg.ncolours, &exact, &partial);

  MmGuessRow *row = &s->history[s->n_committed];
  memcpy(row->pegs, s->draft, sizeof(int) * (size_t)s->cfg.npegs);
  row->exact = exact;
  row->partial = partial;
  s->n_committed++;

  if (exact == s->cfg.npegs) {
    s->outcome = 1;
    return true;
  }
  if (s->n_committed >= s->cfg.nguesses) {
    s->outcome = -1;
    return true;
  }

  memset(s->draft, 0, sizeof(s->draft));
  s->cursor_slot = 0;
  return true;
}
