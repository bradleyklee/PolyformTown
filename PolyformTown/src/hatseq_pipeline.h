#ifndef HATSEQ_PIPELINE_H
#define HATSEQ_PIPELINE_H

#include "vcomp_pipeline.h"

void run_hatseq_levels(const Tile *tile,
                       int max_n,
                       int selector_mask,
                       int double_check_mode,
                       int strict_three_mode,
                       VCompLevelFn on_level,
                       void *userdata);

#endif
