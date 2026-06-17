# Explosive (Throwable Bomb) Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a throwable Bomb consumable that the player buys from the store, lobs in a gravity arc toward the cursor on right-click, and detonates on impact — destroying a circular area of foreground blocks and damaging/knocking back the player.

**Architecture:** New focused `src/game/explosive.{c,h}` module mirrors `lava.c`'s pattern: file-local state for the one active projectile + flash (single-player assumption, no struct/save changes). `explosive_throw()` is routed from `interact_handle_place` (before the reach check, so throws bypass reach). `explosive_update()` (physics + impact + blast) runs after `lava_update`; `explosive_render()` runs inside the tile batch after `renderer_draw_break_progress`, using the existing `camera_world_to_screen` + `renderer_draw_rect` path proven by `renderer_draw_player`. No renderer or save-format changes.

**Tech Stack:** C11, SDL2, OpenGL (fixed-function). Build with `make clean && make` (must compile with zero warnings under `-Wall -Wextra`). No test framework; verification is build + manual playtest.

**Spec:** `docs/superpowers/specs/2026-06-17-explosive-design.md`

---

## File Structure

| File | Status | Responsibility |
|---|---|---|
| `src/game/explosive.h` | NEW | Constants + public API (`explosive_throw/update/render`) |
| `src/game/explosive.c` | NEW | File-local `s_bomb`/`s_flash` state, `is_bomb_spare()`, throw physics, impact detection, blast (destruction + damage + knockback), flash, render |
| `src/world/items.h` | MODIFY | Add `#define ITEM_BOMB 9200` |
| `src/world/items.c` | MODIFY | Bump `MISC_ITEM_COUNT` 7→8; append positional Bomb entry to `ITEM_DEFS[]` |
| `src/game/store.c` | MODIFY | `cat_add(cat, ITEM_BOMB, 50)` in `STORE_CAT_SPECIAL` |
| `src/game/interact.c` | MODIFY | Add `#include "explosive.h"`; bomb branch in `interact_handle_place` right after the right-click gate |
| `src/main.c` | MODIFY | Add `#include "game/explosive.h"`; call `explosive_update` after `lava_update`; call `explosive_render` after `renderer_draw_break_progress` |
| `Makefile` | unchanged | `$(wildcard $(SRCDIR)/**/*.c)` already picks up `src/game/explosive.c` automatically — verify `explosive.o` appears in build output, do not edit |

**Build note:** The Makefile uses `$(wildcard $(SRCDIR)/**/*.c)` which already matches `src/game/lava.c`, `src/game/farming.c`, etc. Adding `src/game/explosive.c` requires NO Makefile edit; just confirm `explosive.o` appears under `build/game/` after building.

---

## Task 1: Register the Bomb Item and Sell It in the Store

**Goal:** The Bomb exists as a stackable consumable item the player can buy. No gameplay behavior yet — this establishes the item in the economy so later tasks have something to throw.

**Files:**
- Modify: `src/world/items.h` (add `ITEM_BOMB` define near the other item-id constants)
- Modify: `src/world/items.c` (bump `MISC_ITEM_COUNT`, append positional entry to `ITEM_DEFS[]`)
- Modify: `src/game/store.c` (add to `STORE_CAT_SPECIAL` in `store_init`)

- [ ] **Step 1: Add `ITEM_BOMB` constant to `src/world/items.h`**

In `src/world/items.h`, at the end of the item-id constant block (after `#define ITEM_PANTS 9102`, around line 59), add:

```c
#define ITEM_BOMB   9200
```

This range sits clear of tools (9000s), clothing (9100s), and gems (9999), with room for future consumables.

- [ ] **Step 2: Add the Bomb entry to `ITEM_DEFS[]` in `src/world/items.c`**

The bomb belongs in the **positional** `ITEM_DEFS[]` array (the one holding Fist/Wrench/Pickaxe/Hat/Shirt/Pants/Gems) — NOT in `block_defs[]` or `seed_defs[]`, which use designated initializers keyed by block/seed id and cannot hold id 9200. `item_get_def()` finds these misc items by looping and matching `.id`.

Change the count (around line 51) from:

```c
#define MISC_ITEM_COUNT 7
```

to:

```c
#define MISC_ITEM_COUNT 8
```

Then append a positional entry to the `ITEM_DEFS[]` array (after the Gems entry at line 60, before the closing `};` at line 61):

```c
    {ITEM_BOMB, "Bomb", ITEM_CAT_CONSUMABLE, 50, 50, 25, 0, 40, 40, 40, 0, 0, 0, 0, 0, 0},
```

Field meaning (from `ItemDef` in `items.h`): id=ITEM_BOMB, name="Bomb", category=ITEM_CAT_CONSUMABLE (first item to use this existing enum value), max_stack=50, gem_cost=50, sell_cost=25, sprite_id=0, color 40/40/40 (dark grey), then zeros for `is_seed, seed_grows_into, grow_time_ms, harvest_count, harvest_seed_count, tool_power`. `ITEM_DEF_COUNT` is defined as `MISC_ITEM_COUNT`, so it stays in sync automatically.

- [ ] **Step 3: Add the Bomb to the Store SPECIAL category in `src/game/store.c`**

In `store_init`, inside the `STORE_CAT_SPECIAL` block (right after `cat_add(cat, BLOCK_PORTAL, 500);` at line 65, before the closing `}` of that block), add:

```c
    cat_add(cat, ITEM_BOMB, 50);
```

Price (50) matches `gem_cost` in `items.c`. `STORE_CAT_SPECIAL` is used (not a new category) because the store has exactly 5 fixed-size categories and tab labels rendered from `STORE_CAT_COUNT`; SPECIAL already holds one-off non-block items (Sign/Lock/Door/Store/Mailbox/Portal) and `store_handle_click` does not check `category == block`, so a consumable buys/sells correctly with no further changes.

- [ ] **Step 4: Build and verify zero warnings**

Run:

```bash
make clean && make 2>&1 | tee /tmp/explosive_build_t1.log
```

Expected: `gcc` compiles every file with no warnings/errors under `-Wall -Wextra`, links successfully, produces `./growtopia`. If warnings appear, fix before proceeding.

- [ ] **Step 5: Playtest verification**

Run:

```bash
make run
```

Enter a world. Open the store (place/interact with a Store block, or whatever the existing store-open flow is), click the **SPECIAL** tab, and confirm:
- The "Bomb" item appears in the SPECIAL category list.
- The price shows 50 gems.
- Clicking it with enough gems removes 50 gems and adds 1 Bomb to the inventory.
- The Bomb appears in the inventory/hotbar as a dark grey icon with the name "Bomb" and a stack count.
- Buying several stacks them up to 50 per slot.

Right-clicking with the Bomb selected does nothing yet (no throw logic) — that is expected for this task.

- [ ] **Step 6: Commit**

```bash
git add src/world/items.h src/world/items.c src/game/store.c
git commit -m "Add Bomb consumable item and store entry"
```

---

## Task 2: Explosive Module — Throw, Flight, Impact Flash, Rendering

**Goal:** The player can throw a Bomb. It flies in a gravity arc toward the cursor and produces an expanding flash on impact. No block destruction or player damage yet — this verifies the projectile physics, impact detection, and world-space rendering in isolation. Task 3 adds the destructive blast.

**Files:**
- Create: `src/game/explosive.h`
- Create: `src/game/explosive.c`
- Modify: `src/game/interact.c` (add include; add bomb branch right after the right-click gate in `interact_handle_place`)
- Modify: `src/main.c` (add include; call `explosive_update`; call `explosive_render`)

- [ ] **Step 1: Create `src/game/explosive.h`**

Create the new header with constants and the public API. The signatures reference `Player`, `Camera`, `Input`, `World`, and `Renderer`, so include the headers that declare those types:

```c
#ifndef EXPLOSIVE_H
#define EXPLOSIVE_H

#include "../world/world.h"
#include "../game/player.h"
#include "../engine/camera.h"
#include "../engine/input.h"
#include "../engine/renderer.h"

#define BOMB_THROW_SPEED    350.0f
#define BOMB_GRAVITY        980.0f
#define BOMB_RADIUS_TILES   3
#define BOMB_DAMAGE         50
#define BOMB_KNOCKBACK      420.0f
#define BOMB_MAX_FLIGHT_S   5.0f
#define BOMB_FLASH_S        0.25f

int  explosive_throw(Player *p, Camera *cam, Input *in);
void explosive_update(World *w, Player *p, float dt);
void explosive_render(Renderer *r, Camera *cam);

#endif
```

- [ ] **Step 2: Create `src/game/explosive.c` with state + throw + flight + impact-flash + render**

Create the file. In this task, `detonate()` only spawns the flash and deactivates the bomb (no destruction/damage yet — that is Task 3). The `is_bomb_spare()` helper is deferred to Task 3.

```c
#include "explosive.h"
#include "../world/block.h"
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

static void detonate(World *w, Player *p, int cx, int cy)
{
    (void)w;
    (void)p;
    s_flash.x = (cx + 0.5f) * TILE_SIZE;
    s_flash.y = (cy + 0.5f) * TILE_SIZE;
    s_flash.timer = 0.0f;
    s_flash.active = 1;
}
```

In this task `detonate()` only spawns the flash (the blast center is the impact tile's center). Task 3 will add block destruction and player damage above the flash-spawn lines. The `(void)w; (void)p;` lines suppress `-Wunused-parameter` for now; they are removed in Task 3 when `w` and `p` are used.

Then append the update and render functions:

```c
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
        Tile *t = world_get_tile(w, tx, ty);
        if (t && block_is_solid(t->fg)) detonate_now = 1;
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
```

Note: `TILE_SIZE`, `Renderer`, `world_get_tile`, `block_is_solid`, and `camera_world_to_screen`/`camera_screen_to_world` all come from the includes. `sqrtf` comes from `<math.h>`. The Makefile links `-lm` (already in `LDFLAGS`).

- [ ] **Step 3: Wire the throw into `src/game/interact.c`**

Add the include. At the top of `src/game/interact.c`, after `#include "farming.h"` (line 11), add:

```c
#include "explosive.h"
```

Add the bomb branch. In `interact_handle_place` (starts at line 264), **immediately after the right-click gate** — i.e. after:

```c
    if (!input_is_mouse_clicked(in, 3))
        return;
```

(lines 268–269) and **before** the existing `int mouse_wx, mouse_wy;` declaration (line 271) — insert:

```c
    {
        uint16_t held_bomb = inventory_get_hotbar_item(inv, hotbar_sel);
        if (held_bomb == ITEM_BOMB) {
            if (explosive_throw(p, cam, in))
                inventory_remove(inv, ITEM_BOMB, 1);
            return;
        }
    }
```

A block scope (`{ ... }`) is used so the local `held_bomb` does not conflict with the `held` variable declared later in the function (~line 288). This placement is BEFORE the `REACH_RADIUS` check (~line 281) and the null-tile check (~line 285), so throws correctly bypass reach and can be aimed at the open sky. `explosive_throw` re-derives the cursor world position itself (it needs pixel world coords; the later `mouse_wx/mouse_wy` are tile-divided).

- [ ] **Step 4: Wire update + render into `src/main.c`**

Add the include. After `#include "game/lava.h"` (line 23), add:

```c
#include "game/explosive.h"
```

Add the update call. In `game_update`, after `lava_update(&g->world, &g->player, dt);` (line 338), add:

```c
    explosive_update(&g->world, &g->player, dt);
```

Add the render call. In `game_render`, after `renderer_draw_break_progress(&g->renderer, &g->world, &g->player, &g->camera);` (line 413) and before `renderer_end_tile_batch(&g->renderer);` (line 415), add:

```c
    explosive_render(&g->renderer, &g->camera);
```

- [ ] **Step 5: Build and verify zero warnings**

Run:

```bash
make clean && make 2>&1 | tee /tmp/explosive_build_t2.log
```

Expected: clean compile, no warnings under `-Wall -Wextra`. Confirm `build/game/explosive.o` was built (the Makefile wildcard picks it up automatically). The `(void)w; (void)p;` lines in the `detonate` stub are what keep `-Wunused-parameter` quiet until Task 3 uses those parameters.

- [ ] **Step 6: Playtest verification**

Run:

```bash
make run
```

Enter a world. Buy a Bomb from the store SPECIAL tab, place it on your hotbar, select it. Then:
- **Right-click** toward a target. A small dark grey square should launch from the player's mid-torso and arc downward under gravity toward the cursor.
- Aiming higher sends it farther; aiming at the ground drops it short. Confirms the aim vector + fixed-speed lob.
- When the bomb hits a solid block, the world edge, or after ~5 seconds of flight, an **expanding orange-yellow square flash** appears at the impact point and fades out over ~0.25s. Confirms impact detection + flash render.
- Each throw consumes 1 Bomb from the stack.
- Throwing while a flash is still visible does nothing (the throw is blocked until the flash finishes) — confirms the single-projectile gate.
- The bomb does NOT destroy blocks and does NOT damage the player yet — that is Task 3.

- [ ] **Step 7: Commit**

```bash
git add src/game/explosive.h src/game/explosive.c src/game/interact.c src/main.c
git commit -m "Add explosive throw, flight, impact flash, and rendering"
```

---

## Task 3: Destructive Detonation (Block Destruction + Player Damage + Knockback)

**Goal:** On impact the bomb now destroys foreground blocks in a circular radius (sparing Bedrock and special/interactive blocks) and damages + knocks back the player if they are inside the blast. This completes the feature.

**Files:**
- Modify: `src/game/explosive.c` (add `is_bomb_spare()` helper; flesh out `detonate()` with destruction + damage + knockback before the flash spawn)

- [ ] **Step 1: Add the `is_bomb_spare()` helper to `src/game/explosive.c`**

Add this file-local predicate just above the existing `detonate()` function (it needs the block-id macros from `world.h`, already included via `explosive.h`, and `block_is_interactive` from `block.h`, already included):

```c
static int is_bomb_spare(uint16_t fg)
{
    return fg == BLOCK_BEDROCK
        || fg == BLOCK_STORE
        || fg == BLOCK_LOCK
        || fg == BLOCK_MAILBOX
        || fg == BLOCK_TREE_CARCASS
        || block_is_interactive(fg);
}
```

Why an explicit check: the existing `block_is_interactive()` (in `block.c`) returns true only for `BLOCK_DOOR`, `BLOCK_SIGN`, and `BLOCK_PORTAL`. The blast must additionally spare `BLOCK_STORE` (costs 1000 gems!), `BLOCK_LOCK`, `BLOCK_MAILBOX`, and `BLOCK_TREE_CARCASS`, so these are listed explicitly.

- [ ] **Step 2: Replace `detonate()` with the full destructive version**

Replace the entire current `detonate()` function (the Task 2 stub) with:

```c
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
```

`interact_cleanup_break` is declared in `interact.h`. Add that include at the top of `src/game/explosive.c`, after `#include "../world/block.h"`:

```c
#include "../game/interact.h"
```

`MAX_HEALTH`, `PLAYER_HEIGHT` come from `player.h` (already included via `explosive.h`). The knockback is a one-shot velocity set (same pattern as `lava_update`'s bounce), so it will not fight collision resolution.

- [ ] **Step 3: Build and verify zero warnings**

Run:

```bash
make clean && make 2>&1 | tee /tmp/explosive_build_t3.log
```

Expected: clean compile under `-Wall -Wextra`, successful link, `./growtopia` produced.

- [ ] **Step 4: Playtest verification**

Run:

```bash
make run
```

Enter a world with some terrain. Buy Bombs, select one on the hotbar, and verify:

**Block destruction:**
- Throw a bomb at a wall / into the ground. A roughly circular crater (~3-tile radius) of foreground blocks is cleared where it detonates.
- Bedrock (world bottom) is NOT destroyed by a bomb landing on it — the crater stops at bedrock.
- Place a Door, Sign, Portal, Store, Lock, or Mailbox (buy from the store) and detonate a bomb on it — these blocks survive (the blast spares them).
- Background tiles (e.g. placed Wood Background) are NOT removed — only foreground.
- Destroyed blocks yield NO item drops (the crater is empty, nothing appears in inventory).

**Player damage + knockback:**
- Stand near (within ~3 tiles of) an impact. You take 50 damage (watch the health bar) and get flung away from the blast center — closer to center = harder knockback; at the edge you still get a small kick.
- Detonating a bomb while standing exactly on it launches you straight up.
- If a blast reduces your health to 0, you respawn at world spawn with full health and zero velocity (same behavior as dying in lava). Console prints "Player killed by bomb, respawning."

**Regression checks:**
- Lava still damages/bounces as before.
- Mining, placing, farming, portals, signs, and the store still work.
- Save and reload (`make run` again, re-enter the world): the bomb item count persists (it is saved with the inventory profile). The crater persists (it is saved with the world tiles). No projectile/flash state is saved (correct — they are transient).

- [ ] **Step 5: Commit**

```bash
git add src/game/explosive.c
git commit -m "Add destructive bomb blast: block destruction, damage, knockback"
```

---

## Verification Summary (AGENTS.md Code Review Workflow)

Before presenting the feature as complete, run the full review:

1. **Build clean:** `make clean && make` — zero warnings under `-Wall -Wextra`.
2. **Convention check (from AGENTS.md):**
   - No comments in code (the plan's code blocks contain no `//` or `/* */` comments — confirm none were added).
   - Headers use `#ifndef` include guards (`explosive.h` does).
   - Functions prefixed by module name (`explosive_*`).
   - `snake_case` throughout.
   - No hardcoded `SCREEN_WIDTH`/`SCREEN_HEIGHT` — uses `g_screen_w`/`g_screen_h` (the explosive code does not reference screen size directly; it uses the camera transforms).
   - UI click detection vs rendering positions — N/A (no new UI slots added; the bomb reuses existing store/inventory slots).
   - Font scales readable — N/A (no new text rendered).
   - Collision/physics don't break grounding — knockback is a one-shot velocity set, mirroring `lava_update`'s proven bounce.
   - Save/load compatibility — no struct changes; v3 format unchanged (verified by the regression reload in Task 3 Step 4).
3. **Full playtest** as described in Task 3 Step 4.
