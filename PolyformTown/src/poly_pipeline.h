#ifndef POLY_PIPELINE_H
#define POLY_PIPELINE_H

#include "tile.h"
#include "vec.h"

typedef int (*PolyLevelFn)(int level,
                           const PolyVec *cur,
                           const Tile *tile,
                           void *userdata);

void run_poly_levels(const Tile *tile,
                     int max_n,
                     int live_only,
                     PolyLevelFn on_level,
                     void *userdata);

#endif
