#ifndef VCOMP_H
#define VCOMP_H

#include "cycle.h"
#include "tile.h"

typedef void (*VCompEmitFn)(const Poly *p,
                            const Coord *hidden,
                            int hidden_count,
                            void *userdata);
typedef void (*VCompTraceEmitFn)(const Poly *p,
                                 const Coord *hidden,
                                 int hidden_count,
                                 const Cycle *added_tiles,
                                 int added_tile_count,
                                 void *userdata);

int build_boundary_vertices(const Poly *p, Coord *verts);
void enumerate_vertex_completions(const Poly *base,
                                  const Tile *tile,
                                  Coord target,
                                  const Coord *initial_hidden,
                                  int initial_hidden_count,
                                  VCompEmitFn emit,
                                  void *userdata);

void enumerate_vertex_completions_mode(const Poly *base,
                                       const Tile *tile,
                                       Coord target,
                                       const Coord *initial_hidden,
                                       int initial_hidden_count,
                                       int rotations_only,
                                       VCompEmitFn emit,
                                       void *userdata);

void enumerate_vertex_completions_steps(const Poly *base,
                                        const Tile *tile,
                                        Coord target,
                                        const Coord *initial_hidden,
                                        int initial_hidden_count,
                                        int rotations_only,
                                        int required_steps,
                                        VCompEmitFn emit,
                                        void *userdata);

void enumerate_vertex_completions_steps_trace(const Poly *base,
                                              const Tile *tile,
                                              Coord target,
                                              const Coord *initial_hidden,
                                              int initial_hidden_count,
                                              int rotations_only,
                                              int required_steps,
                                              VCompTraceEmitFn emit,
                                              void *userdata);

int has_vertex_completion(const Poly *base,
                          const Tile *tile,
                          Coord target,
                          const Coord *initial_hidden,
                          int initial_hidden_count,
                          int rotations_only);

int has_vertex_completion_steps(const Poly *base,
                                const Tile *tile,
                                Coord target,
                                const Coord *initial_hidden,
                                int initial_hidden_count,
                                int rotations_only,
                                int required_steps);
#endif
