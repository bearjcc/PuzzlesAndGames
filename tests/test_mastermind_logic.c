#include "mastermind_logic.h"

#include <stdio.h>
#include <string.h>

static int fail;

#define CHECK(cond)                                         \
  do {                                                      \
    if (!(cond)) {                                         \
      fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
      fail = 1;                                            \
    }                                                      \
  } while (0)

int main(void)
{
  int secret[] = {1, 2, 3, 4};
  int guess[] = {1, 1, 2, 3};
  int ex = 0;
  int pa = 0;
  mm_compute_feedback(secret, guess, 4, 6, &ex, &pa);
  CHECK(ex == 1);
  CHECK(pa == 2);

  int secret2[] = {5, 5, 5, 5};
  int guess2[] = {5, 4, 4, 4};
  mm_compute_feedback(secret2, guess2, 4, 6, &ex, &pa);
  CHECK(ex == 1);
  CHECK(pa == 0);

  MmConfig cfg;
  mm_config_default(&cfg);
  cfg.allow_duplicates = false;
  cfg.ncolours = 6;
  cfg.npegs = 4;
  CHECK(mm_config_validate(&cfg) == NULL);

  MmState st;
  mm_state_reset(&st, &cfg, 42u);
  int seen[MM_MAX_COLOURS];
  memset(seen, 0, sizeof(seen));
  for (int i = 0; i < cfg.npegs; i++) {
    int c = st.secret[i];
    CHECK(c >= 1 && c <= cfg.ncolours);
    CHECK(seen[c - 1] == 0);
    seen[c - 1] = 1;
  }

  st.draft[0] = st.secret[0];
  st.draft[1] = st.secret[1];
  st.draft[2] = st.secret[2];
  st.draft[3] = st.secret[3];
  CHECK(mm_try_submit(&st));
  CHECK(st.outcome == 1);

  return fail ? 1 : 0;
}
