#ifndef NUMERICS_H
#define NUMERICS_H

#include "core/attach.h"

/* geometry */

int point_in_cycle(double px, double py,
                       const Cycle *c, int lattice);

int cycle_interior_point(const Cycle *c, int lattice,
                             double *px, double *py);

#endif