# Duplicate code and restructuring plan

## Scope

This note analyzes duplication and waste in `src/` and proposes a
layered structure that separates low-level math from high-level
enumeration and output programs.

## Current duplication and waste

### 1) Nearly identical growth loops in count/print binaries

`count_poly.c` and `print_poly.c` contain the same frontier-growth
loop with mostly output differences.

- same setup/load/seed path
- same attach + canonicalize + hash dedup path
- different final reporting behavior

Evidence:
- `src/count_poly.c` lines 9-64
- `src/print_poly.c` lines 9-66

### 2) Nearly identical state machines in vcomp binaries

`count_vcomp.c` and `print_vcomp.c` duplicate:

- `State` / `StateVec` definitions
- `sv_init/sv_destroy/sv_push`
- `coord_cmp`, `poly_equal_local`, `state_equal`, `sv_contains`
- traversal over levels and calls to `enumerate_vertex_completions`

Evidence:
- `src/count_vcomp.c` lines 13-156
- `src/print_vcomp.c` lines 13-150

### 3) Repeated lattice transform cardinality logic

`lattice_transform_count()` appears in at least two files with
slightly different default behavior.

Evidence:
- `src/cycle.c` lines 138-142
- `src/tile.c` lines 7-12

### 4) Repeated direction mapping logic

Direction sets for triangular/tetrille unit vectors are repeated as
if-chains in multiple places.

Evidence:
- `src/attach.c` lines 23-62
- `src/tetrille.c` lines 3-11 and 117-142
- `src/vcomp.c` lines 21-36

### 5) Repeated geometric embedding details

Tetrille embedding formulas are duplicated locally instead of routed
through one utility surface.

Evidence:
- `src/attach.c` lines 66-86
- `src/tetrille.c` lines 144-155

### 6) O(n^2) dedup paths likely to dominate bigger runs

Several lookup paths scan linearly instead of using hashed sets.

Evidence:
- `sv_contains` linear scan in `src/count_vcomp.c` lines 70-73
- same in `src/print_vcomp.c` lines 69-72
- vertex uniq via `coord_in_list` in `src/vcomp.c` lines 16-19,
  76-86, 116-123

## Proposed layering

Split code into explicit layers with one-way dependencies.

### Layer 0: types and constants

- `geom_types.h`: Coord, Edge, Cycle, Poly, limits
- `lattice.h`: lattice enum + transform counts + dir counts

No dependency on higher layers.

### Layer 1: lattice math kernels

- `lattice_square.c`
- `lattice_tri.c`
- `lattice_tetrille.c`

Each exposes:
- coordinate transforms
- adjacency and direction index mapping
- point-on-segment and embedding helpers

These files are pure math and deterministic.

### Layer 2: cycle/poly canonical geometry

- `cycle_ops.c`
- `poly_canon.c`

Responsibilities:
- orientation normalization (outer CCW, holes CW)
- canonical shifts, sorting, transforms
- lattice-agnostic wrappers calling Layer 1

### Layer 3: mutation/enumeration primitives

- `attach_engine.c`
- `vcomp_engine.c`

Responsibilities:
- frontier extraction
- tile attach trials
- vcomp expansion

Must call Layer 2 canonicalization before hash insertion.

### Layer 4: reusable pipelines

- `pipeline_poly.c`
- `pipeline_vcomp.c`

These own the shared BFS/level loops and expose callbacks:
- `on_level(level, states)`
- `on_emit(state)`

Count and print tools become thin wrappers around this layer.

### Layer 5: binaries and output adapters

- `count_poly.c`, `print_poly.c`
- `count_vcomp.c`, `print_vcomp.c`
- `tile_to_imgtable.c`, `imgtable.c`

Only orchestration/CLI/formatting should remain here.

## Dependency graph

Allowed imports are downward only:

- L5 -> L4 -> L3 -> L2 -> L1 -> L0
- L5 may also use L2 for formatting helpers if needed
- no upward or cross-layer back edges

## Concrete extraction sequence

1. Extract shared poly growth loop from `count_poly.c` and
   `print_poly.c` into `pipeline_poly.c` with callbacks.
2. Extract shared vcomp state machine into `pipeline_vcomp.c`.
3. Move `lattice_transform_count` to `lattice.h/.c` and replace
   file-local copies.
4. Unify direction/index APIs in lattice math layer.
5. Route tetrille embedding helpers through one implementation.
6. Replace repeated linear dedup in vcomp pipeline with hashed
   state keys (`poly key + sorted hidden`).

## Expected gains

- less duplicate maintenance across count/print binaries
- clearer math-vs-orchestration boundaries
- lower bug risk from inconsistent lattice logic
- better scaling in vertex-completion enumeration
- easier targeted testing at each layer

## Validation approach after refactor

- keep existing smoke tests green
- add unit tests for lattice direction/index functions
- add unit tests for canonicalization invariants
- cross-check counts/prints before and after refactor
