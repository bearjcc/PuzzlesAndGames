#include "g2048_board.h"

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

static void test_merge_left(void)
{
  G2048Board b;
  memset(&b, 0, sizeof(b));
  b.next_id = 10;
  b.rng = 1u;
  b.v[0][0] = 2;
  b.v[0][1] = 2;
  b.id[0][0] = 1;
  b.id[0][1] = 2;

  G2048Board out;
  G2048AnimSprite sp[G2048_MAX_ANIM_SPRITES];
  int sc = 0;
  int sd = 0;
  bool wd = false;
  bool ok = g2048_board_prepare_move(&b, G2048_DIR_LEFT, &out, sp, &sc, &sd, &wd);
  assert_true("move should succeed", ok);
  assert_eq_int("merged value", out.v[0][0], 4);
  assert_eq_int("row cleared after merge", out.v[0][1], 0);
  assert_eq_int("score includes merge", sd, 4);
  assert_true("two sprites for merge", sc >= 2);
}

static void test_no_move(void)
{
  G2048Board b;
  memset(&b, 0, sizeof(b));
  b.next_id = 1;
  b.rng = 1u;
  b.v[0][0] = 2;
  b.v[0][1] = 4;
  b.id[0][0] = 1;
  b.id[0][1] = 2;

  G2048Board out;
  G2048AnimSprite sp[G2048_MAX_ANIM_SPRITES];
  int sc = 0;
  int sd = 0;
  bool wd = false;
  bool ok = g2048_board_prepare_move(&b, G2048_DIR_LEFT, &out, sp, &sc, &sd, &wd);
  assert_true("blocked move should fail", !ok);
}

static void test_merge_right(void)
{
  G2048Board b;
  memset(&b, 0, sizeof(b));
  b.next_id = 5;
  b.rng = 1u;
  b.v[1][2] = 2;
  b.v[1][3] = 2;
  b.id[1][2] = 1;
  b.id[1][3] = 2;

  G2048Board out;
  G2048AnimSprite sp[G2048_MAX_ANIM_SPRITES];
  int sc = 0;
  int sd = 0;
  bool wd = false;
  bool ok = g2048_board_prepare_move(&b, G2048_DIR_RIGHT, &out, sp, &sc, &sd, &wd);
  assert_true("right merge should succeed", ok);
  assert_eq_int("merged to right wall", out.v[1][3], 4);
  assert_eq_int("cleared neighbor", out.v[1][2], 0);
}

static void test_merge_up(void)
{
  G2048Board b;
  memset(&b, 0, sizeof(b));
  b.next_id = 5;
  b.rng = 1u;
  b.v[0][0] = 2;
  b.v[1][0] = 2;
  b.id[0][0] = 1;
  b.id[1][0] = 2;

  G2048Board out;
  G2048AnimSprite sp[G2048_MAX_ANIM_SPRITES];
  int sc = 0;
  int sd = 0;
  bool wd = false;
  bool ok = g2048_board_prepare_move(&b, G2048_DIR_UP, &out, sp, &sc, &sd, &wd);
  assert_true("up merge should succeed", ok);
  assert_eq_int("merged to top", out.v[0][0], 4);
  assert_eq_int("cleared below", out.v[1][0], 0);
}

int main(void)
{
  test_merge_left();
  test_no_move();
  test_merge_right();
  test_merge_up();
  return g_failures == 0 ? 0 : 1;
}
