#include "slitherlink_board.h"

#include <stdlib.h>
#include <string.h>

#define SL_MAX_W 16
#define SL_MAX_H 16

/* #region rng */

static uint32_t sl_rng_u32(uint32_t *rng)
{
  uint32_t x = *rng;
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  *rng = x ? x : 2463534242u;
  return *rng;
}

static int sl_rng_int(uint32_t *rng, int n)
{
  if (n <= 0) {
    return 0;
  }
  return (int)(sl_rng_u32(rng) % (uint32_t)n);
}

/* #endregion */

/* #region indexing */

static size_t sl_h_idx(int w, int ri, int ci)
{
  return (size_t)ri * (size_t)w + (size_t)ci;
}

static size_t sl_v_idx(int w, int ri, int ci)
{
  return (size_t)ri * (size_t)(w + 1u) + (size_t)ci;
}

static bool sl_cell_on(const SlitherlinkBoard *b, int r, int c)
{
  if (r < 0 || c < 0 || r >= b->h || c >= b->w) {
    return false;
  }
  if (b->shape == SL_SHAPE_RECT) {
    return true;
  }
  return b->cell_active[(size_t)r * (size_t)b->w + (size_t)c];
}

/* #endregion */

void sl_board_init_defaults(SlitherlinkBoard *b)
{
  memset(b, 0, sizeof(*b));
  b->w = 8;
  b->h = 8;
  b->shape = SL_SHAPE_RECT;
  b->difficulty = SL_DIFF_NORMAL;
  b->rng = 1u;
}

void sl_board_free(SlitherlinkBoard *b)
{
  free(b->h_user);
  free(b->h_sol);
  free(b->v_user);
  free(b->v_sol);
  free(b->clue);
  free(b->cell_active);
  b->h_user = b->h_sol = NULL;
  b->v_user = b->v_sol = NULL;
  b->clue = NULL;
  b->cell_active = NULL;
}

bool sl_board_alloc(SlitherlinkBoard *b, int w, int h)
{
  if (w < 3 || h < 3 || w > SL_MAX_W || h > SL_MAX_H) {
    return false;
  }
  sl_board_free(b);
  b->w = w;
  b->h = h;
  size_t nh = (size_t)(h + 1) * (size_t)w;
  size_t nv = (size_t)h * (size_t)(w + 1u);
  size_t nc = (size_t)w * (size_t)h;
  b->h_user = calloc(nh, sizeof(bool));
  b->h_sol = calloc(nh, sizeof(bool));
  b->v_user = calloc(nv, sizeof(bool));
  b->v_sol = calloc(nv, sizeof(bool));
  b->clue = malloc(nc);
  b->cell_active = calloc(nc, sizeof(bool));
  if (!b->h_user || !b->h_sol || !b->v_user || !b->v_sol || !b->clue || !b->cell_active) {
    sl_board_free(b);
    return false;
  }
  memset(b->clue, -1, nc);
  return true;
}

/* #region solution from active cells */

static void sl_build_solution_edges(SlitherlinkBoard *b)
{
  int w = b->w;
  int h = b->h;
  memset(b->h_sol, 0, (size_t)(h + 1) * (size_t)w * sizeof(bool));
  memset(b->v_sol, 0, (size_t)h * (size_t)(w + 1u) * sizeof(bool));

  for (int r = 0; r <= h; r++) {
    for (int c = 0; c < w; c++) {
      bool up = sl_cell_on(b, r - 1, c);
      bool dn = sl_cell_on(b, r, c);
      b->h_sol[sl_h_idx(w, r, c)] = up != dn;
    }
  }
  for (int r = 0; r < h; r++) {
    for (int c = 0; c <= w; c++) {
      bool lf = sl_cell_on(b, r, c - 1);
      bool rt = sl_cell_on(b, r, c);
      b->v_sol[sl_v_idx(w, r, c)] = lf != rt;
    }
  }
}

static int sl_cell_sol_clue(const SlitherlinkBoard *b, int r, int c)
{
  int w = b->w;
  int n = 0;
  if (b->h_sol[sl_h_idx(w, r, c)]) {
    n++;
  }
  if (b->h_sol[sl_h_idx(w, r + 1, c)]) {
    n++;
  }
  if (b->v_sol[sl_v_idx(w, r, c)]) {
    n++;
  }
  if (b->v_sol[sl_v_idx(w, r, c + 1)]) {
    n++;
  }
  return n;
}

/* #endregion */

/* #region polyomino */

static float sl_fill_ratio(SlDifficulty d)
{
  switch (d) {
  case SL_DIFF_EASY:
    return 0.72f;
  case SL_DIFF_HARD:
    return 0.42f;
  default:
    return 0.55f;
  }
}

static bool sl_grow_blob(SlitherlinkBoard *b, uint32_t *rng)
{
  int w = b->w;
  int h = b->h;
  size_t nc = (size_t)w * (size_t)h;
  memset(b->cell_active, 0, nc * sizeof(bool));

  int target = (int)((float)(w * h) * sl_fill_ratio(b->difficulty));
  if (target < 4) {
    target = 4;
  }
  if (target > w * h - 1) {
    target = w * h - 1;
  }

  int sr = sl_rng_int(rng, h);
  int sc = sl_rng_int(rng, w);
  b->cell_active[(size_t)sr * (size_t)w + (size_t)sc] = true;
  int count = 1;

  while (count < target) {
    int picks[64][2];
    int pc = 0;
    for (int r = 0; r < h && pc < 64; r++) {
      for (int c = 0; c < w && pc < 64; c++) {
        size_t ix = (size_t)r * (size_t)w + (size_t)c;
        if (!b->cell_active[ix]) {
          continue;
        }
        const int dr[4] = {-1, 1, 0, 0};
        const int dc[4] = {0, 0, -1, 1};
        for (int k = 0; k < 4; k++) {
          int nr = r + dr[k];
          int nc = c + dc[k];
          if (nr < 0 || nc < 0 || nr >= h || nc >= w) {
            continue;
          }
          size_t nix = (size_t)nr * (size_t)w + (size_t)nc;
          if (!b->cell_active[nix] && pc < 64) {
            picks[pc][0] = nr;
            picks[pc][1] = nc;
            pc++;
          }
        }
      }
    }
    if (pc == 0) {
      break;
    }
    int j = sl_rng_int(rng, pc);
    int nr = picks[j][0];
    int nc = picks[j][1];
    b->cell_active[(size_t)nr * (size_t)w + (size_t)nc] = true;
    count++;
  }

  /* Require hole-free simply connected polyomino: flood fill exterior from border should not reach interior holes */
  bool *vis = calloc(nc, sizeof(bool));
  if (!vis) {
    return false;
  }
  int *q = malloc(nc * sizeof(int));
  if (!q) {
    free(vis);
    return false;
  }
  int qh = 0;
  int qt = 0;
  for (int c = 0; c < w; c++) {
    if (!b->cell_active[0u * (size_t)w + (size_t)c]) {
      q[qt++] = c;
      vis[0u * (size_t)w + (size_t)c] = true;
    }
    if (!b->cell_active[(size_t)(h - 1) * (size_t)w + (size_t)c]) {
      size_t ix = (size_t)(h - 1) * (size_t)w + (size_t)c;
      if (!vis[ix]) {
        q[qt++] = (int)ix;
        vis[ix] = true;
      }
    }
  }
  for (int r = 0; r < h; r++) {
    if (!b->cell_active[(size_t)r * (size_t)w]) {
      size_t ix = (size_t)r * (size_t)w;
      if (!vis[ix]) {
        q[qt++] = (int)ix;
        vis[ix] = true;
      }
    }
    if (!b->cell_active[(size_t)r * (size_t)w + (size_t)(w - 1)]) {
      size_t ix = (size_t)r * (size_t)w + (size_t)(w - 1);
      if (!vis[ix]) {
        q[qt++] = (int)ix;
        vis[ix] = true;
      }
    }
  }
  const int dr[4] = {-1, 1, 0, 0};
  const int dc[4] = {0, 0, -1, 1};
  while (qh < qt) {
    int cur = q[qh++];
    int r = cur / w;
    int c = cur % w;
    for (int k = 0; k < 4; k++) {
      int nr = r + dr[k];
      int nc = c + dc[k];
      if (nr < 0 || nc < 0 || nr >= h || nc >= w) {
        continue;
      }
      size_t nix = (size_t)nr * (size_t)w + (size_t)nc;
      if (vis[nix] || b->cell_active[nix]) {
        continue;
      }
      vis[nix] = true;
      q[qt++] = (int)nix;
    }
  }
  bool hole = false;
  for (size_t i = 0; i < nc; i++) {
    if (!b->cell_active[i] && !vis[i]) {
      hole = true;
      break;
    }
  }
  free(q);
  free(vis);
  return !hole;
}

/* #endregion */

/* #region solver (-1 unknown, 0 off, 1 on) */

typedef struct {
  int w, h;
  int8_t *hst;
  int8_t *vst;
  const int8_t *clue;
  const bool *active;
  SlShape shape;
} SlSolve;

static void sl_vertex_edges(const SlSolve *s, int vr, int vc, int *eidx, int8_t *eorh, int *ec)
{
  int w = s->w;
  int h = s->h;
  int n = 0;
  if (vc > 0) {
    eidx[n] = (int)sl_h_idx(w, vr, vc - 1);
    eorh[n] = 1; /* h */
    n++;
  }
  if (vc < w) {
    eidx[n] = (int)sl_h_idx(w, vr, vc);
    eorh[n] = 1;
    n++;
  }
  if (vr > 0) {
    eidx[n] = (int)sl_v_idx(w, vr - 1, vc);
    eorh[n] = 0;
    n++;
  }
  if (vr < h) {
    eidx[n] = (int)sl_v_idx(w, vr, vc);
    eorh[n] = 0;
    n++;
  }
  *ec = n;
}

static int8_t *sl_hptr(SlSolve *s, int i, int is_h)
{
  return is_h ? &s->hst[(size_t)i] : &s->vst[(size_t)i];
}

static bool sl_propagate(SlSolve *s)
{
  int w = s->w;
  int h = s->h;
  bool progress = true;
  while (progress) {
    progress = false;

    for (int r = 0; r < h; r++) {
      for (int c = 0; c < w; c++) {
        if (s->shape == SL_SHAPE_BLOB && !s->active[(size_t)r * (size_t)w + (size_t)c]) {
          continue;
        }
        int clue = s->clue[(size_t)r * (size_t)w + (size_t)c];
        if (clue < 0) {
          continue;
        }
        int idx[4];
        int8_t kind[4];
        int n = 0;
        idx[n] = (int)sl_h_idx(w, r, c);
        kind[n++] = 1;
        idx[n] = (int)sl_h_idx(w, r + 1, c);
        kind[n++] = 1;
        idx[n] = (int)sl_v_idx(w, r, c);
        kind[n++] = 0;
        idx[n] = (int)sl_v_idx(w, r, c + 1);
        kind[n++] = 0;

        int on = 0;
        int off = 0;
        int un = 0;
        for (int k = 0; k < 4; k++) {
          int8_t *p = sl_hptr(s, idx[k], kind[k]);
          if (*p < 0) {
            un++;
          } else if (*p == 1) {
            on++;
          } else {
            off++;
          }
        }
        if (on > clue || off > (4 - clue)) {
          return false;
        }
        int need = clue - on;
        if (need < 0 || need > un) {
          return false;
        }
        if (need == 0) {
          for (int k = 0; k < 4; k++) {
            int8_t *p = sl_hptr(s, idx[k], kind[k]);
            if (*p < 0) {
              *p = 0;
              progress = true;
            }
          }
        } else if (need == un) {
          for (int k = 0; k < 4; k++) {
            int8_t *p = sl_hptr(s, idx[k], kind[k]);
            if (*p < 0) {
              *p = 1;
              progress = true;
            }
          }
        }
      }
    }

    for (int vr = 0; vr <= h; vr++) {
      for (int vc = 0; vc <= w; vc++) {
        int eidx[4];
        int8_t ekh[4];
        int ec = 0;
        (void)sl_vertex_edges(s, vr, vc, eidx, ekh, &ec);
        int on = 0;
        int un = 0;
        for (int k = 0; k < ec; k++) {
          int8_t v = *sl_hptr(s, eidx[k], ekh[k]);
          if (v == 1) {
            on++;
          } else if (v < 0) {
            un++;
          }
        }
        if (on > 2) {
          return false;
        }
        if (on == 2) {
          for (int k = 0; k < ec; k++) {
            int8_t *p = sl_hptr(s, eidx[k], ekh[k]);
            if (*p < 0) {
              *p = 0;
              progress = true;
            }
          }
        } else if (on == 1 && un == 1) {
          for (int k = 0; k < ec; k++) {
            int8_t *p = sl_hptr(s, eidx[k], ekh[k]);
            if (*p < 0) {
              *p = 1;
              progress = true;
            }
          }
        } else if (on == 0 && un == 1) {
          for (int k = 0; k < ec; k++) {
            int8_t *p = sl_hptr(s, eidx[k], ekh[k]);
            if (*p < 0) {
              *p = 0;
              progress = true;
            }
          }
        }
      }
    }
  }
  return true;
}

static bool sl_vertices_balanced_full(const SlSolve *s)
{
  int w = s->w;
  int h = s->h;
  for (int vr = 0; vr <= h; vr++) {
    for (int vc = 0; vc <= w; vc++) {
      int eidx[4];
      int8_t ekh[4];
      int ec = 0;
      sl_vertex_edges(s, vr, vc, eidx, ekh, &ec);
      int on = 0;
      for (int k = 0; k < ec; k++) {
        const int8_t *p = ekh[k] ? &s->hst[(size_t)eidx[k]] : &s->vst[(size_t)eidx[k]];
        if (*p == 1) {
          on++;
        }
      }
      if (on != 0 && on != 2) {
        return false;
      }
    }
  }
  return true;
}

static void sl_solve_copy_st(const SlitherlinkBoard *b, SlSolve *s, int8_t *hst, int8_t *vst)
{
  s->w = b->w;
  s->h = b->h;
  s->hst = hst;
  s->vst = vst;
  s->clue = b->clue;
  s->active = b->cell_active;
  s->shape = b->shape;
  size_t nh = (size_t)(b->h + 1) * (size_t)b->w;
  size_t nv = (size_t)b->h * (size_t)(b->w + 1u);
  for (size_t i = 0; i < nh; i++) {
    hst[i] = -1;
  }
  for (size_t i = 0; i < nv; i++) {
    vst[i] = -1;
  }
  for (int r = 0; r <= b->h; r++) {
    for (int c = 0; c < b->w; c++) {
      if (b->h_user[sl_h_idx(b->w, r, c)]) {
        hst[sl_h_idx(b->w, r, c)] = 1;
      } else {
        /* If user drew "off" we do not track; leave unknown unless we add marks */
      }
    }
  }
  for (int r = 0; r < b->h; r++) {
    for (int c = 0; c <= b->w; c++) {
      if (b->v_user[sl_v_idx(b->w, r, c)]) {
        vst[sl_v_idx(b->w, r, c)] = 1;
      }
    }
  }
}

static int sl_dfs(SlSolve *s, int depth, int limit, int8_t *bak_h, int8_t *bak_v)
{
  if (!sl_propagate(s)) {
    return 0;
  }
  int w = s->w;
  int h = s->h;
  size_t nh = (size_t)(h + 1) * (size_t)w;
  size_t nv = (size_t)h * (size_t)(w + 1u);

  int bi = -1;
  int8_t bh = 0;
  for (size_t i = 0; i < nh; i++) {
    if (s->hst[i] < 0) {
      bi = (int)i;
      bh = 1;
      break;
    }
  }
  if (bi < 0) {
    for (size_t i = 0; i < nv; i++) {
      if (s->vst[i] < 0) {
        bi = (int)i;
        bh = 0;
        break;
      }
    }
  }
  if (bi < 0) {
    if (!sl_vertices_balanced_full(s)) {
      return 0;
    }
    return 1;
  }
  if (depth > 12000) {
    return limit + 1;
  }

  int8_t *hp = sl_hptr(s, bi, bh);
  int8_t saved = *hp;
  int total = 0;

  memcpy(bak_h, s->hst, nh * sizeof(int8_t));
  memcpy(bak_v, s->vst, nv * sizeof(int8_t));

  *hp = 0;
  int c0 = sl_dfs(s, depth + 1, limit - total, bak_h, bak_v);
  total += c0;
  memcpy(s->hst, bak_h, nh * sizeof(int8_t));
  memcpy(s->vst, bak_v, nv * sizeof(int8_t));
  if (total >= limit) {
    *hp = saved;
    return total;
  }

  *hp = 1;
  int c1 = sl_dfs(s, depth + 1, limit - total, bak_h, bak_v);
  total += c1;
  memcpy(s->hst, bak_h, nh * sizeof(int8_t));
  memcpy(s->vst, bak_v, nv * sizeof(int8_t));

  *hp = saved;
  return total;
}

int sl_count_solutions(const SlitherlinkBoard *b, int limit)
{
  if (limit <= 0) {
    return 0;
  }
  size_t nh = (size_t)(b->h + 1) * (size_t)b->w;
  size_t nv = (size_t)b->h * (size_t)(b->w + 1u);
  int8_t *hst = malloc(nh * sizeof(int8_t));
  int8_t *vst = malloc(nv * sizeof(int8_t));
  int8_t *bak_h = malloc(nh * sizeof(int8_t));
  int8_t *bak_v = malloc(nv * sizeof(int8_t));
  if (!hst || !vst || !bak_h || !bak_v) {
    free(hst);
    free(vst);
    free(bak_h);
    free(bak_v);
    return 0;
  }
  SlSolve s;
  sl_solve_copy_st(b, &s, hst, vst);
  int r = sl_dfs(&s, 0, limit, bak_h, bak_v);
  free(hst);
  free(vst);
  free(bak_h);
  free(bak_v);
  return r;
}

/* #endregion */

/* #region strip clues */

static int sl_clue_strip_budget(SlDifficulty d)
{
  switch (d) {
  case SL_DIFF_EASY:
    return 6;
  case SL_DIFF_HARD:
    return 28;
  default:
    return 16;
  }
}

static bool sl_strip_clues(SlitherlinkBoard *b, uint32_t *rng)
{
  int w = b->w;
  int h = b->h;
  int *cells = malloc((size_t)w * (size_t)h * sizeof(int));
  if (!cells) {
    return false;
  }
  int n = 0;
  for (int r = 0; r < h; r++) {
    for (int c = 0; c < w; c++) {
      if (b->shape == SL_SHAPE_BLOB && !b->cell_active[(size_t)r * (size_t)w + (size_t)c]) {
        continue;
      }
      cells[n++] = r * w + c;
    }
  }
  for (int i = n - 1; i > 0; i--) {
    int j = sl_rng_int(rng, i + 1);
    int t = cells[i];
    cells[i] = cells[j];
    cells[j] = t;
  }

  int removed = 0;
  int budget = sl_clue_strip_budget(b->difficulty);
  for (int i = 0; i < n && removed < budget; i++) {
    int ix = cells[i];
    int r = ix / w;
    int c = ix % w;
    int8_t old = b->clue[(size_t)r * (size_t)w + (size_t)c];
    if (old < 0) {
      continue;
    }
    b->clue[(size_t)r * (size_t)w + (size_t)c] = -1;
    int sc = sl_count_solutions(b, 2);
    if (sc != 1) {
      b->clue[(size_t)r * (size_t)w + (size_t)c] = old;
    } else {
      removed++;
    }
  }
  free(cells);
  return sl_count_solutions(b, 2) == 1;
}

/* #endregion */

bool sl_board_generate(SlitherlinkBoard *b)
{
  if (!b->h_user || !b->h_sol || !b->v_user || !b->v_sol || !b->clue || !b->cell_active) {
    return false;
  }
  uint32_t rng = b->rng;
  for (int attempt = 0; attempt < 80; attempt++) {
    sl_rng_u32(&rng);
    memset(b->h_user, 0, (size_t)(b->h + 1) * (size_t)b->w * sizeof(bool));
    memset(b->v_user, 0, (size_t)b->h * (size_t)(b->w + 1u) * sizeof(bool));
    b->solved = false;

    if (b->shape == SL_SHAPE_RECT) {
      size_t nc = (size_t)b->w * (size_t)b->h;
      memset(b->cell_active, 1, nc * sizeof(bool));
    } else {
      if (!sl_grow_blob(b, &rng)) {
        continue;
      }
    }

    sl_build_solution_edges(b);

    for (int r = 0; r < b->h; r++) {
      for (int c = 0; c < b->w; c++) {
        if (b->shape == SL_SHAPE_BLOB && !b->cell_active[(size_t)r * (size_t)b->w + (size_t)c]) {
          b->clue[(size_t)r * (size_t)b->w + (size_t)c] = -1;
        } else {
          b->clue[(size_t)r * (size_t)b->w + (size_t)c] = (int8_t)sl_cell_sol_clue(b, r, c);
        }
      }
    }

    if (!sl_strip_clues(b, &rng)) {
      continue;
    }
    b->rng = rng;
    return true;
  }
  b->rng = rng;
  return false;
}

/* #region user API */

void sl_board_toggle_h(SlitherlinkBoard *b, int ri, int ci)
{
  if (ri < 0 || ci < 0 || ri > b->h || ci >= b->w) {
    return;
  }
  size_t ix = sl_h_idx(b->w, ri, ci);
  b->h_user[ix] = !b->h_user[ix];
  sl_board_check_solved(b);
}

void sl_board_toggle_v(SlitherlinkBoard *b, int ri, int ci)
{
  if (ri < 0 || ci < 0 || ri >= b->h || ci > b->w) {
    return;
  }
  size_t ix = sl_v_idx(b->w, ri, ci);
  b->v_user[ix] = !b->v_user[ix];
  sl_board_check_solved(b);
}

void sl_board_clear_user(SlitherlinkBoard *b)
{
  memset(b->h_user, 0, (size_t)(b->h + 1) * (size_t)b->w * sizeof(bool));
  memset(b->v_user, 0, (size_t)b->h * (size_t)(b->w + 1u) * sizeof(bool));
  b->solved = false;
}

static bool sl_edges_match_sol(const SlitherlinkBoard *b)
{
  int w = b->w;
  int h = b->h;
  for (int r = 0; r <= h; r++) {
    for (int c = 0; c < w; c++) {
      if (b->h_user[sl_h_idx(w, r, c)] != b->h_sol[sl_h_idx(w, r, c)]) {
        return false;
      }
    }
  }
  for (int r = 0; r < h; r++) {
    for (int c = 0; c <= w; c++) {
      if (b->v_user[sl_v_idx(w, r, c)] != b->v_sol[sl_v_idx(w, r, c)]) {
        return false;
      }
    }
  }
  return true;
}

static bool sl_clues_satisfied(const SlitherlinkBoard *b)
{
  int w = b->w;
  int h = b->h;
  for (int r = 0; r < h; r++) {
    for (int c = 0; c < w; c++) {
      if (b->shape == SL_SHAPE_BLOB && !b->cell_active[(size_t)r * (size_t)w + (size_t)c]) {
        continue;
      }
      int clue = b->clue[(size_t)r * (size_t)w + (size_t)c];
      if (clue < 0) {
        continue;
      }
      int n = 0;
      if (b->h_user[sl_h_idx(w, r, c)]) {
        n++;
      }
      if (b->h_user[sl_h_idx(w, r + 1, c)]) {
        n++;
      }
      if (b->v_user[sl_v_idx(w, r, c)]) {
        n++;
      }
      if (b->v_user[sl_v_idx(w, r, c + 1)]) {
        n++;
      }
      if (n != clue) {
        return false;
      }
    }
  }
  return true;
}

void sl_board_check_solved(SlitherlinkBoard *b)
{
  b->solved = sl_clues_satisfied(b) && sl_edges_match_sol(b);
}

/* #endregion */
