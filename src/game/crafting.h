#ifndef CRAFTING_H
#define CRAFTING_H

#include <stdint.h>

typedef struct {
    uint16_t seed_a;
    uint16_t seed_b;
    uint16_t result;
} Recipe;

int crafting_splice(uint16_t seed_a, uint16_t seed_b, uint16_t *result);
int crafting_get_recipes(Recipe **out_recipes, int *out_count);

#endif
