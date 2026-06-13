#include "interact.h"
#include "../world/block.h"
#include "../engine/renderer.h"
#include <string.h>
#include "../engine/input.h"
#include "../engine/camera.h"
#include "inventory.h"
#include "../engine/ui.h"
#include "../world/items.h"
#include "crafting.h"
#include "farming.h"
#include <stdlib.h>

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

int interact_handle_click(World *w, Player *p, Inventory *inv, int hotbar_sel,
                          Input *in, Camera *cam,
                          int *sign_x, int *sign_y)
{
    if (!input_is_mouse_clicked(in, 1))
        return 0;

    int mouse_wx, mouse_wy;
    camera_screen_to_world(cam, in->mouse_x, in->mouse_y, &mouse_wx, &mouse_wy);
    mouse_wx /= TILE_SIZE;
    mouse_wy /= TILE_SIZE;

    int px = (int)(p->x / TILE_SIZE);
    int py = (int)(p->y / TILE_SIZE);
    int dist_x = mouse_wx - px;
    int dist_y = mouse_wy - py;

    if (dist_x * dist_x + dist_y * dist_y > REACH_RADIUS * REACH_RADIUS)
        return 0;

    Tile *t = world_get_tile(w, mouse_wx, mouse_wy);
    if (!t || !block_is_interactive(t->fg))
        return 0;

    int tool_id = inventory_get_hotbar_item(inv, hotbar_sel);
    if (tool_id == ITEM_PICKAXE)
        return 0;

    int result = interact_punch(w, p, mouse_wx, mouse_wy);
    p->breaking = 0;
    p->break_timer = 0;

    if (result == 2) {
        const char *txt = interact_get_sign_text(w, mouse_wx, mouse_wy);
        if (txt) {
            *sign_x = mouse_wx;
            *sign_y = mouse_wy;
            return 2;
        }
    }
    return 1;
}

void interact_handle_break(World *w, Player *p, Inventory *inv, int hotbar_sel,
                           Input *in, Camera *cam, float dt)
{
    if (!input_is_mouse_down(in, 1)) {
        p->breaking = 0;
        p->break_timer = 0;
        return;
    }

    int mouse_wx, mouse_wy;
    camera_screen_to_world(cam, in->mouse_x, in->mouse_y, &mouse_wx, &mouse_wy);
    mouse_wx /= TILE_SIZE;
    mouse_wy /= TILE_SIZE;

    int px = (int)(p->x / TILE_SIZE);
    int py = (int)(p->y / TILE_SIZE);
    int dist_x = mouse_wx - px;
    int dist_y = mouse_wy - py;

    if (dist_x * dist_x + dist_y * dist_y > REACH_RADIUS * REACH_RADIUS)
        return;

    Tile *t = world_get_tile(w, mouse_wx, mouse_wy);
    if (!t || t->fg == BLOCK_AIR || t->fg == BLOCK_BEDROCK)
        return;

    int tool_id = inventory_get_hotbar_item(inv, hotbar_sel);

    if (block_is_interactive(t->fg) && tool_id != ITEM_PICKAXE) {
        return;
    }

    if (p->breaking && p->break_x == mouse_wx && p->break_y == mouse_wy) {
        p->break_timer += (int)(dt * 1000);
        int break_time = block_get_break_time(t->fg);
        int power = item_get_tool_power(tool_id);
        if (power > 0) {
            p->break_timer += (int)(power * dt * 1000);
        }
        if (p->break_timer >= break_time) {
            uint16_t drop = block_get_drop(t->fg);
            int count = block_get_drop_count(t->fg);
            if (drop != 0 && count > 0) {
                inventory_add(inv, drop, count);
            }
            if (t->growth_stage >= GROWTH_COMPLETE) {
                uint16_t drops[8];
                int dcounts[8];
                int ndrops = 0;
                farming_harvest(w, mouse_wx, mouse_wy, drops, dcounts, &ndrops);
                for (int i = 0; i < ndrops; i++) {
                    inventory_add(inv, drops[i], dcounts[i]);
                }
                int gem_drop = 1 + (rand() % 3);
                p->gems += gem_drop;
            }
            interact_cleanup_break(w, mouse_wx, mouse_wy);
            t->fg = BLOCK_AIR;
            t->growth_stage = 0;
            t->growth_timer = 0;
            t->extra_data = 0;
            p->breaking = 0;
            p->break_timer = 0;
        }
    } else {
        p->breaking = 1;
        p->break_x = mouse_wx;
        p->break_y = mouse_wy;
        p->break_timer = 0;
    }
}

void interact_handle_place(World *w, Player *p, Inventory *inv, int hotbar_sel,
                           Input *in, Camera *cam, UI *ui,
                           int *portal_pending, int *portal_x, int *portal_y)
{
    if (!input_is_mouse_clicked(in, 3))
        return;

    int mouse_wx, mouse_wy;
    camera_screen_to_world(cam, in->mouse_x, in->mouse_y, &mouse_wx, &mouse_wy);
    mouse_wx /= TILE_SIZE;
    mouse_wy /= TILE_SIZE;

    int px = (int)(p->x / TILE_SIZE);
    int py = (int)(p->y / TILE_SIZE);
    int dist_x = mouse_wx - px;
    int dist_y = mouse_wy - py;

    if (dist_x * dist_x + dist_y * dist_y > REACH_RADIUS * REACH_RADIUS)
        return;

    Tile *t = world_get_tile(w, mouse_wx, mouse_wy);
    if (!t)
        return;

    uint16_t held = inventory_get_hotbar_item(inv, hotbar_sel);
    int held_count = inventory_get_hotbar_count(inv, hotbar_sel);

    if (held == ITEM_WRENCH && t->fg != BLOCK_AIR) {
        int result = interact_wrench(w, mouse_wx, mouse_wy,
            portal_x, portal_y, portal_pending);
        if (result == 1) {
            ui_init_sign_edit(ui, w, mouse_wx, mouse_wy);
        }
        return;
    }

    if (held != 0 && held_count > 0) {
        const ItemDef *def = item_get_def(held);
        if (def && def->is_seed && t->growth_stage >= GROWTH_STAGE_1 && t->growth_stage < GROWTH_COMPLETE) {
            uint16_t tile_seed = (uint16_t)t->extra_data;
            uint16_t result;
            if (crafting_splice(held, tile_seed, &result) == 0) {
                farming_plant_seed(w, mouse_wx, mouse_wy, result);
                inventory_remove(inv, held, 1);
            }
        } else if (def && t->fg == BLOCK_AIR) {
            if (def->is_seed) {
                if (farming_can_plant(w, mouse_wx, mouse_wy)) {
                    farming_plant_seed(w, mouse_wx, mouse_wy, held);
                    inventory_remove(inv, held, 1);
                }
            } else if (def->category == ITEM_CAT_BLOCK) {
                t->fg = held;
                t->extra_data = 0;
                inventory_remove(inv, held, 1);
            }
        }
    }
}
