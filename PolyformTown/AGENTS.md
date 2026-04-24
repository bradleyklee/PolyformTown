# AGENTS.md

Scope: this file applies to the repository rooted at `PolyformTown/`.

## Purpose
AGENTS is the runtime entry point for contributor/agent policy.
Keep this file short and procedural. Put durable process state in
`meta/`.

## Startup pipeline
1. Read this file.
2. Read, in order:
   - `meta/RESET.md`
   - `meta/PERSONAE.md`
   - `meta/LESSONS.md`
   - `meta/FUTURES.md`
   - `README.md`
3. Inspect tree quickly with `rg --files` and targeted reads.
4. Restate requested edits in implementation terms.

## Metadata layout
- `meta/` stores agent-facing process docs and memory indexes.
- `meta/memory/` stores indexed lesson/future records.
- `meta/history/` stores prior planning artifacts.

## Change and validation rules
- Keep patches small, reversible, and scoped.
- For `src/` changes: run `make clean && make` and
  `bash tests/smoke.sh`.
- For docs-only changes: run `git diff --check` and line-width
  checks on edited docs.
- If usage changes, update `README.md`.
- If architecture/process changes, update `SCHEMA.md`.

## Invariants
- Canonical outer cycle is CCW; holes are CW.
- Canonicalize before hash insertion.
- Tetrille coordinates are tagged and translated via 6-system lifts.
