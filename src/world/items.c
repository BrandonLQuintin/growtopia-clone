#include "items.h"
#include "world.h"
#include <stddef.h>

static const ItemDef block_defs[BLOCK_FOREGROUND_MAX] = {
    [BLOCK_DIRT]      = {BLOCK_DIRT,      "Dirt",            ITEM_CAT_BLOCK, 200,  5,  2, BLOCK_DIRT,      139,  90,  43, 0, 0, 0, 0, 0, 0},
    [BLOCK_STONE]     = {BLOCK_STONE,     "Stone",           ITEM_CAT_BLOCK, 200, 10,  5, BLOCK_STONE,     128, 128, 128, 0, 0, 0, 0, 0, 0},
    [BLOCK_GRASS]     = {BLOCK_GRASS,     "Grass",           ITEM_CAT_BLOCK, 200,  5,  2, BLOCK_GRASS,      76, 153,   0, 0, 0, 0, 0, 0, 0},
    [BLOCK_WOOD]      = {BLOCK_WOOD,      "Wood",            ITEM_CAT_BLOCK, 200, 10,  5, BLOCK_WOOD,      160,  82,  45, 0, 0, 0, 0, 0, 0},
    [BLOCK_WOOD_BG]   = {BLOCK_WOOD_BG,   "Wood Background", ITEM_CAT_BLOCK, 200,  5,  2, BLOCK_WOOD_BG,   139,  69,  19, 0, 0, 0, 0, 0, 0},
    [BLOCK_LEAVES]    = {BLOCK_LEAVES,    "Leaves",          ITEM_CAT_BLOCK, 200,  5,  2, BLOCK_LEAVES,     34, 139,  34, 0, 0, 0, 0, 0, 0},
    [BLOCK_DOOR]      = {BLOCK_DOOR,      "Door",            ITEM_CAT_BLOCK, 200, 15,  7, BLOCK_DOOR,      101,  67,  33, 0, 0, 0, 0, 0, 0},
    [BLOCK_BRICK]     = {BLOCK_BRICK,     "Brick",           ITEM_CAT_BLOCK, 200, 15,  7, BLOCK_BRICK,     178,  34,  34, 0, 0, 0, 0, 0, 0},
    [BLOCK_GLASS]     = {BLOCK_GLASS,     "Glass",           ITEM_CAT_BLOCK, 200, 20, 10, BLOCK_GLASS,     173, 216, 230, 0, 0, 0, 0, 0, 0},
    [BLOCK_SAND]      = {BLOCK_SAND,      "Sand",            ITEM_CAT_BLOCK, 200,  5,  2, BLOCK_SAND,      237, 201, 175, 0, 0, 0, 0, 0, 0},
    [BLOCK_LAVA]      = {BLOCK_LAVA,      "Lava",            ITEM_CAT_BLOCK, 200, 25, 12, BLOCK_LAVA,      220,  80,  20, 0, 0, 0, 0, 0, 0},
    [BLOCK_ROCK]      = {BLOCK_ROCK,      "Rock",            ITEM_CAT_BLOCK, 200, 10,  5, BLOCK_ROCK,      105, 105, 105, 0, 0, 0, 0, 0, 0},
    [BLOCK_LIMESTONE] = {BLOCK_LIMESTONE, "Limestone",       ITEM_CAT_BLOCK, 200, 15,  7, BLOCK_LIMESTONE, 211, 211, 211, 0, 0, 0, 0, 0, 0},
    [BLOCK_MUD]       = {BLOCK_MUD,       "Mud",             ITEM_CAT_BLOCK, 200,  5,  2, BLOCK_MUD,       101,  67,  33, 0, 0, 0, 0, 0, 0},
    [BLOCK_CLAY]      = {BLOCK_CLAY,      "Clay",            ITEM_CAT_BLOCK, 200, 10,  5, BLOCK_CLAY,      180, 120,  80, 0, 0, 0, 0, 0, 0},
    [BLOCK_GRAVEL]    = {BLOCK_GRAVEL,    "Gravel",          ITEM_CAT_BLOCK, 200,  5,  2, BLOCK_GRAVEL,    144, 144, 144, 0, 0, 0, 0, 0, 0},
    [BLOCK_ICE]       = {BLOCK_ICE,       "Ice",             ITEM_CAT_BLOCK, 200, 15,  7, BLOCK_ICE,       200, 230, 255, 0, 0, 0, 0, 0, 0},
    [BLOCK_SNOW]      = {BLOCK_SNOW,      "Snow",            ITEM_CAT_BLOCK, 200, 10,  5, BLOCK_SNOW,      240, 240, 255, 0, 0, 0, 0, 0, 0},
    [BLOCK_SIGN]      = {BLOCK_SIGN,      "Sign",            ITEM_CAT_BLOCK, 200, 15,  7, BLOCK_SIGN,      160,  82,  45, 0, 0, 0, 0, 0, 0},
    [BLOCK_LOCK]      = {BLOCK_LOCK,      "Lock",            ITEM_CAT_BLOCK, 200, 50, 25, BLOCK_LOCK,      192, 192, 192, 0, 0, 0, 0, 0, 0},
    [BLOCK_STORE]     = {BLOCK_STORE,     "Store",           ITEM_CAT_BLOCK, 200, 30, 15, BLOCK_STORE,     139,  90,  43, 0, 0, 0, 0, 0, 0},
    [BLOCK_MAILBOX]   = {BLOCK_MAILBOX,   "Mailbox",         ITEM_CAT_BLOCK, 200, 20, 10, BLOCK_MAILBOX,   101,  67,  33, 0, 0, 0, 0, 0, 0},
    [BLOCK_PORTAL]    = {BLOCK_PORTAL,    "Portal",          ITEM_CAT_BLOCK, 200, 50, 25, BLOCK_PORTAL,    128,   0, 128, 0, 0, 0, 0, 0, 0},
};

static const ItemDef seed_defs[SEED_MAX - SEED_OFFSET] = {
    [SEED_DIRT      - SEED_OFFSET] = {SEED_DIRT,      "Dirt Seed",      ITEM_CAT_SEED, 200,  10,  5, SEED_DIRT,      80, 180, 80, 1, BLOCK_DIRT,      30000,  1, 1, 0},
    [SEED_STONE     - SEED_OFFSET] = {SEED_STONE,     "Stone Seed",     ITEM_CAT_SEED, 200,  15,  7, SEED_STONE,     80, 180, 80, 1, BLOCK_STONE,     60000,  1, 0, 0},
    [SEED_GRASS     - SEED_OFFSET] = {SEED_GRASS,     "Grass Seed",     ITEM_CAT_SEED, 200,  10,  5, SEED_GRASS,     80, 180, 80, 1, BLOCK_GRASS,     30000,  2, 1, 0},
    [SEED_WOOD      - SEED_OFFSET] = {SEED_WOOD,      "Wood Seed",      ITEM_CAT_SEED, 200,  15,  7, SEED_WOOD,      80, 180, 80, 1, BLOCK_WOOD,      60000,  2, 1, 0},
    [SEED_LEAVES    - SEED_OFFSET] = {SEED_LEAVES,    "Leaves Seed",    ITEM_CAT_SEED, 200,  15,  7, SEED_LEAVES,    80, 180, 80, 1, BLOCK_LEAVES,    45000,  2, 2, 0},
    [SEED_BRICK     - SEED_OFFSET] = {SEED_BRICK,     "Brick Seed",     ITEM_CAT_SEED, 200,  25, 12, SEED_BRICK,     80, 180, 80, 1, BLOCK_BRICK,    120000,  1, 0, 0},
    [SEED_SAND      - SEED_OFFSET] = {SEED_SAND,      "Sand Seed",      ITEM_CAT_SEED, 200,  10,  5, SEED_SAND,      80, 180, 80, 1, BLOCK_SAND,      30000,  2, 1, 0},
    [SEED_ROCK      - SEED_OFFSET] = {SEED_ROCK,      "Rock Seed",      ITEM_CAT_SEED, 200,  20, 10, SEED_ROCK,      80, 180, 80, 1, BLOCK_ROCK,      90000,  1, 0, 0},
    [SEED_LIMESTONE - SEED_OFFSET] = {SEED_LIMESTONE, "Limestone Seed", ITEM_CAT_SEED, 200,  25, 12, SEED_LIMESTONE, 80, 180, 80, 1, BLOCK_LIMESTONE,120000,  1, 0, 0},
    [SEED_MUD       - SEED_OFFSET] = {SEED_MUD,       "Mud Seed",       ITEM_CAT_SEED, 200,  10,  5, SEED_MUD,       80, 180, 80, 1, BLOCK_MUD,       30000,  1, 1, 0},
    [SEED_CLAY      - SEED_OFFSET] = {SEED_CLAY,      "Clay Seed",      ITEM_CAT_SEED, 200,  15,  7, SEED_CLAY,      80, 180, 80, 1, BLOCK_CLAY,      60000,  1, 1, 0},
    [SEED_ICE       - SEED_OFFSET] = {SEED_ICE,       "Ice Seed",       ITEM_CAT_SEED, 200,  20, 10, SEED_ICE,       80, 180, 80, 1, BLOCK_ICE,       90000,  1, 0, 0},
    [SEED_SNOW      - SEED_OFFSET] = {SEED_SNOW,      "Snow Seed",      ITEM_CAT_SEED, 200,  15,  7, SEED_SNOW,      80, 180, 80, 1, BLOCK_SNOW,      60000,  1, 0, 0},
    [SEED_CACTUS    - SEED_OFFSET] = {SEED_CACTUS,    "Cactus Seed",    ITEM_CAT_SEED, 200,  50, 25, SEED_CACTUS,    80, 180, 80, 1, 28,            180000,  3, 1, 0},
    [SEED_FLOWER    - SEED_OFFSET] = {SEED_FLOWER,    "Flower Seed",    ITEM_CAT_SEED, 200,  40, 20, SEED_FLOWER,    80, 180, 80, 1, 29,            120000,  2, 2, 0},
    [SEED_MUSHROOM  - SEED_OFFSET] = {SEED_MUSHROOM,  "Mushroom Seed",  ITEM_CAT_SEED, 200,  35, 17, SEED_MUSHROOM,  80, 180, 80, 1, 30,             90000,  4, 2, 0},
    [SEED_BUSH      - SEED_OFFSET] = {SEED_BUSH,      "Bush Seed",      ITEM_CAT_SEED, 200,  30, 15, SEED_BUSH,      80, 180, 80, 1, 31,             60000,  3, 1, 0},
};

#define MISC_ITEM_COUNT 8

const ItemDef ITEM_DEFS[MISC_ITEM_COUNT] = {
    {0,    "Fist",    ITEM_CAT_TOOL,     1,    0,  0,    0, 255, 200, 150, 0, 0, 0, 0, 0, 1},
    {9000, "Wrench",  ITEM_CAT_TOOL,     1,   50, 25, 9000, 150, 150, 150, 0, 0, 0, 0, 0, 2},
    {9001, "Pickaxe", ITEM_CAT_TOOL,     1,  100, 50, 9001, 100, 100, 100, 0, 0, 0, 0, 0, 5},
    {9100, "Hat",     ITEM_CAT_CLOTHING, 1,   25, 12, 9100,  50,  50, 200, 0, 0, 0, 0, 0, 0},
    {9101, "Shirt",   ITEM_CAT_CLOTHING, 1,   30, 15, 9101, 200,  50,  50, 0, 0, 0, 0, 0, 0},
    {9102, "Pants",   ITEM_CAT_CLOTHING, 1,   30, 15, 9102,  50,  50, 150, 0, 0, 0, 0, 0, 0},
    {9999, "Gems",    ITEM_CAT_CURRENCY, 9999,  1,  0, 9999,   0, 200, 255, 0, 0, 0, 0, 0, 0},
    {ITEM_BOMB, "Bomb", ITEM_CAT_CONSUMABLE, 50, 50, 25, 0, 40, 40, 40, 0, 0, 0, 0, 0, 0},
};

const int ITEM_DEF_COUNT = MISC_ITEM_COUNT;

const ItemDef *item_get_def(uint16_t item_id) {
    for (int i = 0; i < MISC_ITEM_COUNT; i++) {
        if (ITEM_DEFS[i].id == item_id) {
            return &ITEM_DEFS[i];
        }
    }
    if (item_id > 0 && item_id < BLOCK_FOREGROUND_MAX) {
        if (block_defs[item_id].id == item_id) {
            return &block_defs[item_id];
        }
    }
    if (item_id >= SEED_OFFSET && item_id < SEED_MAX) {
        int idx = item_id - SEED_OFFSET;
        if (seed_defs[idx].id == item_id) {
            return &seed_defs[idx];
        }
    }
    return NULL;
}

const char *item_get_name(uint16_t item_id) {
    const ItemDef *def = item_get_def(item_id);
    return def ? def->name : NULL;
}

int item_get_max_stack(uint16_t item_id) {
    const ItemDef *def = item_get_def(item_id);
    return def ? def->max_stack : 0;
}

int item_is_seed(uint16_t item_id) {
    const ItemDef *def = item_get_def(item_id);
    return def ? def->is_seed : 0;
}

int item_is_block(uint16_t item_id) {
    const ItemDef *def = item_get_def(item_id);
    return def ? def->category == ITEM_CAT_BLOCK : 0;
}

uint16_t item_get_seed_product(uint16_t seed_id) {
    const ItemDef *def = item_get_def(seed_id);
    return def ? def->seed_grows_into : 0;
}

int item_get_grow_time(uint16_t seed_id) {
    const ItemDef *def = item_get_def(seed_id);
    return def ? def->grow_time_ms : 0;
}

int item_get_harvest_count(uint16_t seed_id) {
    const ItemDef *def = item_get_def(seed_id);
    return def ? def->harvest_count : 0;
}

int item_get_harvest_seed_count(uint16_t seed_id) {
    const ItemDef *def = item_get_def(seed_id);
    return def ? def->harvest_seed_count : 0;
}

int item_get_gem_cost(uint16_t item_id) {
    const ItemDef *def = item_get_def(item_id);
    return def ? def->gem_cost : 0;
}

int item_get_sell_cost(uint16_t item_id) {
    const ItemDef *def = item_get_def(item_id);
    return def ? def->sell_cost : 0;
}

void item_get_color(uint16_t item_id, int *r, int *g, int *b) {
    const ItemDef *def = item_get_def(item_id);
    if (def) {
        *r = def->color_r;
        *g = def->color_g;
        *b = def->color_b;
    } else {
        *r = 0;
        *g = 0;
        *b = 0;
    }
}

int item_get_sprite(uint16_t item_id) {
    const ItemDef *def = item_get_def(item_id);
    return def ? def->sprite_id : 0;
}

int item_is_clothing(uint16_t item_id) {
    const ItemDef *def = item_get_def(item_id);
    return def ? def->category == ITEM_CAT_CLOTHING : 0;
}

int item_clothing_slot(uint16_t item_id) {
    if (!item_is_clothing(item_id))
        return -1;
    if (item_id == ITEM_HAT)
        return 0;
    if (item_id == ITEM_SHIRT)
        return 1;
    if (item_id == ITEM_PANTS)
        return 2;
    return -1;
}

int item_is_tool(uint16_t item_id) {
    const ItemDef *def = item_get_def(item_id);
    return def ? def->category == ITEM_CAT_TOOL : 0;
}

int item_get_tool_power(uint16_t item_id) {
    const ItemDef *def = item_get_def(item_id);
    return def ? def->tool_power : 0;
}
