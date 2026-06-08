#include "interact.h"
#include "../world/block.h"
#include "../engine/renderer.h"
#include <string.h>

int interact_punch(World *w, Player *p, int tx, int ty) {
    Tile *t = world_get_tile(w, tx, ty);
    if (!t) return 0;

    if (t->fg == BLOCK_DOOR) {
        t->extra_data ^= 1;
        return 1;
    }

    if (t->fg == BLOCK_SIGN) {
        return 2;
    }

    if (t->fg == BLOCK_PORTAL) {
        if (t->extra_data == PORTAL_UNLINKED) return 0;
        int px = ((int)(t->extra_data >> 16)) - 1;
        int py = ((int)(t->extra_data & 0xFFFF)) - 1;
        if (px >= 0 && px < w->width && py >= 0 && py < w->height) {
            p->x = px * TILE_SIZE + TILE_SIZE / 2.0f;
            p->y = (py + 1) * TILE_SIZE;
            p->vx = 0;
            p->vy = 0;
        }
        return 3;
    }

    return 0;
}

int interact_wrench(World *w, int tx, int ty, int *pending_x, int *pending_y, int *pending) {
    Tile *t = world_get_tile(w, tx, ty);
    if (!t) return 0;

    if (t->fg == BLOCK_SIGN) {
        return 1;
    }

    if (t->fg == BLOCK_PORTAL) {
        if (!(*pending)) {
            *pending_x = tx;
            *pending_y = ty;
            *pending = 1;
            return 0;
        } else {
            if (*pending_x == tx && *pending_y == ty) return 0;
            Tile *a = world_get_tile(w, *pending_x, *pending_y);
            if (!a || a->fg != BLOCK_PORTAL) {
                *pending = 0;
                return 0;
            }
            uint32_t link_a = ((uint32_t)(tx + 1) << 16) | (uint32_t)(ty + 1);
            uint32_t link_b = ((uint32_t)(*pending_x + 1) << 16) | (uint32_t)(*pending_y + 1);
            if (a->extra_data != PORTAL_UNLINKED) {
                int opx = ((int)(a->extra_data >> 16)) - 1;
                int opy = ((int)(a->extra_data & 0xFFFF)) - 1;
                if (opx >= 0 && opx < w->width && opy >= 0 && opy < w->height) {
                    Tile *op = world_get_tile(w, opx, opy);
                    if (op) op->extra_data = PORTAL_UNLINKED;
                }
            }
            if (t->extra_data != PORTAL_UNLINKED) {
                int opx = ((int)(t->extra_data >> 16)) - 1;
                int opy = ((int)(t->extra_data & 0xFFFF)) - 1;
                if (opx >= 0 && opx < w->width && opy >= 0 && opy < w->height) {
                    Tile *op = world_get_tile(w, opx, opy);
                    if (op) op->extra_data = PORTAL_UNLINKED;
                }
            }
            a->extra_data = link_a;
            t->extra_data = link_b;
            *pending = 0;
            return 2;
        }
    }

    return 0;
}

const char *interact_get_sign_text(World *w, int tx, int ty) {
    Tile *t = world_get_tile(w, tx, ty);
    if (!t || t->fg != BLOCK_SIGN || t->extra_data == 0) return NULL;
    int idx = (int)t->extra_data - 1;
    if (idx < 0 || idx >= SIGN_TABLE_SIZE) return NULL;
    if (w->sign_texts[idx][0] == '\0') return NULL;
    return w->sign_texts[idx];
}

int interact_alloc_sign(World *w, int tx, int ty) {
    Tile *t = world_get_tile(w, tx, ty);
    if (!t || t->fg != BLOCK_SIGN) return 0;
    if (t->extra_data != 0) return (int)t->extra_data;
    for (int i = 0; i < SIGN_TABLE_SIZE; i++) {
        if (w->sign_texts[i][0] == '\0') {
            t->extra_data = (uint32_t)(i + 1);
            if (i >= w->sign_count) w->sign_count = i + 1;
            return (int)t->extra_data;
        }
    }
    return 0;
}

void interact_cleanup_break(World *w, int tx, int ty) {
    Tile *t = world_get_tile(w, tx, ty);
    if (!t) return;

    if (t->fg == BLOCK_SIGN) {
        if (t->extra_data != 0) {
            int idx = (int)t->extra_data - 1;
            if (idx >= 0 && idx < SIGN_TABLE_SIZE) {
                memset(w->sign_texts[idx], 0, SIGN_TEXT_MAX_LEN + 1);
            }
            t->extra_data = 0;
        }
    }

    if (t->fg == BLOCK_PORTAL) {
        if (t->extra_data != PORTAL_UNLINKED) {
            int px = ((int)(t->extra_data >> 16)) - 1;
            int py = ((int)(t->extra_data & 0xFFFF)) - 1;
            t->extra_data = PORTAL_UNLINKED;
            if (px >= 0 && px < w->width && py >= 0 && py < w->height) {
                Tile *partner = world_get_tile(w, px, py);
                if (partner) {
                    partner->extra_data = PORTAL_UNLINKED;
                }
            }
        }
    }
}

int interact_is_portal_linked(World *w, int tx, int ty) {
    Tile *t = world_get_tile(w, tx, ty);
    if (!t) return 0;
    return t->extra_data != PORTAL_UNLINKED;
}
