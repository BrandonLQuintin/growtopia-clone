#ifndef ITEMS_H
#define ITEMS_H

#include <stdint.h>

typedef enum {
    ITEM_CAT_BLOCK,
    ITEM_CAT_SEED,
    ITEM_CAT_TOOL,
    ITEM_CAT_CLOTHING,
    ITEM_CAT_CONSUMABLE,
    ITEM_CAT_CURRENCY
} ItemCategory;

typedef struct {
    uint16_t id;
    const char *name;
    ItemCategory category;
    int max_stack;
    int gem_cost;
    int sell_cost;
    int sprite_id;
    int color_r, color_g, color_b;
    int is_seed;
    uint16_t seed_grows_into;
    int grow_time_ms;
    int harvest_count;
    int harvest_seed_count;
    int tool_power;
} ItemDef;

const ItemDef *item_get_def(uint16_t item_id);
const char *item_get_name(uint16_t item_id);
int item_get_max_stack(uint16_t item_id);
int item_is_seed(uint16_t item_id);
int item_is_block(uint16_t item_id);
uint16_t item_get_seed_product(uint16_t seed_id);
int item_get_grow_time(uint16_t seed_id);
int item_get_harvest_count(uint16_t seed_id);
int item_get_harvest_seed_count(uint16_t seed_id);
int item_get_gem_cost(uint16_t item_id);
int item_get_sell_cost(uint16_t item_id);
void item_get_color(uint16_t item_id, int *r, int *g, int *b);
int item_get_sprite(uint16_t item_id);
int item_is_clothing(uint16_t item_id);
int item_clothing_slot(uint16_t item_id);
int item_is_tool(uint16_t item_id);
int item_get_tool_power(uint16_t item_id);

extern const ItemDef ITEM_DEFS[];
extern const int ITEM_DEF_COUNT;

#define ITEM_FIST 0
#define ITEM_GEMS 9999
#define ITEM_WRENCH  9000
#define ITEM_PICKAXE 9001
#define ITEM_HAT     9100
#define ITEM_SHIRT   9101
#define ITEM_PANTS   9102
#define ITEM_BOMB   9200

#endif
