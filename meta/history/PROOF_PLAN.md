# PolyformTown “Booting the Tiling” Engine
## Design Document v0.2

---

## 1. Purpose

The goal of this system is to construct a tiling engine that does not
rely on brute-force enumeration alone, but instead learns from valid
configurations and uses that knowledge to guide future search.

Rather than repeatedly rediscovering the same structural facts about a
tiling system, the engine incrementally builds a body of knowledge in
the form of rules and lookup tables. These rules allow the engine to
prune invalid configurations early, force deterministic growth where
possible, and reduce the overall search space dramatically.

Conceptually, the system behaves like a machine that must “boot”
itself. At the lowest level, it has only raw geometric rules. As it
runs, it learns increasingly powerful constraints, eventually reaching
a stage where it can perform highly efficient inference over tilings.

---

## 2. Fundamental Representation

A key shift in this system is that the primary object of study is not
just a boundary, but a pair:

    State = (Tileset, Boundary)

The tileset is the authoritative object. It represents an actual
realized configuration of tiles in the plane. The boundary is derived
from the tileset and is used to guide further growth.

This distinction is critical. The system learns from tilesets, not from
boundaries. Boundaries are incomplete information, while tilesets
represent fully valid configurations. All inference rules ultimately
originate from valid tilesets.

---

## 3. Vertex Figures and Canonical Representation

Each vertex in a tiling can be described by a cyclic arrangement of
incident edges. These are not arbitrary lists; they are geometrically
ordered and must be preserved in cyclic order (typically CCW).

To ensure consistency across the system, vertices and edges should be
numbered according to their identity in the tile definition file. This
provides a stable indexing scheme that survives transformations.

A vertex figure is therefore represented as:

    (principal_vertex_id, cyclic sequence of edge-pairs)

Canonicalization is essential. Because the data is cyclic, we cannot
sort it arbitrarily without destroying geometric meaning. Instead,
canonical form is defined as the lexicographically minimal cyclic
rotation of the sequence.

This ensures:
- identical geometric configurations map to identical representations
- no duplicates arise from rotation
- geometric structure is preserved

All stored data must be written in this canonical cyclic form.

---

## 4. Partial Completions and Forgetful Mapping

The central mechanism for learning is the forgetful map.

Given a valid tileset, we examine local configurations (especially
vertex figures) and deliberately remove information. Specifically, we
remove one or more consecutive tiles in the cyclic order around a
vertex, producing a partial configuration.

This process is constrained:

- tiles must be removed consecutively in cyclic order
- at least two spanning edges must remain
- the structure must remain geometrically meaningful

The resulting object is a partial completion. Because it comes from a
valid configuration, we already know at least one way it can be
completed.

We then record a mapping:

    partial → completion

Both sides are stored in canonical form.

Multiple full configurations may map to the same partial configuration.
These preimage collisions are expected and are not an error. The lookup
table therefore maps a single partial to a set of possible completions.

Over time, this table becomes a powerful inference mechanism.

---

## 5. Lookup Tables and Inference

The lookup table is the core knowledge structure of the engine.

Each entry associates a canonical partial configuration with a set of
canonical completions that have been observed in valid tilesets.

During growth, the engine examines a boundary vertex, reconstructs the
local partial configuration from incident tiles, and queries the lookup
table for possible completions.

These completions are not guaranteed to be valid in the current global
context. They must be checked against the boundary for compatibility.
However, the lookup table dramatically reduces the search space by
restricting attention to completions that are known to work somewhere.

A particularly important case occurs when a partial configuration maps
to exactly one valid completion. In this case, the completion is forced.
Forced completions can be applied greedily depending on boundary and 
the runlevel. There will always be a fixed point at which the greedy 
growth is exhausted and multiple growth options are found across the
entire boundary. 

---

## 6. Dead Boundary Detection

A boundary is classified as either living or dead according to 
whether it will allow any more growth.

A boundary is living if every vertex admits at least one valid
completion or dead if there exists at least one vertex for which no
valid completion exists.

To test this, the engine scans each boundary vertex and attempts to find
at least one valid completion. It is not necessary to enumerate all
completions. Finding a single valid completion is sufficient to mark
the vertex as viable.

If any vertex has no valid completion, the boundary is dead; otherwise, 
the boundary is living. In the context of a search, dead boundaries are 
pruned and live boundaries are continued. 

Most importantly, boundary analysis can depend runlevel, i.e. the state 
of accumulated knowledge about inadmissible configurations (also called
false positives). 

---

## 7. Growth and Boundary-Driven Search

The growth process happens on the boundary of the current tileset.

The boundary provides a set of vertices where new tiles may be attached.
At each boundary vertex, the engine constructs a partial configuration
from the incident tiles and uses the lookup table to generate candidate
completions.

Each candidate completion must be validated against the current
boundary. This includes checking edge compatibility, valence
constraints, and ensuring no overlaps occur.

The engine iterates over boundary vertices, generating candidates and
either applying forced completions or branching when multiple choices
exist.

Search is primarily conducted in detail using depth-first search (DFS).
Search continues along a boundary until either a new waypoint boundary 
is formed or a contradiction is reached. We also may use breadth first
search when waypoint boundaries branch in totaly distinct directions, 
yet we need to prove all of them ultimately die off. 

---


## 8. Waypoint Completion

The primary search goal is usually elimination of a false positive.
Depening how the DFS is coded, this may happen all at once. Even 
then, waypoint completion can be an important intermediary goal, 
and especially helpful when tracking search histories.  

Given a boundary, the objective is to grow the tiling such that all
current boundary vertices become interior points of a new boundary 
(or set of boundaries). Waypoint completion is not a technical term, 
so we also call this particular type of waypoint completion a ring 
completion. 

Ring completion requires selecting vertex completions at each 
boundary vertex, maintaining consistency all around the original 
boundary, and then closing the cycle on one final addition. This 
is usually a "DFS within BFS" where new living boundaries are 
obtained. 

The lookup table provides not only candidate completions but also the
number of candidates at each vertex. This information is useful for
applying heuristics to sort order and choice of starting point. Hint:
minimize branching as much as possible!

Each successful ring completion produces a new boundary and is recorded
as part of the search tree relating all completions. 

---

## 9. Search Tree Representation

The search process naturally forms a graph structure, but we are
typically only concerned with its coarse grained appearence. 

Nodes represent waypoint completions (tileset + boundary), and edges 
typically correspond to difficult searches between waypoints. 

Storing this structure allows for later analysis, debugging, and
potential reuse of discovered configurations.

---

## 10. Runlevel System

The engine operates in stages called runlevels. Each runlevel represents
a refinement of the system’s knowledge.

A runlevel consists of:
1. a dataset (rules, lookup tables, configurations)
2. a method for using that dataset to generate more data

Until the system is capable of tiling with ease, the purpose of each 
successive runlevel is to get closer and closer to that goal. This is 
done by transforming data and methods between levels. 

Runlevels are hierarchical. Higher runlevels include and build upon the
data of lower runlevels.

The initial runlevels are defined as:

    0: tetrille inset
    1: vertex surrounds
    2: tile surrounds 
    3: super surrounds
    4: combinatorial hexagons
    5: fractal singularity

Lower runlevels may operate with minimal data and rely on direct search.
Higher runlevels incorporate learned rules and may use entirely
different representations. At level four, hat tilings can be built 
using a SAT solver on a hexagonal lattice. At level five, replacement 
rules operate on the hats to rapidly produce all valid tilings. 

---

## 12. Filesystem Model

The data of each runlevel is persisted to disk to allow reuse 
and reproducibility.

A typical structure is:

    /runlevel_X/
        dataset/
        lookup_tables/
        rules/
        metadata/

Runlevels can be loaded directly or recomputed from lower levels to
verify correctness. In practice, the system will usually progress
forward, but the ability to recompute ensures rigor.

---

## 13. Heuristics

Heuristics are essential for managing search complexity.

Key heuristics include:

- apply forced completions immediately
- start where the branching ratio is smallest 
- start where the neighborhood imposes strong constraints
- continue to follow strong constraints consecutively  

These heuristics do not change correctness but may have a 
significant impact on performance.

---

## 14. Canonicalization at the Boundary Level

Canonicalization of boundaries follows the standard approach of
generating symmetry variants and selecting a lexicographically minimal
representation.

While this process may not be optimal, it is sufficient provided that
each boundary has a unique valid completion. Under this assumption,
canonicalizing the boundary is enough to ensure uniqueness of states.

---

## 15. Guiding Principle

The system is governed by a simple principle:

    learn from valid configurations,
    compress that knowledge into rules,
    and reuse those rules to avoid recomputation
