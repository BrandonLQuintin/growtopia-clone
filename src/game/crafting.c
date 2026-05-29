#include "crafting.h"
#include "../world/world.h"

static Recipe recipes[] = {
    { SEED_DIRT, SEED_GRASS, SEED_WOOD },
    { SEED_DIRT, SEED_STONE, SEED_BRICK },
    { SEED_SAND, SEED_SAND, SEED_CACTUS },
    { SEED_GRASS, SEED_GRASS, SEED_FLOWER },
    { SEED_DIRT, SEED_MUD, SEED_MUSHROOM },
    { SEED_GRASS, SEED_SAND, SEED_BUSH },
    { SEED_STONE, SEED_LIMESTONE, SEED_ROCK },
    { SEED_SAND, SEED_MUD, SEED_CLAY },
    { SEED_STONE, SEED_SAND, SEED_LIMESTONE },
    { SEED_WOOD, SEED_WOOD, SEED_LEAVES },
    { SEED_ICE, SEED_DIRT, SEED_SNOW },
    { SEED_GRASS, SEED_WOOD, SEED_LEAVES },
    { SEED_DIRT, SEED_SAND, SEED_MUD },
    { SEED_CLAY, SEED_SAND, SEED_BRICK },
    { SEED_DIRT, SEED_CLAY, SEED_MUD },
    { SEED_ICE, SEED_GRASS, SEED_SNOW },
    { SEED_STONE, SEED_STONE, SEED_LIMESTONE },
};

#define RECIPE_COUNT (sizeof(recipes) / sizeof(recipes[0]))

int crafting_splice(uint16_t seed_a, uint16_t seed_b, uint16_t *result)
{
    for (int i = 0; i < (int)RECIPE_COUNT; i++) {
        if ((recipes[i].seed_a == seed_a && recipes[i].seed_b == seed_b) ||
            (recipes[i].seed_a == seed_b && recipes[i].seed_b == seed_a)) {
            *result = recipes[i].result;
            return 0;
        }
    }
    return -1;
}

int crafting_get_recipes(Recipe **out_recipes, int *out_count)
{
    *out_recipes = recipes;
    *out_count = (int)RECIPE_COUNT;
    return 0;
}
