#include "qop.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <stdio.h>
#include <assert.h>

// PCG32 RNG - fast and deterministic
typedef struct
{
    uint64_t state;
    uint64_t inc;
} pcg32_t;

static uint32_t pcg32_random(pcg32_t *rng)
{
    uint64_t oldstate = rng->state;
    rng->state = oldstate * 6364136223846793005ULL + (rng->inc | 1);
    uint32_t xorshifted = ((oldstate >> 18u) ^ oldstate) >> 27u;
    uint32_t rot = oldstate >> 59u;
    return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
}

static void pcg32_init(pcg32_t *rng, uint64_t seed)
{
    rng->state = seed;
    rng->inc = (seed << 1) | 1;
    pcg32_random(rng);
}

// MCTS Node for query optimization
typedef struct QOPNode
{
    uint32_t pattern_id;