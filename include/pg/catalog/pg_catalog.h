#ifndef PG_CATALOG_H
#define PG_CATALOG_H

#include "pg/app.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct PgCatalogGameSpec {
  const char *const *cli_names; /* NULL-terminated list of accepted argv[1] strings */
  const PgGameVtable *(*vt_fn)(void);
  const char *window_title;
  const char *menu_label;
  int default_win_w;
  int default_win_h;
} PgCatalogGameSpec;

const PgGameVtable *pg_catalog_game_vt(void);
int pg_catalog_game_count(void);
const PgCatalogGameSpec *pg_catalog_game_spec(int index);
const PgCatalogGameSpec *pg_catalog_find_game_by_cli_name(const char *name);

bool pg_catalog_launch(PgApp *app);
bool pg_catalog_launch_from_renderer(SDL_Renderer *renderer);

#ifdef __cplusplus
}
#endif

#endif
