# AGENTS.md

Scope: this file applies to the entire repository tree rooted at
`/workspace/PolyformTown`.

## Purpose
This is the top-level Codex harness for reliable work on
PolyformTown. It complements (not replaces) `RESET.txt`,
`SUSPEND.md`, `LESSONS.md`, `FUTURES.md`, and `README.md`.

Use this file to keep future edits consistent, testable, and easy
to review.

## Codex-first operating model (not chat-style)
- Prefer concrete file edits over broad prose plans.
- Prefer deterministic shell commands over speculative reasoning.
- Always cite files/lines in final summaries when asked.
- Keep patches small, reversible, and scoped to the request.
- Do not invent behavior; verify against repository data.

## STARTUP pipeline (run at the start of each task)
1. Read `AGENTS.md` (this file) fully.
2. Enter project dir: `cd PolyformTown`.
3. Read, in order:
   - `RESET.txt`
   - `PERSONAE.md`
   - `LESSONS.md`
   - `FUTURES.md`
   - `README.md`
4. Inspect current tree quickly (`rg --files`, targeted reads).
5. Validate baseline when task risk is medium/high:
   - `make clean && make`
   - `bash tests/smoke.sh`
6. Restate task in implementation terms before editing.

## File-format and writing rules
- Use fixed-column-friendly prose (target <= 72 chars/line).
- Keep text tight; avoid long, wrapped paragraphs.
- Preserve existing naming and style in touched files.
- Keep comments high-signal and implementation-specific.

## Engineering rules for this codebase
- Core invariants are mandatory:
  - canonical outer cycle is CCW; holes are CW.
  - lattice symmetry handling is lattice-specific.
  - tetrille coordinates are tagged; translation is lifted
    through 6-system formulas.
- Never bypass canonicalization before hash insertion.
- Prefer integer-safe transforms; avoid introducing float-based
  geometry into core enumeration logic.

## Change sizing and safety
- Prefer one logical change per commit.
- Avoid broad refactors unless requested.
- If behavior changes, update docs and tests in same patch.
- If usage changes, update `README.md`.
- If new durable lessons arise, update `LESSONS.md` and/or
  `FUTURES.md` entries as appropriate.

## Validation matrix (minimum expectations)
- Docs-only change:
  - run `git diff --check`
  - run line-width check for edited docs.
- Code change in `src/`:
  - `make clean && make`
  - `bash tests/smoke.sh`
- New executable behavior:
  - include a reproducible command example in final notes.

## Preferred command snippets
- Build:
  - `cd PolyformTown && make clean && make`
- Smoke test:
  - `cd PolyformTown && bash tests/smoke.sh`
- Line-width check (docs):
  - `awk 'length($0)>72 {print NR ":" length($0) " " $0}' <file>`

## PR/response checklist
Before finishing:
1. `git status --short` is clean except intended files.
2. Tests/checks run and outcomes are recorded.
3. Commit message is specific and scoped.
4. Final response includes:
   - what changed,
   - why,
   - exact validation commands and results,
   - file citations when required.

## Priority model when instructions conflict
1. System/developer/user instructions (prompt-level).
2. `AGENTS.md` files by scope (nearest file wins).
3. Repository memory/process docs (`RESET`, `SUSPEND`,
   `LESSONS`, `FUTURES`).

This ordering keeps Codex behavior predictable.
