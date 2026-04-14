#include "letterlock_puzzle.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int g_failures = 0;

static void fail(const char *msg)
{
  fprintf(stderr, "FAIL: %s\n", msg);
  g_failures++;
}

static void assert_true(const char *msg, bool v)
{
  if (!v) {
    fail(msg);
  }
}

static void assert_eq_int(const char *msg, int a, int b)
{
  if (a != b) {
    fprintf(stderr, "FAIL: %s (got %d expected %d)\n", msg, a, b);
    g_failures++;
  }
}

static bool letter_in_phrase(const char *phrase, char letter)
{
  for (const char *p = phrase; *p != '\0'; p++) {
    if (*p == letter) {
      return true;
    }
  }
  return false;
}

static void test_win_by_full_alphabet_sweep(void)
{
  LetterlockPuzzle p;
  letterlock_puzzle_reset(&p, 1u);
  const char *phrase = letterlock_puzzle_phrase(&p);
  assert_true("phrase non-empty", phrase[0] != '\0');

  for (char c = 'A'; c <= 'Z'; c++) {
    if (letter_in_phrase(phrase, c)) {
      bool ok = letterlock_puzzle_guess(&p, c);
      assert_true("guess accepted", ok);
    }
  }
  assert_true("won", p.won);
  assert_true("not lost", !p.lost);
}

static void test_lose_after_max_wrong(void)
{
  LetterlockPuzzle p;
  letterlock_puzzle_reset(&p, 1u);
  const char *phrase = letterlock_puzzle_phrase(&p);

  int guesses = 0;
  for (char c = 'A'; c <= 'Z' && !p.lost; c++) {
    if (!letter_in_phrase(phrase, c)) {
      bool ok = letterlock_puzzle_guess(&p, c);
      assert_true("wrong guess accepted", ok);
      guesses++;
    }
  }
  assert_true("lost", p.lost);
  assert_eq_int("wrong count at cap", p.wrong_count, LETTERLOCK_MAX_WRONG);
  assert_true("not won", !p.won);
  assert_eq_int("expected wrong guesses", guesses, LETTERLOCK_MAX_WRONG);
}

static void test_duplicate_ignored(void)
{
  LetterlockPuzzle p;
  letterlock_puzzle_reset(&p, 2u);
  assert_true("first A", letterlock_puzzle_guess(&p, 'A'));
  assert_true("second A ignored", !letterlock_puzzle_guess(&p, 'A'));
}

int main(void)
{
  test_win_by_full_alphabet_sweep();
  test_lose_after_max_wrong();
  test_duplicate_ignored();
  return g_failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
