#ifndef LAVA_H
#define LAVA_H

#include "../world/world.h"
#include "../game/player.h"

#define LAVA_DAMAGE_PER_SEC     20.0f
#define LAVA_BOUNCE_VELOCITY  -550.0f

#define LAVA_POOL_MIN            5
#define LAVA_POOL_MAX            8
#define LAVA_POOL_MAX_TILES     24
#define LAVA_POOL_DEPTH_MIN     40
#define LAVA_POOL_DEPTH_MAX     56

void lava_update(World *w, Player *p, float dt);
void lava_generate_pools(World *w);

#endif
