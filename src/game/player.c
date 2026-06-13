#include "player.h"
#include <string.h>
#include <math.h>
#include "../engine/renderer.h"

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
    if (!p->on_ground || p->vy < 0) {
        p->vy += PLAYER_GRAVITY * dt;
    }

    if (p->vy > 800.0f) p->vy = 800.0f;

    p->x += p->vx * dt;

    float hw = PLAYER_WIDTH / 2.0f;
    if (p->x - hw < 0)
        p->x = hw;
    if (p->x + hw > world_w)
        p->x = world_w - hw;

    p->y += p->vy * dt;

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

void player_collide(Player *p, World *w)
{
    float hw = PLAYER_WIDTH / 2.0f;
    float left = p->x - hw;
    float right = p->x + hw;
    float top = p->y - PLAYER_HEIGHT;
    float bottom = p->y;

    int tile_left = (int)(left / TILE_SIZE);
    int tile_right = (int)(right / TILE_SIZE);
    int tile_top = (int)(top / TILE_SIZE);
    int tile_bottom = (int)(bottom / TILE_SIZE);

    p->on_ground = 0;

    for (int ty = tile_top; ty <= tile_bottom; ty++) {
        for (int tx = tile_left; tx <= tile_right; tx++) {
            if (world_is_solid(w, tx, ty)) {
                float block_left = tx * TILE_SIZE;
                float block_right = block_left + TILE_SIZE;
                float block_top = ty * TILE_SIZE;
                float block_bottom = block_top + TILE_SIZE;

                float overlap_left = right - block_left;
                float overlap_right = block_right - left;
                float overlap_top = bottom - block_top;
                float overlap_bottom = block_bottom - top;

                float min_overlap = overlap_left;
                int resolve_axis = 0;

                if (overlap_right < min_overlap) { min_overlap = overlap_right; resolve_axis = 1; }
                if (overlap_top < min_overlap) { min_overlap = overlap_top; resolve_axis = 2; }
                if (overlap_bottom < min_overlap) { min_overlap = overlap_bottom; resolve_axis = 3; }

                switch (resolve_axis) {
                    case 0:
                        p->x = block_left - hw;
                        p->vx = 0;
                        break;
                    case 1:
                        p->x = block_right + hw;
                        p->vx = 0;
                        break;
                    case 2:
                        p->y = block_top;
                        p->vy = 0;
                        p->on_ground = 1;
                        break;
                    case 3:
                        p->y = block_bottom + PLAYER_HEIGHT;
                        p->vy = 0;
                        break;
                }

                left = p->x - hw;
                right = p->x + hw;
                top = p->y - PLAYER_HEIGHT;
                bottom = p->y;
            }
        }
    }
}
