#ifndef PLAYER_H
#define PLAYER_H

#include <stdint.h>
#include "../world/world.h"

#define PLAYER_WIDTH 24
#define PLAYER_HEIGHT 48
#define PLAYER_SPEED 200.0f
#define PLAYER_JUMP_SPEED -400.0f
#define PLAYER_GRAVITY 980.0f
#define MAX_HEALTH 100

typedef struct {
    float x, y;
    float vx, vy;
    int on_ground;
    int facing_right;
    int health;
    int gems;
    int breaking;
    int break_timer;
    int break_x, break_y;
    int place_cooldown;
    int anim_frame;
    int anim_timer;
    int walking;
} Player;

void player_init(Player *p, float x, float y);
void player_update(Player *p, float dt, int world_w, int world_h);
void player_collide(Player *p, World *w);
void player_get_tile_pos(Player *p, int *tx, int *ty);
void player_get_facing_tile(Player *p, int *tx, int *ty);

#endif
