#ifndef PRNG_H
#define PRNG_H

#include <stdint.h>

typedef struct {
    uint32_t state;
} Prng;

void prng_seed(Prng *p, uint32_t seed);
float prng_float(Prng *p);
int prng_range(Prng *p, int min, int max);

#endif
