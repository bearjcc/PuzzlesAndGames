#ifndef PG_THEME_H
#define PG_THEME_H

#include <SDL.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Visual language: quiet "newsprint" puzzles (2048, NYT-style grids).
 * Flat fills, crisp vector-like strokes, bundled sans text, no skeuomorphic chrome.
 * Games should take colors and stroke rules from here instead of ad hoc literals.
 */

/* #region Core paper and ink */

#define PG_COLOR_PAPER ((SDL_Color){0xF9, 0xF8, 0xF6, 255})
#define PG_COLOR_SURFACE ((SDL_Color){0xF2, 0xF0, 0xED, 255})
#define PG_COLOR_SURFACE_STRONG ((SDL_Color){0xE8, 0xE4, 0xDE, 255})
#define PG_COLOR_RULE ((SDL_Color){0xC8, 0xC2, 0xBA, 255})
#define PG_COLOR_INK ((SDL_Color){0x1A, 0x1A, 0x1A, 255})
#define PG_COLOR_INK_MUTED ((SDL_Color){0x5C, 0x5C, 0x58, 255})
#define PG_COLOR_INK_FAINT ((SDL_Color){0x8A, 0x88, 0x84, 255})
#define PG_COLOR_ACCENT_SELECT ((SDL_Color){0xD4, 0xD1, 0xCC, 255})
#define PG_COLOR_OVERLAY_SCRIM ((SDL_Color){26, 26, 26, 165})
#define PG_COLOR_OVERLAY_SCRIM_LIGHT ((SDL_Color){249, 248, 246, 220})
#define PG_COLOR_INVERSE_INK ((SDL_Color){0xFC, 0xFA, 0xF7, 255})
#define PG_COLOR_INVERSE_MUTED ((SDL_Color){0xC8, 0xC6, 0xC2, 255})

/* #endregion */

/* #region Semantic (status without loud hues) */

#define PG_COLOR_STATE_POSITIVE ((SDL_Color){0x2E, 0x6B, 0x4E, 255})
#define PG_COLOR_STATE_NEGATIVE ((SDL_Color){0x8B, 0x3A, 0x3A, 255})
#define PG_COLOR_STATE_CAUTION ((SDL_Color){0xB0, 0x8A, 0x2E, 255})

/* #endregion */

/* #region 2048 tiles (classic ladder, slightly calmer on large displays) */

#define PG_G2048_FRAME ((SDL_Color){0xBB, 0xB6, 0xAE, 255})
#define PG_G2048_CELL_EMPTY ((SDL_Color){0xD1, 0xCC, 0xC4, 255})
#define PG_G2048_TILE_0_BG PG_G2048_CELL_EMPTY
#define PG_G2048_TILE_0_FG ((SDL_Color){0xE6, 0xE2, 0xDC, 255})
#define PG_G2048_TILE_2_BG ((SDL_Color){0xEE, 0xEB, 0xE6, 255})
#define PG_G2048_TILE_2_FG PG_COLOR_INK_MUTED
#define PG_G2048_TILE_4_BG ((SDL_Color){0xE8, 0xE2, 0xD8, 255})
#define PG_G2048_TILE_4_FG PG_COLOR_INK_MUTED
#define PG_G2048_TILE_8_BG ((SDL_Color){0xE8, 0xB8, 0x8A, 255})
#define PG_G2048_TILE_8_FG ((SDL_Color){0xFC, 0xFA, 0xF7, 255})
#define PG_G2048_TILE_16_BG ((SDL_Color){0xE0, 0x9A, 0x72, 255})
#define PG_G2048_TILE_16_FG ((SDL_Color){0xFC, 0xFA, 0xF7, 255})
#define PG_G2048_TILE_32_BG ((SDL_Color){0xD6, 0x7A, 0x62, 255})
#define PG_G2048_TILE_32_FG ((SDL_Color){0xFC, 0xFA, 0xF7, 255})
#define PG_G2048_TILE_64_BG ((SDL_Color){0xCC, 0x5A, 0x4C, 255})
#define PG_G2048_TILE_64_FG ((SDL_Color){0xFC, 0xFA, 0xF7, 255})
#define PG_G2048_TILE_HIGH_BG ((SDL_Color){0xD4, 0xBA, 0x6A, 255})
#define PG_G2048_TILE_HIGH_FG ((SDL_Color){0xFC, 0xFA, 0xF7, 255})

/* #endregion */

/* #region Slitherlink */

#define PG_SLITHER_DOT ((SDL_Color){0x55, 0x55, 0x52, 255})
#define PG_SLITHER_BLOB_INACTIVE ((SDL_Color){0xEB, 0xE8, 0xE4, 255})
#define PG_SLITHER_CLUE PG_COLOR_INK
#define PG_SLITHER_EDGE_USER ((SDL_Color){0x1A, 0x1A, 0x1A, 255})
#define PG_SLITHER_EDGE_SOL ((SDL_Color){0x3D, 0x6B, 0x4F, 200})

/* #endregion */

/**
 * Clear the render target to the standard page background (call instead of black clear).
 */
void pg_theme_clear_paper(SDL_Renderer *renderer);

/**
 * Hairline-ish stroke width from a layout reference (cell size, board side, etc.).
 */
float pg_theme_stroke_px(float reference_px, float ratio);

#ifdef __cplusplus
}
#endif

#endif
