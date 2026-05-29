#include "player.h"
#include <string.h>
#include <math.h>

void player_init(Player *p, float x, float y)
{
    memset(p, 0, sizeof(*p));
    p->x = x;
    p->y = y;
    p->health = MAX_HEALTH;
    p->facing_right = 1;
}

void player_update(Player *p, float dt, int world_w, int world_h)
{
    p->vy += PLAYER_GRAVITY * dt;

    p->x += p->vx * dt;

    float hw = PLAYER_WIDTH / 2.0f;
    if (p->x - hw < 0)
        p->x = hw;
    if (p->x + hw > world_w)
        p->x = world_w - hw;

    p->y += p->vy * dt;

    p->on_ground = 0;
    if (p->y > world_h) {
        p->y = world_h;
        p->vy = 0;
        p->on_ground = 1;
    }
    if (p->y - PLAYER_HEIGHT < 0) {
        p->y = PLAYER_HEIGHT;
        p->vy = 0;
    }

    if (p->walking) {
        p->anim_timer += (int)(dt * 1000);
        if (p->anim_timer >= 200) {
            p->anim_timer -= 200;
            p->anim_frame = !p->anim_frame;
        }
    } else {
        p->anim_frame = 0;
        p->anim_timer = 0;
    }

    if (p->place_cooldown > 0)
        p->place_cooldown--;
}

void player_get_tile_pos(Player *p, int *tx, int *ty)
{
    *tx = (int)(p->x) / 32;
    *ty = (int)(p->y) / 32;
}

void player_get_facing_tile(Player *p, int *tx, int *ty)
{
    *tx = (int)(p->x) / 32 + (p->facing_right ? 1 : -1);
    *ty = (int)(p->y) / 32;
}
