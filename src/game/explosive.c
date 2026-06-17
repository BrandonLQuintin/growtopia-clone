#include "explosive.h"
#include "../world/block.h"
#include "../game/interact.h"
#include <math.h>
#include <stdio.h>

typedef struct {
    float x, y;
    float vx, vy;
    float flight;
    int   active;
} Bomb;

typedef struct {
    float x, y;
    float timer;
    int   active;
} Flash;

static Bomb  s_bomb  = {0};
static Flash s_flash = {0};

int explosive_throw(Player *p, Camera *cam, Input *in)
{
    if (s_bomb.active || s_flash.active) return 0;

    int cur_wx, cur_wy;
    camera_screen_to_world(cam, in->mouse_x, in->mouse_y, &cur_wx, &cur_wy);
    float cx = p->x;
    float cy = p->y - PLAYER_HEIGHT / 2.0f;
    float dx = (float)cur_wx - cx;
    float dy = (float)cur_wy - cy;
    float len = sqrtf(dx*dx + dy*dy);
    if (len < 0.001f) {
        dx = p->facing_right ? 1.0f : -1.0f;
        dy = 0.0f;
        len = 1.0f;
    }

    s_bomb.x = cx;
    s_bomb.y = cy;
    s_bomb.vx = (dx / len) * BOMB_THROW_SPEED;
    s_bomb.vy = (dy / len) * BOMB_THROW_SPEED;
    s_bomb.flight = 0.0f;
    s_bomb.active = 1;
    return 1;
}

static int is_bomb_spare(uint16_t fg)
{
    return fg == BLOCK_BEDROCK
        || fg == BLOCK_STORE
        || fg == BLOCK_LOCK
        || fg == BLOCK_MAILBOX
        || fg == BLOCK_TREE_CARCASS
        || block_is_interactive(fg);
}

static void detonate(World *w, Player *p, int cx, int cy)
{
    int R = BOMB_RADIUS_TILES;
    for (int ty = cy - R; ty <= cy + R; ty++) {
        for (int tx = cx - R; tx <= cx + R; tx++) {
            int dx = tx - cx, dy = ty - cy;
            if (dx*dx + dy*dy > R*R + R) continue;
            Tile *t = world_get_tile(w, tx, ty);
            if (!t) continue;
            if (t->fg == BLOCK_AIR) continue;
            if (is_bomb_spare(t->fg)) continue;
            interact_cleanup_break(w, tx, ty);
            t->fg = BLOCK_AIR;
            t->growth_stage = 0;
            t->growth_timer = 0;
            t->extra_data = 0;
        }
    }

    float center_x = (cx + 0.5f) * TILE_SIZE;
    float center_y = (cy + 0.5f) * TILE_SIZE;
    float pdx = p->x - center_x;
    float pdy = (p->y - PLAYER_HEIGHT / 2.0f) - center_y;
    float dist_sq = pdx*pdx + pdy*pdy;
    float radius_px = (BOMB_RADIUS_TILES + 0.5f) * TILE_SIZE;
    if (dist_sq <= radius_px * radius_px) {
        p->health -= BOMB_DAMAGE;
        float dist = sqrtf(dist_sq);
        if (dist < 0.001f) {
            p->vx = 0.0f;
            p->vy = -BOMB_KNOCKBACK;
        } else {
            float scale = 1.0f - (dist / radius_px);
            if (scale < 0.2f) scale = 0.2f;
            p->vx = (pdx / dist) * BOMB_KNOCKBACK * scale;
            p->vy = (pdy / dist) * BOMB_KNOCKBACK * scale - 80.0f;
        }
        p->on_ground = 0;
        if (p->health <= 0) {
            p->health = MAX_HEALTH;
            p->x = w->spawn_x;
            p->y = w->spawn_y;
            p->vx = 0;
            p->vy = 0;
            printf("Player killed by bomb, respawning.\n");
        }
    }

    s_flash.x = center_x;
    s_flash.y = center_y;
    s_flash.timer = 0.0f;
    s_flash.active = 1;
}

void explosive_update(World *w, Player *p, float dt)
{
    if (s_flash.active) {
        s_flash.timer += dt;
        if (s_flash.timer >= BOMB_FLASH_S) s_flash.active = 0;
    }

    if (!s_bomb.active) return;

    s_bomb.vy += BOMB_GRAVITY * dt;
    s_bomb.x  += s_bomb.vx * dt;
    s_bomb.y  += s_bomb.vy * dt;
    s_bomb.flight += dt;

    int tx = (int)(s_bomb.x / TILE_SIZE);
    int ty = (int)(s_bomb.y / TILE_SIZE);

    int detonate_now = 0;
    if (s_bomb.flight >= BOMB_MAX_FLIGHT_S) {
        detonate_now = 1;
    } else if (tx < 0 || tx >= w->width || ty < 0 || ty >= w->height) {
        detonate_now = 1;
    } else {
        if (world_is_solid(w, tx, ty)) detonate_now = 1;
    }

    if (detonate_now) {
        if (tx < 0) tx = 0;
        if (tx >= w->width)  tx = w->width - 1;
        if (ty < 0) ty = 0;
        if (ty >= w->height) ty = w->height - 1;
        detonate(w, p, tx, ty);
        s_bomb.active = 0;
    }
}

void explosive_render(Renderer *r, Camera *cam)
{
    if (s_bomb.active) {
        int sx, sy;
        camera_world_to_screen(cam, s_bomb.x, s_bomb.y, &sx, &sy);
        int s = 12;
        renderer_draw_rect(r, sx - s/2, sy - s/2, s, s, 0.16f, 0.16f, 0.16f, 1.0f);
    }
    if (s_flash.active) {
        int sx, sy;
        camera_world_to_screen(cam, s_flash.x, s_flash.y, &sx, &sy);
        float p = s_flash.timer / BOMB_FLASH_S;
        int half = (int)(8.0f + ((BOMB_RADIUS_TILES + 1) * TILE_SIZE - 8.0f) * p);
        float a = 1.0f - p;
        renderer_draw_rect(r, sx - half, sy - half, half*2, half*2,
                           1.0f, 0.78f, 0.24f, a);
    }
}

void explosive_reset(void)
{
    s_bomb.active = 0;
    s_flash.active = 0;
}
