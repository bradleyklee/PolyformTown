PolyformTown V2.1.1
===================

Goal
----
Enumerate free polyominoes with an edge-based geometry engine that keeps full boundary
cycle structure, including holes. This version targets OEIS A000105.

What is stored
--------------
Each shape is stored as a Poly:
- one outer boundary cycle
- zero or more hole cycles

All edges are unit-length lattice edges. No floating point arithmetic is used.

How growth works
----------------
1. Load a tile boundary from a tile file.
2. For each current polyform, scan the full frontier across all cycles.
3. For each frontier edge and tile edge, align the tile so the chosen tile edge is
   antiparallel and coincident with the chosen base edge.
4. Merge by edge cancellation:
   - antiparallel coincident edges cancel
   - parallel coincident edges reject immediately
5. Extract all resulting boundary cycles.
6. Reject invalid walks (including duplicate vertices inside a cycle).
7. Canonicalize the full polyform under translation, rotation, and reflection.
8. Insert into the hash table to avoid duplicates.

Canonicalization rules
----------------------
- All cycles are transformed together as one rigid object.
- The outer cycle is the cycle with maximum absolute signed area.
- Canonical form is:

    [outer] + sorted(holes)

- Outer orientation is CCW.
- Hole orientation is CW.
- Holes are sorted lexicographically after the outer cycle is fixed.

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
./mono_count 10
./mono_dump 7
./mono_dump 8 | grep '^1 '
./mono_dump 8 | grep '^1 ' | wc -l

Dump format
-----------
Each dumped polyform begins with a boolean hole flag:

    <0|1> cycle0 | cycle1 | ...

where:
- 0 = no holes
- 1 = has holes

The outer cycle is printed first. Holes follow after ` | ` delimiters.

Expected reference counts
-------------------------
This version targets OEIS A000105:

n:     1  2  3  4   5   6    7    8    9    10    11
count  1  1  2  5  12  35  108  369  1285  4655  17073

Expected hole counts from dump
------------------------------
./mono_dump 7 | grep '^1 ' | wc -l   -> 1
./mono_dump 8 | grep '^1 ' | wc -l   -> 6

Current scope
-------------
- monomino tile input
- multi-cycle polyominoes with holes
- exact integer canonicalization
- stable dump format for scripting and verification

Next step
---------
V2.2 should extend the same engine to domino tiles without reverting to any cell-based
representation.
