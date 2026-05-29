#include "block.h"
#include "world.h"
#include <stddef.h>

const BlockDef BLOCK_DEFS[] = {
    {BLOCK_AIR, "Air", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {BLOCK_DIRT, "Dirt", 1, 0, 500, 1, BLOCK_DIRT, 1, SEED_DIRT, 1, 139, 90, 43},
    {BLOCK_STONE, "Stone", 1, 0, 1000, 2, BLOCK_STONE, 1, SEED_STONE, 2, 128, 128, 128},
    {BLOCK_GRASS, "Grass", 1, 0, 400, 1, BLOCK_DIRT, 1, SEED_GRASS, 3, 50, 150, 50},
    {BLOCK_BEDROCK, "Bedrock", 1, 0, 999999, 0, 0, 0, 0, 4, 40, 40, 40},
    {BLOCK_WOOD, "Wood", 1, 0, 600, 2, BLOCK_WOOD, 1, SEED_WOOD, 5, 160, 110, 50},
    {BLOCK_WOOD_BG, "Wood Background", 0, 1, 300, 1, BLOCK_WOOD_BG, 1, 0, 6, 180, 140, 80},
    {BLOCK_LEAVES, "Leaves", 0, 0, 200, 1, SEED_LEAVES, 1, SEED_LEAVES, 7, 30, 130, 30},
    {BLOCK_DOOR, "Door", 0, 0, 500, 2, BLOCK_DOOR, 1, 0, 8, 160, 110, 50},
    {BLOCK_BRICK, "Brick", 1, 0, 800, 3, BLOCK_BRICK, 1, SEED_BRICK, 9, 180, 80, 50},
    {BLOCK_GLASS, "Glass", 1, 0, 100, 2, 0, 0, 0, 10, 200, 220, 255},
    {BLOCK_SAND, "Sand", 1, 0, 400, 1, BLOCK_SAND, 1, SEED_SAND, 11, 210, 190, 130},
    {BLOCK_WATER, "Water", 0, 0, 0, 0, 0, 0, 0, 12, 50, 100, 200},
    {BLOCK_LAVA, "Lava", 0, 0, 0, 0, 0, 0, 0, 13, 220, 80, 20},
    {BLOCK_CLOTH_BG, "Cloth Background", 0, 1, 200, 1, BLOCK_CLOTH_BG, 1, 0, 14, 200, 180, 160},
    {BLOCK_ROCK, "Rock", 1, 0, 900, 3, BLOCK_ROCK, 1, SEED_ROCK, 15, 100, 100, 110},
    {BLOCK_LIMESTONE, "Limestone", 1, 0, 700, 2, BLOCK_LIMESTONE, 1, SEED_LIMESTONE, 16, 210, 200, 180},
    {BLOCK_MUD, "Mud", 1, 0, 450, 1, BLOCK_MUD, 1, SEED_MUD, 17, 100, 70, 40},
    {BLOCK_CLAY, "Clay", 1, 0, 500, 2, BLOCK_CLAY, 1, SEED_CLAY, 18, 170, 150, 140},
    {BLOCK_GRAVEL, "Gravel", 1, 0, 400, 1, BLOCK_GRAVEL, 1, 0, 19, 140, 140, 140},
    {BLOCK_ICE, "Ice", 1, 0, 300, 2, BLOCK_ICE, 1, SEED_ICE, 20, 180, 220, 250},
    {BLOCK_SNOW, "Snow", 1, 0, 200, 1, BLOCK_SNOW, 1, SEED_SNOW, 21, 240, 245, 255},
    {BLOCK_SIGN, "Sign", 0, 0, 400, 2, BLOCK_SIGN, 1, 0, 22, 160, 110, 50},
    {BLOCK_LOCK, "Lock", 0, 0, 600, 5, BLOCK_LOCK, 1, 0, 23, 60, 60, 70},
    {BLOCK_STORE, "Store", 0, 0, 700, 4, BLOCK_STORE, 1, 0, 24, 150, 100, 50},
    {BLOCK_MAILBOX, "Mailbox", 0, 0, 500, 3, BLOCK_MAILBOX, 1, 0, 25, 150, 100, 50},
    {BLOCK_PORTAL, "Portal", 0, 0, 600, 4, BLOCK_PORTAL, 1, 0, 26, 140, 50, 200},
    {BLOCK_TREE_CARCASS, "Tree Carcass", 1, 0, 700, 2, BLOCK_WOOD, 2, 0, 27, 120, 80, 40},
    {BG_DIRT, "Dirt Background", 0, 1, 300, 1, BG_DIRT, 1, 0, 129, 160, 110, 65},
    {BG_STONE, "Stone Background", 0, 1, 500, 2, BG_STONE, 1, 0, 130, 150, 150, 150},
    {BG_GRASS, "Grass Background", 0, 1, 250, 1, BG_GRASS, 1, 0, 131, 70, 170, 70},
    {BG_WOOD, "Wood Background", 0, 1, 300, 2, BG_WOOD, 1, 0, 133, 190, 150, 90},
    {BG_BRICK, "Brick Background", 0, 1, 400, 3, BG_BRICK, 1, 0, 137, 200, 110, 80},
    {BG_CLOTH, "Cloth Background", 0, 1, 200, 1, BG_CLOTH, 1, 0, 142, 220, 200, 180},
    {SEED_DIRT, "Dirt Seed", 0, 0, 200, 1, SEED_DIRT, 1, BLOCK_DIRT, 257, 160, 110, 60},
    {SEED_STONE, "Stone Seed", 0, 0, 200, 2, SEED_STONE, 1, BLOCK_STONE, 258, 150, 150, 150},
    {SEED_GRASS, "Grass Seed", 0, 0, 200, 1, SEED_GRASS, 1, BLOCK_GRASS, 259, 70, 170, 70},
    {SEED_WOOD, "Wood Seed", 0, 0, 200, 2, SEED_WOOD, 1, BLOCK_WOOD, 261, 180, 130, 70},
    {SEED_LEAVES, "Leaves Seed", 0, 0, 200, 1, SEED_LEAVES, 1, BLOCK_LEAVES, 263, 50, 150, 50},
    {SEED_BRICK, "Brick Seed", 0, 0, 200, 3, SEED_BRICK, 1, BLOCK_BRICK, 265, 200, 100, 70},
    {SEED_SAND, "Sand Seed", 0, 0, 200, 1, SEED_SAND, 1, BLOCK_SAND, 267, 230, 210, 150},
    {SEED_ROCK, "Rock Seed", 0, 0, 200, 3, SEED_ROCK, 1, BLOCK_ROCK, 271, 120, 120, 130},
    {SEED_LIMESTONE, "Limestone Seed", 0, 0, 200, 2, SEED_LIMESTONE, 1, BLOCK_LIMESTONE, 272, 230, 220, 200},
    {SEED_MUD, "Mud Seed", 0, 0, 200, 1, SEED_MUD, 1, BLOCK_MUD, 273, 120, 90, 60},
    {SEED_CLAY, "Clay Seed", 0, 0, 200, 2, SEED_CLAY, 1, BLOCK_CLAY, 274, 190, 170, 160},
    {SEED_ICE, "Ice Seed", 0, 0, 200, 2, SEED_ICE, 1, BLOCK_ICE, 276, 200, 240, 255},
    {SEED_SNOW, "Snow Seed", 0, 0, 200, 1, SEED_SNOW, 1, BLOCK_SNOW, 277, 250, 250, 255},
    {SEED_CACTUS, "Cactus Seed", 0, 0, 200, 2, SEED_CACTUS, 1, BLOCK_GRASS, 284, 50, 160, 50},
    {SEED_FLOWER, "Flower Seed", 0, 0, 200, 2, SEED_FLOWER, 1, BLOCK_GRASS, 285, 220, 100, 150},
    {SEED_MUSHROOM, "Mushroom Seed", 0, 0, 200, 3, SEED_MUSHROOM, 1, BLOCK_DIRT, 286, 180, 60, 60},
    {SEED_BUSH, "Bush Seed", 0, 0, 200, 1, SEED_BUSH, 1, BLOCK_LEAVES, 287, 40, 140, 40},
};

const int BLOCK_DEF_COUNT = sizeof(BLOCK_DEFS) / sizeof(BLOCK_DEFS[0]);

static const BlockDef *block_find(uint16_t id) {
    for (int i = 0; i < BLOCK_DEF_COUNT; i++) {
        if (BLOCK_DEFS[i].id == id) return &BLOCK_DEFS[i];
    }
    return NULL;
}

int block_is_solid(uint16_t block_id) {
    if (block_id == BLOCK_AIR) return 0;
    const BlockDef *b = block_find(block_id);
    return b ? b->is_solid : 0;
}

int block_is_background(uint16_t block_id) {
    const BlockDef *b = block_find(block_id);
    return b ? b->is_background : 0;
}

int block_get_break_time(uint16_t block_id) {
    const BlockDef *b = block_find(block_id);
    return b ? b->break_time_ms : 0;
}

int block_get_rarity(uint16_t block_id) {
    const BlockDef *b = block_find(block_id);
    return b ? b->rarity : 0;
}

uint16_t block_get_drop(uint16_t block_id) {
    const BlockDef *b = block_find(block_id);
    return b ? b->drop_item : 0;
}

int block_get_drop_count(uint16_t block_id) {
    const BlockDef *b = block_find(block_id);
    return b ? b->drop_count : 0;
}

uint16_t block_get_seed(uint16_t block_id) {
    const BlockDef *b = block_find(block_id);
    return b ? b->seed_id : 0;
}

const char *block_get_name(uint16_t block_id) {
    const BlockDef *b = block_find(block_id);
    return b ? b->name : "Unknown";
}

void block_get_color(uint16_t block_id, int *r, int *g, int *b) {
    const BlockDef *def = block_find(block_id);
    if (def) {
        *r = def->color_r;
        *g = def->color_g;
        *b = def->color_b;
    } else {
        *r = 255;
        *g = 0;
        *b = 255;
    }
}

int block_get_sprite(uint16_t block_id) {
    const BlockDef *b = block_find(block_id);
    return b ? b->tile_sprite_id : 0;
}
