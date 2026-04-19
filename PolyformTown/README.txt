Monomino edge-geometry prototype

Goal
----
Recreate free polyomino enumeration using ordered boundary cycles instead of cell occupancy.
This version is intentionally minimal and targeted only at the monomino case, but the
file layout is chosen so the next iteration can swap in domino attachment logic.

What is stored
--------------
Each shape is stored as one ordered outer cycle. Holes are not supported in this first
version, but the attachment logic is written as a merge on boundary edges rather than a
cell fill, so the code stays close to the eventual domino / multi-cycle design.

How growth works
----------------
1. Load the tile boundary from a tile file.
2. For each current shape, scan its boundary edges as the frontier.
3. For each frontier edge and each tile edge, align the tile so the chosen tile edge is
   antiparallel and coincident with the chosen base edge.
4. Merge the two cycles by edge cancellation:
   - antiparallel coincident edges cancel
   - parallel coincident edges reject immediately
5. Walk the remaining directed edges as a single cycle.
   - if the walk branches or leaves a second cycle behind, reject
6. Canonicalize by translation, rotation, and reflection.
   After a reflection, cycle direction is repaired so the outer boundary remains CCW.
7. Insert into a hash table to avoid duplicates.

Programs
--------
mono_count N [tilefile]
    Print the count at each level 1..N.

mono_dump N [tilefile]
    Print the canonical edge form for each shape at level N.

Build
-----
make clean
make

Examples
--------
./mono_count 6
./mono_dump 5

Current limitations
-------------------
- one outer cycle only
- no holes yet
- no generic vertex-level intersection machinery yet
- no domino logic yet

Why this version exists
-----------------------
This package is meant to replace the earlier ad hoc monomino code with a more rigorous,
cycle-based edge geometry implementation that is still small enough to inspect and edit.
The next minor iteration should keep the same overall structure and replace only the tile
attachment / validation logic needed for dominoes.
