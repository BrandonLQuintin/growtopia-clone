#ifndef FARMING_H
#define FARMING_H

#include "../world/world.h"

void farming_update(World *w, float dt);
void farming_tick_nearby(World *w, int px, int py, float dt);
int farming_plant_seed(World *w, int x, int y, uint16_t seed_id);
int farming_harvest(World *w, int x, int y, uint16_t *drops, int *drop_counts, int *drop_count);
int farming_can_plant(World *w, int x, int y);
int farming_get_growth_percent(World *w, int x, int y);

#endif
