# SCHEMA

## What this project does
PolyformTown enumerates polyforms by repeatedly attaching a base
boundary tile to existing shapes, canonicalizing each result under
lattice symmetries, and deduplicating by hash. One core pipeline
supports square, triangular, and tetrille-style tagged lattices.

## High-level architecture
- `src/cycle.*`: geometry primitives (`Coord`, `Cycle`, `Poly`),
  transforms, orientation rules, and canonicalization.
- `src/tile.*`: tile parser plus symmetry-variant generation.
- `src/attach.*`: frontier-edge alignment, edge cancellation,
  cycle extraction, and overlap rejection.
- `src/hash.*`: open-addressed hash table for deduping canonical
  polyforms.
- `src/vec.*`: dynamic vector used by level expansion loops.
- `src/count_poly.c` / `src/print_poly.c`: edge-attachment
  enumeration drivers.
- `src/vcomp.*`, `src/count_vcomp.c`, `src/print_vcomp.c`:
  vertex-completion enumeration mode.
- `src/imgtable.c`, `src/tile_to_imgtable.c`: imgtable-style
  conversion and SVG rendering support.

## Build and checks
- Build: `make`
- Full smoke test: `bash tests/smoke.sh`

## Input data and outputs
- Tile definitions live in `tiles/*.tile`.
- A legacy compatibility sample lives in
  `tests/legacy_monomino.tile`.
- Runtime output prints either count-by-level (`poly_count`) or
  canonical boundary rows (`poly_print` / `vcomp_print`).

## Project process/state files (for contributors)
- `RESET.txt` is a restart checklist for an agent/session. It
  points to `PERSONAE.md`, `LESSONS.md`, `FUTURES.md`,
  `README.md`, and source before asking for next steps.
- `SUSPEND.md` is an end-of-pass quality gate. It asks for one
  more refinement pass, consistency checks, and updates to memory
  index files when appropriate.
- `LESSONS.md` and `FUTURES.md` are indexes into `memory/`:
  - `LESSONS.md` points to prioritized operational and technical
    lessons (`memory/Lxxxx.txt`).
  - `FUTURES.md` tracks active and completed roadmap items
    (`memory/Fxxxx.txt`).
- These files are workflow scaffolding for maintainers/agents,
  not runtime requirements for building or running enumerators.

### AGENTS.md vs memory files (Codex workflow)
- If an `AGENTS.md` file is present, it should be treated as the
  authoritative task-policy layer for files in its scope (how to
  edit, test, format, and deliver changes).
- The `memory/` structure (`LESSONS.md`, `FUTURES.md`, and
  `memory/Lxxxx.txt` + `memory/Fxxxx.txt`) is project continuity:
  design lessons, historical constraints, and roadmap direction.
- In practice they are complementary: AGENTS gives immediate
  operating rules; memory preserves longer-term project context.

## Suggested learning path
1. Read `README.md` for the algorithm overview and expected
   sequence values.
2. Skim `RESET.txt`, `SUSPEND.md`, `LESSONS.md`, and
   `FUTURES.md` to understand team process around changes.
3. Trace one `poly_count` level in `src/count_poly.c`.
4. Step through `try_attach_tile_poly` in `src/attach.c`.
5. Read `poly_canonicalize_lattice` in `src/cycle.c` to learn
   dedupe invariants.
6. Compare `count_poly.c` vs `count_vcomp.c` to see frontier-edge
   vs vertex-completion search styles.
7. Run `bash tests/smoke.sh`, then try small custom tiles to
   observe behavior changes.

## Important invariants to keep in mind
- Outer cycle must be CCW; holes must be CW in canonical form.
- For tetrille, coordinates are tagged (`v in {6,4,3}`) and
  translations must be lifted through the 6-system.
- Symmetry handling is lattice-specific (D4 for square, D6-style
  for triangular/tetrille).
- Only canonicalized polyforms are inserted into hash tables.
