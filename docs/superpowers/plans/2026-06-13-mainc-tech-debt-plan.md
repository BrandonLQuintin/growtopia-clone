# main.c Tech Debt Refactor Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Slim main.c from ~780 to ~300 lines by extracting game logic into the modules that own each domain, unifying RNG onto the existing `prng` module, and removing dead code.

**Architecture:** Approach A — expand existing modules (player.c, interact.c, store.c, farming.c, renderer.c, ui.c). No new files. Each extraction is independently compilable. Build verification after each step (`make clean && make`, zero warnings under `-Wall -Wextra`).

**Tech Stack:** C11, SDL2, OpenGL, Make. No test framework — verification is build + manual playtest.

**Spec:** `docs/superpowers/specs/2026-06-13-mainc-tech-debt-design.md`

---

## File Structure

| File | Changes |
|---|---|
| `src/engine/prng.c` | Fix `prng_float` return range bug |
| `src/game/player.h` | Add `player_collide` declaration |
| `src/game/player.c` | Add `player_collide` + includes |
| `src/game/farming.h` | Add `farming_tick_nearby` declaration |
| `src/game/farming.c` | Add `farming_tick_nearby` |
| `src/game/interact.h` | Add `REACH_RADIUS` + 3 handler declarations |
| `src/game/interact.c` | Add 3 handlers + includes |
| `src/game/store.h` | Add layout constants + `store_handle_click` |
| `src/game/store.c` | Add `store_handle_click`, remove local `#define`s |
| `src/engine/ui.h` | Add `ui_handle_sign_edit_event` + `ui_render_exit_confirm` |
| `src/engine/ui.c` | Add 2 functions, use shared store constants |
| `src/engine/renderer.h` | Add `renderer_draw_world` + `renderer_draw_player` |
| `src/engine/renderer.c` | Add 2 functions + includes |
| `src/world/items.h` | Add `ITEM_WRENCH`/`ITEM_PICKAXE` |
| `src/world/world.h` | Add `Prng rng` field + `#include prng.h` |
| `src/world/world.c` | Seed rng, use prng in generation |
| `src/game/inventory.h` | Remove dead `inventory_save`/`inventory_load` |
| `src/game/inventory.c` | Remove dead functions |
| `src/main.c` | Slim to orchestrator — every extraction replaces inline code with a call |

---

## Task 1: Fix `prng_float` Bug (Prerequisite)

**Files:**
- Modify: `src/engine/prng.c:8-12`

- [ ] **Step 1: Fix the return value**

In `src/engine/prng.c`, change the `prng_float` function body from:

```c
float prng_float(Prng *p)
{
    p->state = p->state * 1664525u + 1013904223u;
    return (float)p->state / (float)0x80000000u;
}
```

to:

```c
float prng_float(Prng *p)
{
    p->state = p->state * 1664525u + 1013904223u;
    return (float)(p->state >> 1) / (float)0x80000000u;
}
```

The right-shift produces `[0, 2^31-1]`, divided by `2^31` yields `[0, 1.0)`.

- [ ] **Step 2: Build and verify**

Run: `make clean && make`
Expected: Compiles with zero warnings.

- [ ] **Step 3: Commit**

```bash
git add src/engine/prng.c
git commit -m "fix: prng_float returns [0,1) instead of [0,2)"
```

---

## Task 2: Extract `player_collide`

**Files:**
- Modify: `src/game/player.h:29-33`
- Modify: `src/game/player.c:1-4`
- Modify: `src/main.c:329-394`

- [ ] **Step 1: Add declaration + include to player.h**

In `src/game/player.h`, after `#include <stdint.h>`, add:

```c
#include "../world/world.h"
```

After the `void player_update(...)` line, add:

```c
void player_collide(Player *p, World *w);
```

- [ ] **Step 2: Add include to player.c**

In `src/game/player.c`, after the existing includes, add:

```c
#include "../engine/renderer.h"
```

- [ ] **Step 3: Add `player_collide` to player.c**

At the end of `src/game/player.c`, add:

```c
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
```

- [ ] **Step 4: Replace inline collision in main.c with a call**

In `src/main.c`, replace the entire block from line 331 (the `{` opening the collision scope) through line 394 (the closing `}`) with:

```c
    player_collide(&g->player, &g->world);
```

This replaces the 63-line collision block (main.c:331-394) with a single call. The block starts with `    {` on line 331 and ends with `    }` on line 394.

- [ ] **Step 5: Build and verify**

Run: `make clean && make`
Expected: Compiles with zero warnings.

- [ ] **Step 6: Commit**

```bash
git add src/game/player.h src/game/player.c src/main.c
git commit -m "refactor: extract player_collide into player.c"
```

---

## Task 3: Extract `farming_tick_nearby` + Delete Dead `facing_x`/`facing_y`

**Files:**
- Modify: `src/game/farming.h:6-11`
- Modify: `src/game/farming.c`
- Modify: `src/main.c:396-414`

- [ ] **Step 1: Add declaration to farming.h**

In `src/game/farming.h`, after `void farming_update(World *w, float dt);`, add:

```c
void farming_tick_nearby(World *w, int px, int py, float dt);
```

- [ ] **Step 2: Add `farming_tick_nearby` to farming.c**

In `src/game/farming.c`, add this function (before the existing `farming_update` or at the top after includes):

```c
void farming_tick_nearby(World *w, int px, int py, float dt)
{
    for (int dy = -1; dy <= 2; dy++) {
        for (int dx = -2; dx <= 2; dx++) {
            int cx = px + dx;
            int cy = py + dy;
            if (cx >= 0 && cx < w->width && cy >= 0 && cy < w->height) {
                Tile *t = world_get_tile(w, cx, cy);
                if (t && t->growth_stage > 0 && t->growth_stage < GROWTH_COMPLETE) {
                    t->growth_timer += (uint32_t)(dt * 1000);
                }
            }
        }
    }
}
```

- [ ] **Step 3: Replace inline growth tick + delete dead locals in main.c**

In `src/main.c`, find the block starting at line 396:

```c
    int px = (int)(g->player.x / TILE_SIZE);
    int py = (int)(g->player.y / TILE_SIZE);
    
    for (int dy = -1; dy <= 2; dy++) {
        for (int dx = -2; dx <= 2; dx++) {
            int cx = px + dx;
            int cy = py + dy;
            if (cx >= 0 && cx < g->world.width && cy >= 0 && cy < g->world.height) {
                Tile *t = world_get_tile(&g->world, cx, cy);
                if (t && t->growth_stage > 0 && t->growth_stage < GROWTH_COMPLETE) {
                    t->growth_timer += (uint32_t)(dt * 1000);
                }
            }
        }
    }
    farming_update(&g->world, dt);
    
    int facing_x, facing_y;
    player_get_facing_tile(&g->player, &facing_x, &facing_y);
    
```

Replace it with:

```c
    int px = (int)(g->player.x / TILE_SIZE);
    int py = (int)(g->player.y / TILE_SIZE);

    farming_tick_nearby(&g->world, px, py, dt);
    farming_update(&g->world, dt);

```

This replaces the inline growth-tick loop with a call, keeps `farming_update` inline, and deletes the dead `facing_x`/`facing_y` declaration + call.

- [ ] **Step 4: Build and verify**

Run: `make clean && make`
Expected: Compiles with zero warnings.

- [ ] **Step 5: Commit**

```bash
git add src/game/farming.h src/game/farming.c src/main.c
git commit -m "refactor: extract farming_tick_nearby, delete dead facing vars"
```

---

## Task 4: Centralize `ITEM_WRENCH`/`ITEM_PICKAXE`

This must run before Task 5 (interaction handlers use these constants).

**Files:**
- Modify: `src/world/items.h:52-53`
- Modify: `src/main.c:28-29`
- Modify: `src/game/store.c:6-7`

- [ ] **Step 1: Add constants to items.h**

In `src/world/items.h`, after `#define ITEM_GEMS 9999`, add:

```c
#define ITEM_WRENCH  9000
#define ITEM_PICKAXE 9001
```

- [ ] **Step 2: Remove local defines from main.c**

In `src/main.c`, delete lines 28-29:

```c
#define ITEM_WRENCH 9000
#define ITEM_PICKAXE 9001
```

(These are resolved via `items.h` which main.c already includes.)

- [ ] **Step 3: Remove local defines from store.c**

In `src/game/store.c`, delete lines 6-7:

```c
#define ITEM_WRENCH 9000
#define ITEM_PICKAXE 9001
```

(store.c already includes `../world/items.h`.)

- [ ] **Step 4: Build and verify**

Run: `make clean && make`
Expected: Compiles with zero warnings.

- [ ] **Step 5: Commit**

```bash
git add src/world/items.h src/main.c src/game/store.c
git commit -m "refactor: centralize ITEM_WRENCH/ITEM_PICKAXE in items.h"
```

---

## Task 5: Extract Interaction Handlers (`interact_handle_click` / `break` / `place`)

This is the largest task. It extracts ~140 lines of interaction logic, eliminates the `goto`, and introduces `REACH_RADIUS`.

**Files:**
- Modify: `src/game/interact.h:1-15`
- Modify: `src/game/interact.c:1-5`
- Modify: `src/main.c:416-552`

- [ ] **Step 1: Add declarations + REACH_RADIUS to interact.h**

In `src/game/interact.h`, after the existing function declarations (before `#endif`), add:

```c
#define REACH_RADIUS 6

#include "../engine/input.h"
#include "../engine/camera.h"
#include "inventory.h"
#include "../engine/ui.h"
#include "../world/items.h"

int  interact_handle_click(World *w, Player *p, Inventory *inv, int hotbar_sel,
                           Input *in, Camera *cam,
                           int *sign_x, int *sign_y);
void interact_handle_break(World *w, Player *p, Inventory *inv, int hotbar_sel,
                           Input *in, Camera *cam, float dt);
void interact_handle_place(World *w, Player *p, Inventory *inv, int hotbar_sel,
                           Input *in, Camera *cam, UI *ui,
                           int *portal_pending, int *portal_x, int *portal_y);
```

Note: the includes go in the header so the signatures resolve. These are all leaf modules — no cycle.

- [ ] **Step 2: Add includes to interact.c**

In `src/game/interact.c`, after the existing includes, add:

```c
#include "../engine/input.h"
#include "../engine/camera.h"
#include "inventory.h"
#include "../engine/ui.h"
#include "../world/items.h"
#include "crafting.h"
#include "farming.h"
#include <stdlib.h>
```

Note: `<stdlib.h>` is needed for `rand()` in the gem-drop code (will be removed in Task 9 when RNG is unified).

- [ ] **Step 3: Add `interact_handle_click` to interact.c**

At the end of `src/game/interact.c`, add:

```c
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
```

- [ ] **Step 4: Add `interact_handle_break` to interact.c**

After `interact_handle_click`, add:

```c
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
```

- [ ] **Step 5: Add `interact_handle_place` to interact.c**

After `interact_handle_break`, add:

```c
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
```

- [ ] **Step 6: Replace the interaction block in main.c**

In `src/main.c`, find the code starting at the `if (input_is_mouse_clicked(&g->input, 1))` block (the punch/break/goto section) through the end of the place section. This spans from the line after `farming_update(&g->world, dt);` to just before the hotbar key loop `for (int i = SDL_SCANCODE_1; ...)`.

Replace the entire interaction block (punch click, break hold, `skip_break:` label, place RMB — approximately lines 416-552) with:

```c
    {
        int sign_x, sign_y;
        int click_result = interact_handle_click(&g->world, &g->player,
            &g->inventory, g->ui.hotbar_selection, &g->input, &g->camera,
            &sign_x, &sign_y);
        if (click_result == 2) {
            g->sign_overlay_active = 1;
            g->sign_overlay_timer = 3.0f;
            g->sign_overlay_x = sign_x;
            g->sign_overlay_y = sign_y;
        }
        if (!click_result) {
            interact_handle_break(&g->world, &g->player, &g->inventory,
                g->ui.hotbar_selection, &g->input, &g->camera, dt);
        }
    }

    interact_handle_place(&g->world, &g->player, &g->inventory,
        g->ui.hotbar_selection, &g->input, &g->camera, &g->ui,
        &g->portal_link_pending, &g->portal_link_x, &g->portal_link_y);
```

This eliminates the `goto skip_break` and the `skip_break:` label entirely.

- [ ] **Step 7: Build and verify**

Run: `make clean && make`
Expected: Compiles with zero warnings. If you get "unused variable" warnings for `px`/`py` that were previously used by the interaction code, ensure `px`/`py` are still needed for the growth tick call above (they are — `farming_tick_nearby` uses them).

- [ ] **Step 8: Commit**

```bash
git add src/game/interact.h src/game/interact.c src/main.c
git commit -m "refactor: extract interaction handlers, eliminate goto skip_break"
```

---

## Task 6: Extract `store_handle_click` + Centralize Store Layout

**Files:**
- Modify: `src/game/store.h:1-32`
- Modify: `src/game/store.c`
- Modify: `src/engine/ui.c` (align store render to shared constants)
- Modify: `src/main.c:256-286`

- [ ] **Step 1: Add constants + declaration to store.h**

In `src/game/store.h`, after the existing `#define`s (after `STORE_CAT_COUNT 5`), add:

```c
#define STORE_COLS    5
#define STORE_CELL_W  100
#define STORE_CELL_H  80
```

After the `#include <stdint.h>` at the top, add:

```c
#include "../engine/input.h"
#include "../game/inventory.h"
```

After the existing function declarations (before `#endif`), add:

```c
int store_handle_click(Store *s, Input *in, int *gems,
                       Inventory *inv, int screen_w, int category);
```

- [ ] **Step 2: Add `store_handle_click` to store.c**

In `src/game/store.c`, after the existing `store_init` function, add:

```c
int store_handle_click(Store *s, Input *in, int *gems,
                       Inventory *inv, int screen_w, int category)
{
    if (!input_is_mouse_clicked(in, 1))
        return 0;

    StoreEntry *entries;
    int entry_count;
    store_get_items(s, category, &entries, &entry_count);

    int panel_w = STORE_COLS * STORE_CELL_W + 40;
    int items_x = (screen_w - panel_w) / 2 + 20;
    int tabs_y = 60 + 36;
    int items_y = tabs_y + 28 + 16;

    for (int i = 0; i < entry_count; i++) {
        int col = i % STORE_COLS;
        int row = i / STORE_COLS;
        int cx = items_x + col * STORE_CELL_W;
        int cy = items_y + row * STORE_CELL_H;

        if (in->mouse_x >= cx && in->mouse_x < cx + 48 &&
            in->mouse_y >= cy && in->mouse_y < cy + 48) {
            if (*gems >= entries[i].price) {
                *gems -= entries[i].price;
                inventory_add(inv, entries[i].item_id, 1);
                return 1;
            }
            return 0;
        }
    }
    return 0;
}
```

- [ ] **Step 3: Align ui.c store rendering to shared constants**

In `src/engine/ui.c`, remove the local `#define STORE_COLS 5` (line 14). Add `#include "../game/store.h"` to ui.c's includes. Then in `ui_render_store_screen`, replace the local `cell_w`/`cell_h` with `STORE_CELL_W`/`STORE_CELL_H` and `STORE_COLS` usage.

Specifically, find in `ui_render_store_screen`:

```c
        int cell_w = 100;
        int cell_h = 80;
```

and replace with:

```c
        int cell_w = STORE_CELL_W;
        int cell_h = STORE_CELL_H;
```

The `STORE_COLS` references in `ui_render_store_screen` will now resolve from `store.h` instead of the deleted local `#define`.

- [ ] **Step 4: Replace inline store click in main.c**

In `src/main.c`, find the store purchase block (inside `game_update`, under `if (g->ui.state == UI_STATE_STORE && input_is_mouse_clicked(...))`). Replace the entire block (the throwaway `Store store; store_init(&store); ...` through the closing brace of the for-loop) with:

```c
        if (g->ui.state == UI_STATE_STORE && input_is_mouse_clicked(&g->input, 1)) {
            store_handle_click(&g->store, &g->input, &g->player.gems,
                &g->inventory, g_screen_w, g->ui.store_category);
        }
```

- [ ] **Step 5: Build and verify**

Run: `make clean && make`
Expected: Compiles with zero warnings.

- [ ] **Step 6: Commit**

```bash
git add src/game/store.h src/game/store.c src/engine/ui.c src/main.c
git commit -m "refactor: extract store_handle_click, centralize layout constants"
```

---

## Task 7: Extract `ui_handle_sign_edit_event` + `ui_render_exit_confirm`

**Files:**
- Modify: `src/engine/ui.h:57-77`
- Modify: `src/engine/ui.c`
- Modify: `src/main.c:218-232` and `src/main.c:722-736`

- [ ] **Step 1: Add declarations to ui.h**

In `src/engine/ui.h`, before the `#endif`, add:

```c
void ui_handle_sign_edit_event(UI *ui, const SDL_Event *e);
void ui_render_exit_confirm(UI *ui, Renderer *renderer);
```

- [ ] **Step 2: Add `ui_handle_sign_edit_event` to ui.c**

At the end of `src/engine/ui.c`, add:

```c
void ui_handle_sign_edit_event(UI *ui, const SDL_Event *e)
{
    if (ui->state != UI_STATE_SIGN_EDIT)
        return;

    if (e->type == SDL_TEXTINPUT) {
        if (ui->sign_edit_cursor < SIGN_TEXT_MAX_LEN) {
            int len = (int)strlen(e->text.text);
            if (len > 0 && ui->sign_edit_cursor + len <= SIGN_TEXT_MAX_LEN) {
                ui->sign_edit_text[ui->sign_edit_cursor] = e->text.text[0];
                ui->sign_edit_cursor++;
            }
        }
    }

    if (e->type == SDL_KEYDOWN) {
        if (e->key.keysym.sym == SDLK_BACKSPACE && ui->sign_edit_cursor > 0) {
            ui->sign_edit_cursor--;
            ui->sign_edit_text[ui->sign_edit_cursor] = '\0';
        }
    }
}
```

- [ ] **Step 3: Add `ui_render_exit_confirm` to ui.c**

After `ui_handle_sign_edit_event`, add:

```c
void ui_render_exit_confirm(UI *ui, Renderer *renderer)
{
    (void)ui;
    int dw = 260;
    int dh = 60;
    int dx = (g_screen_w - dw) / 2;
    int dy = (g_screen_h - dh) / 2;
    renderer_draw_rect(renderer, dx, dy, dw, dh, 0.0f, 0.0f, 0.0f, 0.9f);
    renderer_draw_rect(renderer, dx, dy, dw, 1, 0.4f, 0.4f, 0.4f, 1.0f);
    renderer_draw_rect(renderer, dx, dy + dh - 1, dw, 1, 0.0f, 0.0f, 0.0f, 1.0f);
    renderer_draw_rect(renderer, dx, dy, 1, dh, 0.4f, 0.4f, 0.4f, 1.0f);
    renderer_draw_rect(renderer, dx + dw - 1, dy, 1, dh, 0.0f, 0.0f, 0.0f, 1.0f);
    int qtw = renderer_text_width(renderer, "EXIT WORLD?", 2.0f);
    renderer_draw_text(renderer, "EXIT WORLD?", dx + (dw - qtw) / 2, dy + 8, 2.0f, 1.0f, 1.0f, 1.0f);
    int htw = renderer_text_width(renderer, "Y/N", 1.5f);
    renderer_draw_text(renderer, "Y/N", dx + (dw - htw) / 2, dy + 34, 1.5f, 0.7f, 0.7f, 0.7f, 1.0f);
}
```

- [ ] **Step 4: Replace inline sign-edit text handling in main.c**

In `src/main.c`, inside `game_handle_events`, find the two blocks handling sign-edit text input (SDL_TEXTINPUT and SDL_KEYDOWN backspace, approximately lines 218-232). Replace both blocks with a single call:

```c
            ui_handle_sign_edit_event(&g->ui, &e);
```

This single line replaces the `SDL_TEXTINPUT` block, the `SDL_KEYDOWN` backspace block, and the surrounding conditionals. Place it inside the `if (g_game_state == GAME_STATE_PLAYING)` block in `game_handle_events`, after the existing `input_handle_event(&g->input, &e);` call.

- [ ] **Step 5: Replace inline exit-confirm rendering in main.c**

In `src/main.c`, inside `game_render`, find the exit-confirm rendering block (approximately lines 722-736, the `if (g->exit_confirm_active) { ... }` block). Replace it with:

```c
    if (g->exit_confirm_active) {
        ui_render_exit_confirm(&g->ui, &g->renderer);
    }
```

- [ ] **Step 6: Build and verify**

Run: `make clean && make`
Expected: Compiles with zero warnings.

- [ ] **Step 7: Commit**

```bash
git add src/engine/ui.h src/engine/ui.c src/main.c
git commit -m "refactor: extract sign-edit event handler and exit-confirm rendering"
```

---

## Task 8: Extract `renderer_draw_world` + `renderer_draw_player`

**Files:**
- Modify: `src/engine/renderer.h:49`
- Modify: `src/engine/renderer.c:1-8`
- Modify: `src/main.c:595-673`

- [ ] **Step 1: Add declarations to renderer.h**

In `src/engine/renderer.h`, before the `#endif`, add:

```c
#include "camera.h"
#include "../world/world.h"
#include "../game/player.h"

void renderer_draw_world(Renderer *r, Camera *cam, World *w);
void renderer_draw_player(Renderer *r, Camera *cam, Player *p, World *w);
```

- [ ] **Step 2: Add includes to renderer.c**

In `src/engine/renderer.c`, after the existing includes, add:

```c
#include "../world/block.h"
```

- [ ] **Step 3: Add `renderer_draw_world` to renderer.c**

At the end of `src/engine/renderer.c`, add:

```c
void renderer_draw_world(Renderer *r, Camera *cam, World *w)
{
    float visible_w = g_screen_w / cam->zoom;
    float visible_h = g_screen_h / cam->zoom;
    float cam_left = cam->x - visible_w / 2.0f;
    float cam_top = cam->y - visible_h / 2.0f;
    int start_x = (int)(cam_left / TILE_SIZE) - 1;
    int start_y = (int)(cam_top / TILE_SIZE) - 1;
    int end_x = start_x + (int)(visible_w / TILE_SIZE) + 3;
    int end_y = start_y + (int)(visible_h / TILE_SIZE) + 3;

    if (start_x < 0) start_x = 0;
    if (start_y < 0) start_y = 0;
    if (end_x > w->width) end_x = w->width;
    if (end_y > w->height) end_y = w->height;

    for (int y = start_y; y < end_y; y++) {
        for (int x = start_x; x < end_x; x++) {
            Tile *t = world_get_tile(w, x, y);
            if (!t) continue;

            int sx, sy;
            camera_world_to_screen(cam, x * TILE_SIZE, y * TILE_SIZE, &sx, &sy);

            if (t->bg != BLOCK_AIR) {
                int sprite = block_get_sprite(t->bg);
                renderer_draw_tile(r, sx, sy, sprite, 0);
            }

            if (t->fg != BLOCK_AIR) {
                int sprite = block_get_sprite(t->fg);
                if (t->growth_stage > 0 && t->growth_stage < GROWTH_COMPLETE) {
                    int dirt_sprite = block_get_sprite(BLOCK_DIRT);
                    renderer_draw_tile(r, sx, sy, dirt_sprite, 0);
                    float height_factor = 0.3f + 0.7f * (t->growth_stage / (float)GROWTH_COMPLETE);
                    int draw_h = (int)(TILE_SIZE * height_factor);
                    renderer_draw_tile_scaled(r, sx, sy + TILE_SIZE - draw_h,
                        TILE_SIZE, draw_h, sprite, 0);
                } else if (t->growth_stage >= GROWTH_COMPLETE) {
                    renderer_draw_tile(r, sx, sy, sprite, 0);
                    int leaf_sprite = block_get_sprite(BLOCK_LEAVES);
                    renderer_draw_tile_scaled(r, sx - 4, sy - 12,
                        TILE_SIZE + 8, TILE_SIZE / 2 + 12, leaf_sprite, 0);
                    renderer_draw_tile_border(r, sx, sy);
                } else {
                    renderer_draw_tile(r, sx, sy, sprite, 0);
                    renderer_draw_tile_border(r, sx, sy);
                    if (t->fg == BLOCK_PORTAL) {
                        float pulse = 0.5f + 0.5f * sinf((float)SDL_GetTicks() / 300.0f);
                        renderer_draw_rect(r, sx, sy, TILE_SIZE, TILE_SIZE,
                            0.6f * pulse, 0.2f * pulse, 0.9f * pulse, 0.3f);
                    }
                }
            }
        }
    }
}
```

- [ ] **Step 4: Add `renderer_draw_player` to renderer.c**

After `renderer_draw_world`, add:

```c
void renderer_draw_player(Renderer *r, Camera *cam, Player *p, World *w)
{
    int psx, psy;
    camera_world_to_screen(cam, p->x - PLAYER_WIDTH / 2, p->y - PLAYER_HEIGHT, &psx, &psy);
    renderer_draw_rect(r, psx, psy, PLAYER_WIDTH, PLAYER_HEIGHT,
        1.0f, 0.8f, 0.6f, 1.0f);
    renderer_draw_rect(r, psx + 4, psy + 4, 6, 6, 0.0f, 0.0f, 0.0f, 1.0f);
    renderer_draw_rect(r, psx + 14, psy + 4, 6, 6, 0.0f, 0.0f, 0.0f, 1.0f);

    if (p->breaking) {
        int bsx, bsy;
        camera_world_to_screen(cam, p->break_x * TILE_SIZE, p->break_y * TILE_SIZE, &bsx, &bsy);
        Tile *bt = world_get_tile(w, p->break_x, p->break_y);
        if (bt) {
            int break_time = block_get_break_time(bt->fg);
            float progress = (break_time > 0) ? (float)p->break_timer / break_time : 0.0f;
            renderer_draw_rect(r, bsx, bsy, TILE_SIZE, TILE_SIZE,
                1.0f, 1.0f, 1.0f, progress * 0.5f);
        }
    }
}
```

- [ ] **Step 5: Replace inline rendering in main.c**

In `src/main.c`, inside `game_render`, find the block that computes visible bounds and iterates tiles (the camera bounds computation + tile loop + player/break rendering, approximately lines 595-673). This is everything between `renderer_begin_tile_batch(&g->renderer, g->camera.zoom);` and `renderer_end_tile_batch(&g->renderer);`.

Replace that entire block with:

```c
    renderer_begin_tile_batch(&g->renderer, g->camera.zoom);

    renderer_draw_world(&g->renderer, &g->camera, &g->world);
    renderer_draw_player(&g->renderer, &g->camera, &g->player, &g->world);

    renderer_end_tile_batch(&g->renderer);
```

- [ ] **Step 6: Build and verify**

Run: `make clean && make`
Expected: Compiles with zero warnings.

- [ ] **Step 7: Commit**

```bash
git add src/engine/renderer.h src/engine/renderer.c src/main.c
git commit -m "refactor: extract renderer_draw_world and renderer_draw_player"
```

---

## Task 9: RNG Unification

**Files:**
- Modify: `src/world/world.h:4` and `World` struct
- Modify: `src/world/world.c` (world_init seed + world_generate)
- Modify: `src/game/interact.c` (gem drop)

- [ ] **Step 1: Add Prng field to World struct**

In `src/world/world.h`, after `#include <stdint.h>`, add:

```c
#include "../engine/prng.h"
```

In the `World` struct, add a `Prng rng;` field (after `float spawn_y;`):

```c
    Prng rng;
```

- [ ] **Step 2: Seed rng in world_init**

In `src/world/world.c`, inside `world_init`, after the existing initialization (before `return 0;`), add:

```c
    prng_seed(&w->rng, (uint32_t)time(NULL));
```

- [ ] **Step 3: Replace srand/rand in world_generate**

In `src/world/world.c`, inside `world_generate`:

Delete the `srand((unsigned)time(NULL));` line (line 32).

Replace each `rand()` call with `prng_*` using the World's rng. The full replacements:

Replace `if (rand() % 100 < 15)` with `if (prng_float(&w->rng) < 0.15f)`

Replace `if (rand() % 100 < 10)` with `if (prng_float(&w->rng) < 0.10f)`

Replace `int num_caves = 30 + rand() % 25;` with `int num_caves = prng_range(&w->rng, 30, 54);`

Replace `int cx = rand() % w->width;` with `int cx = prng_range(&w->rng, 0, w->width - 1);`

Replace `int cy = 28 + rand() % 29;` with `int cy = prng_range(&w->rng, 28, 56);`

Replace `int radius = 2 + rand() % 4;` with `int radius = prng_range(&w->rng, 2, 5);`

Replace `if (rand() % 100 < 8)` with `if (prng_float(&w->rng) < 0.08f)`

Replace `int trunk_h = 4 + rand() % 3;` with `int trunk_h = prng_range(&w->rng, 4, 6);`

Replace `x += 5 + rand() % 4;` with `x += prng_range(&w->rng, 5, 8);`

- [ ] **Step 4: Replace rand in gem drop (interact.c)**

In `src/game/interact.c`, inside `interact_handle_break`, replace:

```c
                int gem_drop = 1 + (rand() % 3);
```

with:

```c
                int gem_drop = prng_range(&w->rng, 1, 3);
```

Also remove `#include <stdlib.h>` from interact.c (no longer needed — `prng.h` provides the RNG, and it's transitively included via `world.h` → `prng.h`).

- [ ] **Step 5: Build and verify**

Run: `make clean && make`
Expected: Compiles with zero warnings. Verify no remaining `rand()` or `srand()` calls in `src/` (excluding `stb_image.h`):

```bash
grep -rn 'rand()' src/ --include='*.c' | grep -v stb_image
```

Expected: no output.

- [ ] **Step 6: Commit**

```bash
git add src/world/world.h src/world/world.c src/game/interact.c
git commit -m "refactor: unify RNG onto prng module, world owns its Prng"
```

---

## Task 10: Remove Dead Code (`inventory_save` / `inventory_load`)

**Files:**
- Modify: `src/game/inventory.h:23-24`
- Modify: `src/game/inventory.c:93-120`

- [ ] **Step 1: Remove declarations from inventory.h**

In `src/game/inventory.h`, delete these two lines:

```c
int inventory_save(Inventory *inv, const char *path);
int inventory_load(Inventory *inv, const char *path);
```

- [ ] **Step 2: Remove implementations from inventory.c**

In `src/game/inventory.c`, delete the `inventory_save` function (the one without `_profile`, approximately lines 93-104) and the `inventory_load` function (the one without `_profile`, approximately lines 106-120). Do NOT delete `inventory_save_profile` or `inventory_load_profile`.

- [ ] **Step 3: Build and verify**

Run: `make clean && make`
Expected: Compiles with zero warnings.

- [ ] **Step 4: Commit**

```bash
git add src/game/inventory.h src/game/inventory.c
git commit -m "refactor: remove dead inventory_save/inventory_load functions"
```

---

## Final Verification

After all 10 tasks are complete:

- [ ] **Step 1: Full clean build**

Run: `make clean && make`
Expected: Zero warnings.

- [ ] **Step 2: Check main.c line count**

Run: `wc -l src/main.c`
Expected: ~300 lines (down from 780).

- [ ] **Step 3: Verify no rand/srand remains**

Run: `grep -rn 'srand\|rand()' src/ --include='*.c' | grep -v stb_image`
Expected: no output.

- [ ] **Step 4: Verify no goto remains**

Run: `grep -n 'goto' src/main.c`
Expected: no output.

- [ ] **Step 5: Manual playtest**

Launch the game (`make run`) and verify:
- Walk, jump, collide with walls/floor
- Break dirt/stone with and without pickaxe; drops appear, break progress bar shows
- Punch door (toggles), sign (shows text), portal (teleports)
- Place blocks, plant seeds, splice a growing plant
- Wrench: link two portals, edit a sign
- Buy from store in each category
- Sign edit: type, backspace, Enter to save
- Wait for a seed to grow a stage
- Visually confirm: tile rendering, growth stages, portal pulse, player sprite, exit-confirm dialog
- Press ESC → exit confirm → Y exits to menu

- [ ] **Step 6: Save compatibility**

Load an existing world (enter a world name you've played before). Verify it loads and your inventory/gems are preserved.
