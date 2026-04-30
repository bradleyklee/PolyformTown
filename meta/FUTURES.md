This file is a priority-ordered index into `meta/memory/` futures.
Each line references a single future item file. Within ACTIVE, earlier
lines should be pursued first.

Future files are atomic and stable, and detailed notes live in
`meta/memory/Fxxxx.txt`. Reordering this file does not change file index
number or title. A leading check mark (✓) indicates a completed item,
which is moved to COMPLETE. Completed items are not deleted from memory.

---

ACTIVE
F0028 — Full live-boundary lookahead to tile-surround horizon
F0030 — Formalize the tile-surround constellation bound
F0031 — Extract RL1 tile-surround constellations
F0025 — RL0 liveness validation for RL1 pruning
F0026 — Expand smoke tests
F0027 — Detailed print mode for all print tools
F0024 — Quality analysis of existing functionality
F0018 — Define Spectre tile boundary representation
F0019 — Run engine on Spectre tiling

COMPLETE
✓ F0021 — Generalize to vertex figures with non-uniform valencies (tetrille tilings)
✓ F0017 — Run engine on Hat tiling
✓ F0016 — Define Hat tile boundary representation
✓ F0015 — Ensure all geometry remains exact (no floats)
✓ F0014 — Add support for non-square lattice coordinate systems
✓ F0020 — Test generalized geometry against OEIS A000228 and A000577
✓ F0001 — Verify A000105 counts match baseline
✓ F0002 — Verify hole counts at n=7 and n=8
✓ F0003 — Define domino tile boundary with unit edges
✓ F0004 — Include intermediate vertices in tile boundary
✓ F0005 — Enumerate all valid edge matches for domino
✓ F0006 — Integrate domino tile into attachment system
✓ F0007 — Verify internal consistency of domino engine
✓ F0008 — Compare domino counts to OEIS A056785
✓ F0009 — Derive small-n domino counts via tileability if needed
✓ F0010 — Move tile definitions to external files
✓ F0011 — Implement tile file loader
✓ F0012 — Generate symmetry variants for loaded tiles
✓ F0013 — Normalize tiles after reflection
✓ F0022 — Finalize tetrille canonicalization against the n=12 duplicate
✓ F0023 — Trace loss of 6-anchors in offending tetrille print output
✓ F0029 — Manual audit of differential completion examples
