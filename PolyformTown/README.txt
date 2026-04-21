PolyformTown V3.0
=================

Goal
----
Enumerate lattice polyforms and compare the resulting counts against
reference sequences. The current engine supports square-lattice and
triangular-lattice tiles through the same edge-cycle framework.

Schema
------
The code uses a data structure defined in terms of edge cycles. Each
polygon object consists of an outer boundary cycle and zero or more hole
cycles. The outer boundary is oriented counterclockwise (CCW), and hole
cycles are oriented clockwise (CW).

Tile file format
----------------
Tile files live in `tiles/` and use this schema:

    name: <tile name>
    lattice: <square|triangular>
    cycle:
    x y
    x y
    ...

Coordinates are integer lattice coordinates. For triangular tiles the
coordinates are stored in a 2-component triangular basis, so geometry
remains exact and no floating point is used internally.

Procedure
---------
1. Load a tile boundary from a tile file.
2. Precompute the distinct symmetry variants of the tile:
   - square lattice -> D4
   - triangular lattice -> D6
3. For each current polyform, scan the full frontier across all cycles.
4. For each frontier edge and precomputed tile variant edge, align by
   translation so the chosen tile edge is antiparallel and coincident
   with the chosen base edge.
5. Merge by edge cancellation:
   - antiparallel coincident edges cancel
   - parallel coincident edges reject immediately
6. Extract all resulting boundary cycles.
7. Canonicalize the full polyform under translation, rotation, and
   reflection in the active lattice.
8. Test interior cycles to reject overlaps and invalid constructions.
9. Insert into the hash table to avoid duplicates.

Canonicalization
----------------
All cycles are transformed together. The outer cycle is the one with
maximum absolute signed area. Canonical form is `[outer] + sorted(holes)`,
with outer oriented CCW and holes CW, sorted lexicographically. Hashing is
performed on this canonical form so that symmetry-equivalent shapes map to
a single representative and are counted once.

Build
-----
make clean
make

Programs
--------
poly_count N [tilefile]
    Print the count at each level 1..N.

poly_print N [tilefile]
    Print the canonical edge form for each shape at level N.

Examples
--------
Run with defaults (monomino, built-in N):
    ./poly_count
    ./poly_print

Square lattice examples:
    ./poly_count 10 tiles/monomino.tile
    ./poly_print 7 tiles/monomino.tile
    ./poly_count 5 tiles/domino.tile
    ./poly_count 4 tiles/chair.tile
    ./poly_count 3 tiles/tetL.tile

Triangular lattice examples:
    ./poly_count 8 'tiles/triangle.tile'
    ./poly_count 6 'tiles/hexagon.tile'
    ./poly_count 6 'tiles/half hexagon.tile'

Count hole-bearing outputs:
    ./poly_print 7 tiles/monomino.tile | grep '^1 ' | wc -l
    ./poly_print 8 tiles/monomino.tile | grep '^1 ' | wc -l

Print format
------------
Each printed polyform begins with a hole flag:

    <0|1> cycle0 | cycle1 | ...

where: 0 = no holes, 1 = has holes. The outer cycle is printed first;
holes follow after ` | ` delimiters.

Expected reference counts
-------------------------
Square lattice targets:

A000104 (polyominoes without holes):
n:     0  1  2  3  4   5   6    7    8    9    10
count  1  1  1  2  5  12  35  107  363  1248  4460

A000105 (free polyominoes):
n:     1  2  3  4   5   6    7    8    9    10    11
count  1  1  2  5  12  35  108  369  1285  4655  17073

A056785 (polydominoes):
n:     1  2   3    4     5      6
count  1  4  23  211  2227  25824

Triangular lattice targets:

A000577 (polyiamonds):
n:     1  2  3  4  5   6   7   8
count  1  1  1  3  4  12  24  66

A000228 (polyhexes):
n:     1  2  3  4   5   6
count  1  1  3  7  22  82

A057782 (polytriamonds / polytraps):
n:     1  2  3   4    5      6
count  1  9  94  1552 27285 509805

Expected hole counts from print
-------------------------------
Monomino:
    ./poly_print 7 tiles/monomino.tile | grep '^1 ' | wc -l  -> 1
    ./poly_print 8 tiles/monomino.tile | grep '^1 ' | wc -l  -> 6

Domino:
    ./poly_print 4 tiles/domino.tile | grep '^1 ' | wc -l    -> 3

Chair (not proven):
    ./poly_print 3 tiles/chair.tile | grep '^1 ' | wc -l     -> 10

tetL:
    ./poly_print 2 tiles/tetL.tile | grep '^1 ' | wc -l      -> 1

Scope
-----
The system now accepts square-lattice and triangular-lattice tiles in the
same edge-cycle framework. Enumeration remains tractable only for small
primitives, but the architecture is no longer limited to square geometry.

Status
------
V3.0 is a stable milestone with verified square and triangular lattice
support. A natural next goal is to generalize to vertex figures with
non-uniform valencies.
