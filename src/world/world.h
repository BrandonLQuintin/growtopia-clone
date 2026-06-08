#ifndef WORLD_H
#define WORLD_H

#include <stdint.h>

#define WORLD_WIDTH 100
#define WORLD_HEIGHT 60
#define WORLD_TOTAL (WORLD_WIDTH * WORLD_HEIGHT)

#define BLOCK_AIR 0
#define BLOCK_DIRT 1
#define BLOCK_STONE 2
#define BLOCK_GRASS 3
#define BLOCK_BEDROCK 4
#define BLOCK_WOOD 5
#define BLOCK_WOOD_BG 6
#define BLOCK_LEAVES 7
#define BLOCK_DOOR 8
#define BLOCK_BRICK 9
#define BLOCK_GLASS 10
#define BLOCK_SAND 11
#define BLOCK_WATER 12
#define BLOCK_LAVA 13
#define BLOCK_CLOTH_BG 14
#define BLOCK_ROCK 15
#define BLOCK_LIMESTONE 16
#define BLOCK_MUD 17
#define BLOCK_CLAY 18
#define BLOCK_GRAVEL 19
#define BLOCK_ICE 20
#define BLOCK_SNOW 21
#define BLOCK_SIGN 22
#define BLOCK_LOCK 23
#define BLOCK_STORE 24
#define BLOCK_MAILBOX 25
#define BLOCK_PORTAL 26
#define BLOCK_TREE_CARCASS 27
#define BLOCK_FOREGROUND_MAX 128

#define BG_BLOCK_OFFSET 128
#define BG_DIRT (128 + 1)
#define BG_STONE (128 + 2)
#define BG_GRASS (128 + 3)
#define BG_WOOD (128 + 5)
#define BG_BRICK (128 + 9)
#define BG_CLOTH (128 + 14)

#define SEED_OFFSET 256
#define SEED_DIRT (256 + 1)
#define SEED_STONE (256 + 2)
#define SEED_GRASS (256 + 3)
#define SEED_WOOD (256 + 5)
#define SEED_LEAVES (256 + 7)
#define SEED_BRICK (256 + 9)
#define SEED_SAND (256 + 11)
#define SEED_ROCK (256 + 15)
#define SEED_LIMESTONE (256 + 16)
#define SEED_MUD (256 + 17)
#define SEED_CLAY (256 + 18)
#define SEED_ICE (256 + 20)
#define SEED_SNOW (256 + 21)
#define SEED_CACTUS (256 + 28)
#define SEED_FLOWER (256 + 29)
#define SEED_MUSHROOM (256 + 30)
#define SEED_BUSH (256 + 31)
#define SEED_MAX (256 + 64)

#define GROWTH_STAGE_1 1
#define GROWTH_STAGE_2 2
#define GROWTH_STAGE_3 3
#define GROWTH_STAGE_4 4
#define GROWTH_COMPLETE 5

#define SIGN_TABLE_SIZE 64
#define SIGN_TEXT_MAX_LEN 32
#define PORTAL_UNLINKED 0

typedef struct {
    uint16_t fg;
    uint16_t bg;
    uint8_t growth_stage;
    uint32_t growth_timer;
    uint32_t extra_data;
} Tile;

typedef struct {
    int width;
    int height;
    Tile *tiles;
    char name[64];
    char sign_texts[SIGN_TABLE_SIZE][SIGN_TEXT_MAX_LEN + 1];
    int sign_count;
} World;

int world_init(World *w, int width, int height);
void world_free(World *w);
void world_generate(World *w);
Tile *world_get_tile(World *w, int x, int y);
int world_set_fg(World *w, int x, int y, uint16_t block_id);
int world_set_bg(World *w, int x, int y, uint16_t block_id);
int world_save(World *w, const char *path);
int world_load(World *w, const char *path);
int world_is_solid(World *w, int x, int y);

#endif
