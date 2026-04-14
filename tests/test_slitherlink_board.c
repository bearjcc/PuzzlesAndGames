#include "slitherlink_board.h"

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

static void test_rect_unique(void)
{
  SlitherlinkBoard b;
  memset(&b, 0, sizeof(b));
  b.w = 5;
  b.h = 5;
  b.shape = SL_SHAPE_RECT;
  b.difficulty = SL_DIFF_EASY;
  b.rng = 4242u;
  assert_true("alloc", sl_board_alloc(&b, b.w, b.h));
  assert_true("generate", sl_board_generate(&b));
  assert_eq_int("unique solutions", sl_count_solutions(&b, 3), 1);
  sl_board_free(&b);
}

static void test_blob_unique(void)
{
  SlitherlinkBoard b;
  memset(&b, 0, sizeof(b));
  b.w = 7;
  b.h = 7;
  b.shape = SL_SHAPE_BLOB;
  b.difficulty = SL_DIFF_NORMAL;
  b.rng = 9001u;
  assert_true("alloc", sl_board_alloc(&b, b.w, b.h));
  assert_true("generate", sl_board_generate(&b));
  assert_eq_int("unique solutions", sl_count_solutions(&b, 3), 1);
  sl_board_free(&b);
}

int main(void)
{
  test_rect_unique();
  test_blob_unique();
  return g_failures == 0 ? 0 : 1;
}
