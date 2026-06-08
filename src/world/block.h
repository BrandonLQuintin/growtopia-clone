#ifndef BLOCK_H
#define BLOCK_H

#include <stdint.h>

typedef struct {
    uint16_t id;
    const char *name;
    int is_solid;
    int is_background;
    int break_time_ms;
    int rarity;
    uint16_t drop_item;
    int drop_count;
    uint16_t seed_id;
    int tile_sprite_id;
    int color_r, color_g, color_b;
} BlockDef;

int block_is_solid(uint16_t block_id);
int block_is_background(uint16_t block_id);
int block_get_break_time(uint16_t block_id);
int block_get_rarity(uint16_t block_id);
uint16_t block_get_drop(uint16_t block_id);
int block_get_drop_count(uint16_t block_id);
uint16_t block_get_seed(uint16_t block_id);
const char *block_get_name(uint16_t block_id);
void block_get_color(uint16_t block_id, int *r, int *g, int *b);
int block_get_sprite(uint16_t block_id);
int block_is_solid_with_data(uint16_t block_id, uint32_t extra_data);
int block_is_interactive(uint16_t block_id);

extern const BlockDef BLOCK_DEFS[];
extern const int BLOCK_DEF_COUNT;

#endif
