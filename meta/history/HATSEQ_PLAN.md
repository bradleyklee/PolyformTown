# Hatseq implementation plan

## Task restatement

Add a separate vertex-completion workflow named `hatseq`.
It should mirror the existing completion growth style but use
hat-sequence constraints:

- only complete valence-4 and valence-6 vertices;
- only use non-reflected tile orientations;
- optional `double check` mode that rejects emitted states if
  any relevant boundary vertex is dead (has no completion).

## Implementation steps

1. Extend completion engine helpers in `src/vcomp.*` so callers can
   request rotation-only variants and probe whether a target vertex
   has at least one completion.
2. Add a new pipeline `src/hatseq_pipeline.*` that:
   - tracks states by `(poly, hidden)` as in vcomp,
   - expands only vertices with tag 4 or 6,
   - uses rotation-only completion generation,
   - optionally filters emitted states via dead-vertex checks.
3. Add `src/count_hatseq.c` binary and Makefile target
   `hatseq_count`.
4. Document CLI usage in `README.md` and add a focused smoke check.
5. Validate with build + smoke + comprehensive + a reproducible
   no-reflection hatseq run.

## Validation targets

- `cd PolyformTown && make clean && make`
- `cd PolyformTown && bash tests/smoke.sh`
- `cd PolyformTown && bash tests/comprehensive.sh`
- `cd PolyformTown && ./hatseq_count 2 tiles/hat.tile`
- `cd PolyformTown && ./hatseq_count 2 tiles/hat.tile --double-check`

## Consolidated notes

This file now serves as the single hatseq plan/history note.
Follow-up pass notes were consolidated here and removed to keep
history uncluttered.
