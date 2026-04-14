#include "g2048_board.h"

#include <string.h>

uint32_t g2048_rng_next(uint32_t *rng)
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

static void board_clear(G2048Board *b)
{
  memset(b->v, 0, sizeof(b->v));
  memset(b->id, 0, sizeof(b->id));
  b->next_id = 1;
  b->score = 0;
  b->won = false;
}

void g2048_board_init(G2048Board *b, uint32_t seed)
{
  b->rng = seed == 0u ? 0xC0FFEEu : seed;
  board_clear(b);
}

void g2048_board_reset(G2048Board *b, uint32_t seed)
{
  b->rng = seed == 0u ? 0xC0FFEEu : seed;
  board_clear(b);
  g2048_board_spawn_random(b);
  g2048_board_spawn_random(b);
}

bool g2048_board_grids_equal(const int a[G2048_SIZE][G2048_SIZE], const int b[G2048_SIZE][G2048_SIZE])
{
  return memcmp(a, b, sizeof(int) * G2048_SIZE * G2048_SIZE) == 0;
}

bool g2048_board_has_empty(const G2048Board *b)
{
  for (int r = 0; r < G2048_SIZE; r++) {
    for (int c = 0; c < G2048_SIZE; c++) {
      if (b->v[r][c] == 0) {
        return true;
      }
    }
  }
  return false;
}

bool g2048_board_has_move(const G2048Board *b)
{
  if (g2048_board_has_empty(b)) {
    return true;
  }
  for (int r = 0; r < G2048_SIZE; r++) {
    for (int c = 0; c < G2048_SIZE; c++) {
      int v = b->v[r][c];
      if (v == 0) {
        continue;
      }
      if (c + 1 < G2048_SIZE && b->v[r][c + 1] == v) {
        return true;
      }
      if (r + 1 < G2048_SIZE && b->v[r + 1][c] == v) {
        return true;
      }
    }
  }
  return false;
}

void g2048_board_spawn_random(G2048Board *b)
{
  int empties[G2048_SIZE * G2048_SIZE][2];
  int n = 0;
  for (int r = 0; r < G2048_SIZE; r++) {
    for (int c = 0; c < G2048_SIZE; c++) {
      if (b->v[r][c] == 0) {
        empties[n][0] = r;
        empties[n][1] = c;
        n++;
      }
    }
  }
  if (n == 0) {
    return;
  }
  uint32_t pick = g2048_rng_next(&b->rng) % (uint32_t)n;
  int r = empties[pick][0];
  int c = empties[pick][1];
  uint32_t r2 = g2048_rng_next(&b->rng);
  int value = (r2 % 10u) == 0u ? 4 : 2;
  b->v[r][c] = value;
  b->id[r][c] = b->next_id++;
}

typedef struct {
  int v;
  int id;
  int r;
  int c;
} G2048Cell;

static int g2048_read_line(
    G2048Dir dir, int line, const int v[G2048_SIZE][G2048_SIZE], const int id[G2048_SIZE][G2048_SIZE], G2048Cell *out)
{
  int n = 0;
  switch (dir) {
  case G2048_DIR_LEFT:
    for (int c = 0; c < G2048_SIZE; c++) {
      if (v[line][c] != 0) {
        out[n++] = (G2048Cell){v[line][c], id[line][c], line, c};
      }
    }
    break;
  case G2048_DIR_RIGHT:
    for (int c = G2048_SIZE - 1; c >= 0; c--) {
      if (v[line][c] != 0) {
        out[n++] = (G2048Cell){v[line][c], id[line][c], line, c};
      }
    }
    break;
  case G2048_DIR_UP:
    for (int r = 0; r < G2048_SIZE; r++) {
      if (v[r][line] != 0) {
        out[n++] = (G2048Cell){v[r][line], id[r][line], r, line};
      }
    }
    break;
  case G2048_DIR_DOWN:
    for (int r = G2048_SIZE - 1; r >= 0; r--) {
      if (v[r][line] != 0) {
        out[n++] = (G2048Cell){v[r][line], id[r][line], r, line};
      }
    }
    break;
  }
  return n;
}

static void g2048_write_slot(G2048Dir dir, int line, int k, int vv, int iid, int outv[G2048_SIZE][G2048_SIZE], int outid[G2048_SIZE][G2048_SIZE])
{
  switch (dir) {
  case G2048_DIR_LEFT:
    outv[line][k] = vv;
    outid[line][k] = iid;
    break;
  case G2048_DIR_RIGHT:
    outv[line][G2048_SIZE - 1 - k] = vv;
    outid[line][G2048_SIZE - 1 - k] = iid;
    break;
  case G2048_DIR_UP:
    outv[k][line] = vv;
    outid[k][line] = iid;
    break;
  case G2048_DIR_DOWN:
    outv[G2048_SIZE - 1 - k][line] = vv;
    outid[G2048_SIZE - 1 - k][line] = iid;
    break;
  }
}

static void g2048_target_cell(G2048Dir dir, int line, int k, float *tr, float *tc)
{
  switch (dir) {
  case G2048_DIR_LEFT:
    *tr = (float)line;
    *tc = (float)k;
    break;
  case G2048_DIR_RIGHT:
    *tr = (float)line;
    *tc = (float)(G2048_SIZE - 1 - k);
    break;
  case G2048_DIR_UP:
    *tr = (float)k;
    *tc = (float)line;
    break;
  case G2048_DIR_DOWN:
    *tr = (float)(G2048_SIZE - 1 - k);
    *tc = (float)line;
    break;
  }
}

static bool g2048_push_sprite(
    G2048AnimSprite *sprites, int *count, int cap, int value, float fr, float fc, float tr, float tc, bool merge_pulse)
{
  if (*count >= cap) {
    return false;
  }
  sprites[*count] = (G2048AnimSprite){value, fr, fc, tr, tc, merge_pulse};
  (*count)++;
  return true;
}

static void g2048_merge_line(
    G2048Dir dir,
    int line,
    const G2048Cell *in,
    int n,
    int outv[G2048_SIZE][G2048_SIZE],
    int outid[G2048_SIZE][G2048_SIZE],
    int *next_id,
    G2048AnimSprite *sprites,
    int *sprite_count,
    int *score_delta,
    bool *won_delta)
{
  int write_k = 0;
  for (int i = 0; i < n;) {
    float tr = 0.0f;
    float tc = 0.0f;
    g2048_target_cell(dir, line, write_k, &tr, &tc);
    if (i + 1 < n && in[i].v == in[i + 1].v) {
      int merged = in[i].v * 2;
      int new_id = (*next_id)++;
      (void)g2048_push_sprite(
          sprites,
          sprite_count,
          G2048_MAX_ANIM_SPRITES,
          in[i].v,
          (float)in[i].r,
          (float)in[i].c,
          tr,
          tc,
          false);
      (void)g2048_push_sprite(
          sprites,
          sprite_count,
          G2048_MAX_ANIM_SPRITES,
          in[i + 1].v,
          (float)in[i + 1].r,
          (float)in[i + 1].c,
          tr,
          tc,
          true);
      g2048_write_slot(dir, line, write_k, merged, new_id, outv, outid);
      *score_delta += merged;
      if (merged >= 2048) {
        *won_delta = true;
      }
      i += 2;
    } else {
      (void)g2048_push_sprite(
          sprites,
          sprite_count,
          G2048_MAX_ANIM_SPRITES,
          in[i].v,
          (float)in[i].r,
          (float)in[i].c,
          tr,
          tc,
          false);
      g2048_write_slot(dir, line, write_k, in[i].v, in[i].id, outv, outid);
      i += 1;
    }
    write_k++;
  }
  for (int k = write_k; k < G2048_SIZE; k++) {
    g2048_write_slot(dir, line, k, 0, 0, outv, outid);
  }
}

bool g2048_board_prepare_move(
    const G2048Board *in,
    G2048Dir dir,
    G2048Board *out,
    G2048AnimSprite *sprites,
    int *sprite_count,
    int *score_delta,
    bool *won_delta)
{
  int outv[G2048_SIZE][G2048_SIZE];
  int outid[G2048_SIZE][G2048_SIZE];
  memset(outv, 0, sizeof(outv));
  memset(outid, 0, sizeof(outid));

  int sc = 0;
  int sd = 0;
  bool wd = false;
  int next = in->next_id;

  for (int line = 0; line < G2048_SIZE; line++) {
    G2048Cell cells[16];
    int n = g2048_read_line(dir, line, in->v, in->id, cells);
    g2048_merge_line(dir, line, cells, n, outv, outid, &next, sprites, &sc, &sd, &wd);
  }

  *sprite_count = sc;
  *score_delta = sd;
  *won_delta = wd;

  if (g2048_board_grids_equal(in->v, outv)) {
    *sprite_count = 0;
    *score_delta = 0;
    *won_delta = false;
    return false;
  }

  out->next_id = next;
  memcpy(out->v, outv, sizeof(out->v));
  memcpy(out->id, outid, sizeof(out->id));
  out->score = in->score + sd;
  out->won = in->won || wd;
  out->rng = in->rng;
  return true;
}
