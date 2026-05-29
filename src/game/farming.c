#include "farming.h"
#include "../world/items.h"

void farming_update(World *w, float dt)
{
    uint32_t dt_ms = (uint32_t)(dt * 1000);
    for (int i = 0; i < w->width * w->height; i++) {
        Tile *t = &w->tiles[i];
        if (t->growth_stage > 0 && t->growth_stage < GROWTH_COMPLETE) {
            t->growth_timer += dt_ms;
            uint16_t seed_id = (uint16_t)t->extra_data;
            uint32_t grow_time = (uint32_t)item_get_grow_time(seed_id);
            if (grow_time > 0 && t->growth_timer >= grow_time) {
                t->growth_stage++;
                t->growth_timer = 0;
            }
        }
    }
}

int farming_plant_seed(World *w, int x, int y, uint16_t seed_id)
{
    Tile *t = world_get_tile(w, x, y);
    if (!t) return -1;
    const ItemDef *def = item_get_def(seed_id);
    if (!def || !def->is_seed) return -1;
    t->fg = def->seed_grows_into;
    t->growth_stage = GROWTH_STAGE_1;
    t->growth_timer = 0;
    t->extra_data = seed_id;
    return 0;
}

int farming_harvest(World *w, int x, int y, uint16_t *drops, int *drop_counts, int *drop_count)
{
    Tile *t = world_get_tile(w, x, y);
    if (!t || t->growth_stage < GROWTH_COMPLETE) return -1;
    *drop_count = 0;
    uint16_t seed_id = (uint16_t)t->extra_data;
    uint16_t product = item_get_seed_product(seed_id);
    int harvest_count = item_get_harvest_count(seed_id);
    int seed_count = item_get_harvest_seed_count(seed_id);
    if (product != 0 && harvest_count > 0 && *drop_count < 8) {
        drops[*drop_count] = product;
        drop_counts[*drop_count] = harvest_count;
        (*drop_count)++;
    }
    if (seed_id != 0 && seed_count > 0 && *drop_count < 8) {
        drops[*drop_count] = seed_id;
        drop_counts[*drop_count] = seed_count;
        (*drop_count)++;
    }
    return 0;
}

int farming_can_plant(World *w, int x, int y)
{
    Tile *t = world_get_tile(w, x, y);
    if (!t || t->fg != BLOCK_AIR) return 0;
    Tile *below = world_get_tile(w, x, y + 1);
    if (!below) return 0;
    return below->fg != BLOCK_AIR && world_is_solid(w, x, y + 1);
}

int farming_get_growth_percent(World *w, int x, int y)
{
    Tile *t = world_get_tile(w, x, y);
    if (!t || t->growth_stage == 0) return 0;
    if (t->growth_stage >= GROWTH_COMPLETE) return 100;
    uint16_t seed_id = (uint16_t)t->extra_data;
    uint32_t grow_time = (uint32_t)item_get_grow_time(seed_id);
    if (grow_time == 0) return 0;
    return (int)((t->growth_timer * 100) / grow_time);
}
