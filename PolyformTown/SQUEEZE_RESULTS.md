# Squeeze results: plan vs current code

## Short answer

All planned steps (1 through 6) are now implemented.
At each upgrade step, smoke and comprehensive tests passed.

## Plan alignment snapshot

Reference steps are in `RESTRUCTURE_PLAN.md` lines 148-158.

- Step 1 (poly pipeline extraction): DONE.
- Step 2 (vcomp pipeline extraction): DONE.
- Step 3 (shared transform-count API): DONE.
- Step 4 (shared direction/index API): DONE.
- Step 5 (single tetrille embedding utility): DONE.
- Step 6 (hashed vcomp state dedup): DONE.

## What changed in steps 3-6

- Added `src/lattice.c` and `src/lattice.h` as the shared lattice API.
- Replaced local transform-count functions in `cycle.c` and `tile.c`.
- Replaced local direction/index and embedding logic in `attach.c`.
- Replaced local adjacency logic in `vcomp.c`.
- Routed cycle tetrille embedding through
  `tetrille_embed_point_scaled`.
- Replaced linear `sv_contains` dedup in `vcomp_pipeline.c` with a
  per-level hash set keyed by canonical poly key + sorted hidden set.

## Net accounting

### Step 1 accounting (poly pipeline)

Before Step 1:
- `count_poly.c` + `print_poly.c` = 65 + 66 = 131 lines.

After Step 1:
- `count_poly.c` + `print_poly.c` + `poly_pipeline.c`
- = 33 + 40 + 62 = 135 lines.

Net differential for Step 1:
- +4 lines.

### Step 2 accounting (vcomp pipeline)

Before Step 2:
- `count_vcomp.c` + `print_vcomp.c` = 156 + 150 = 306 lines.

After Step 2:
- `count_vcomp.c` + `print_vcomp.c` + `vcomp_pipeline.c`
- = 34 + 50 + 168 = 252 lines.

Net differential for Step 2:
- -54 lines.

### Steps 3-5 accounting (shared lattice/math utilities)

Before Steps 3-5:
- `cycle.c` + `tile.c` + `attach.c` + `vcomp.c`
- = 464 + 278 + 473 + 183 = 1398 lines.

After Steps 3-5:
- updated originals + new shared API
- = (450 + 272 + 411 + 167) + (73 + 13)
- = 1386 lines.

Net differential for Steps 3-5:
- -12 lines.

### Step 6 accounting (hashed vcomp state dedup)

Before Step 6:
- `vcomp_pipeline.c` = 168 lines.

After Step 6:
- `vcomp_pipeline.c` = 282 lines.

Net differential for Step 6:
- +114 lines.
- tradeoff accepted for expected asymptotic wins in duplicate-state
  detection under larger traversals.

## Verification

- Existing smoke test passes.
- Comprehensive suite passes and exits with `0`.
- Coverage includes poly/vcomp count and print checks across
  monomino, domino, chair, triangle, hexagon, hh, kite, and hat.

## Further squeeze ideas

1. Add microbench timing mode for vcomp traversal to quantify step-6
   speed gains directly.
2. Add unit tests for `lattice_direction_index` and
   `lattice_transform_count`.
3. Add tests that stress high-duplicate vcomp states to validate hash
   dedup behavior and collision handling.
