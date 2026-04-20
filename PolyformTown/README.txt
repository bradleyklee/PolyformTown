PolyformTown V2.2
=================

Goal
----
Enumerate polyforms and compare the resulting counts against
reference sequences, especially OEIS A000104, A000105, and
A056785. Also generate similar sequences beyond the present
scope of OEIS, specifically polychairs and polytetraLs.

Schema
------
The code uses a data structure defined in terms of edge
cycles. Each polygon object consists of an outer boundary
cycle and zero or more hole cycles. The outer boundary is
oriented counterclockwise (CCW), and hole cycles are
oriented clockwise (CW). All edges are unit-length lattice
edges, and edge crossings are assumed to occur only at
vertices, never between vertices.

Procedure
---------
1. Load a tile boundary from a tile file and compute the
   reflected variant if the tile is chiral.
2. For each current polyform, scan the full frontier across
   all cycles.
3. For each frontier edge and tile edge, align the tile so
   the chosen tile edge is antiparallel and coincident with
   the chosen base edge.
4. Merge by edge cancellation:
   - antiparallel coincident edges cancel
   - parallel coincident edges reject immediately
5. Extract all resulting boundary cycles, with traversal
   maximizing hole count (rejecting invalid walks including
   duplicate vertices inside a cycle).
6. Canonicalize the full polyform under translation,
   rotation, and reflection.
7. Test interior cycles to ensure they are holes and not
   intersections.
8. Insert into the hash table to avoid duplicates.

Canonicalization
----------------
All cycles are transformed together. The outer cycle is the
one with maximum absolute signed area. Canonical form is
[outer] + sorted(holes), with outer oriented CCW and holes
CW, sorted lexicographically. Hashing is performed on this
canonical form so that chiral variants map to a single
representative and are counted once.

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

Explicit monomino runs:
    ./poly_count 10 tiles/monomino.tile
    ./poly_print 7 tiles/monomino.tile

Other tile sets (smaller N recommended):
    ./poly_count 5 tiles/domino.tile
    ./poly_count 4 tiles/chair.tile
    ./poly_count 3 tiles/tetL.tile

Count hole-bearing outputs:
    ./poly_print 7 tiles/monomino.tile | grep '^1 ' | wc -l
    ./poly_print 8 tiles/monomino.tile | grep '^1 ' | wc -l

Print format
------------
Each printed polyform begins with a hole flag:

    <0|1> cycle0 | cycle1 | ...

where: 0 = no holes, 1 = has holes. The outer cycle is
printed first; holes follow after ` | ` delimiters.

Expected reference counts
-------------------------
Targets include OEIS A000104, A000105, and A056785.

A000104 (number of polyominoes without holes):
n:     0  1  2  3  4   5   6    7    8    9    10
count  1  1  1  2  5  12  35  107  363  1248  4460

A000105 (number of polyominoes):
n:     1  2  3  4   5   6    7    8    9    10    11
count  1  1  2  5  12  35  108  369  1285  4655  17073

A056785 (number of polydominoes):
n:     1  2   3    4     5      6
count  1  4  23  211  2227  25824

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
The system is designed to accept any simple polyomino tile
as input and operate on it within the same edge-cycle
framework. In practice, enumeration remains tractable for
small primitives such as monominoes, dominoes, trominoes,
and tetrominoes, but quickly becomes impractical beyond
that scale due to combinatorial growth.

Status
------
V2.2 is a stable milestone with multiple tile primitives
and verified enumeration behavior. A natural next goal is
to extend the approach to polyiamonds and polyhexes.
