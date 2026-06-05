#include "prng.h"

void prng_seed(Prng *p, uint32_t seed)
{
    p->state = seed ? seed : 1;
}

float prng_float(Prng *p)
{
    p->state = p->state * 1664525u + 1013904223u;
    return (float)(p->state >> 1) / (float)0x7FFFFFFFu;
}

int prng_range(Prng *p, int min, int max)
{
    float f = prng_float(p);
    return min + (int)(f * (max - min + 1));
}
