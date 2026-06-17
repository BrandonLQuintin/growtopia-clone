# Explosive (Throwable Bomb)

## Summary

Add a throwable consumable, the **Bomb**, to the game. The player buys it from the store, selects it on the hotbar, and right-clicks to lob it in a gravity arc toward the cursor. It detonates on impact with any solid block or world edge, destroying a circular area of foreground blocks (sparing Bedrock and special/interactive blocks) and damaging/knocking back the player if caught in the blast. This is the first throwable item in the game, so it introduces a small projectile system.

## Requirements

- Bomb is a new inventory item: id `ITEM_BOMB` (9200), category `ITEM_CAT_CONSUMABLE` (first use of this category)
- Bomb is stackable up to 50, bought from the store (SPECIAL category) for 50 gems, sells back for 25
- Right-click while holding a Bomb lobs it from the player toward the cursor at `BOMB_THROW_SPEED` in an arc under gravity
- Only one bomb may be in flight (or flashing) at a time; throwing while one is active is ignored
- Flight uses simple Euler integration with `BOMB_GRAVITY` (matches `PLAYER_GRAVITY`); no tunneling at normal frame rates (per-frame step ~6px ≪ 32px tile). At the main-loop `dt` cap of 0.1s the step could exceed one tile, matching the existing player-physics limitation (no sub-stepping) — acceptable and consistent.
- The bomb detonates on impact with a solid block, world edge, or after a `BOMB_MAX_FLIGHT_S` safety timeout (air-burst)
- Detonation destroys all foreground blocks within a circular radius (`BOMB_RADIUS_TILES`), except `BLOCK_BEDROCK` and special/interactive blocks (Door, Sign, Portal, Store, Lock, Mailbox, Tree Carcass). These are spared explicitly — the existing `block_is_interactive()` only covers Door/Sign/Portal, so `explosive.c` adds its own spare-set check.
- Detonation applies flat `BOMB_DAMAGE` (50 HP) to the player if they are within the blast radius, with proximity-scaled knockback away from the center; lethal damage respawns the player (same path as lava)
- Destroyed blocks yield no item drops (purely destructive)
- Background tiles are not affected (foreground only)
- An explosion flash (expanding fading square) renders at the blast center for `BOMB_FLASH_S`
- Existing save/load format (v3) is unchanged; the bomb is just an inventory item id and saves/loads via the existing inventory profile save
- Existing single-player assumption is preserved (file-local projectile/flash state, like lava's damage accumulator)

## Architecture

### New module: `src/game/explosive.h` / `explosive.c`

Encapsulates all bomb gameplay logic, mirroring the convention of `lava.c`, `farming.c`, `store.c`.

**Constants (`explosive.h`):**
```c
#define BOMB_THROW_SPEED    350.0f
#define BOMB_GRAVITY        980.0f
#define BOMB_RADIUS_TILES   3
#define BOMB_DAMAGE         50
#define BOMB_KNOCKBACK      420.0f
#define BOMB_MAX_FLIGHT_S   5.0f
#define BOMB_FLASH_S        0.25f
```

**Public functions:**
- `int  explosive_throw(Player *p, Camera *cam, Input *in);` - returns 1 if a bomb was thrown, 0 if blocked (one already active). Called from `interact_handle_place` when the held item is `ITEM_BOMB`.
- `void explosive_update(World *w, Player *p, float dt);` - called from `main.c` once per frame after `lava_update()`. Advances projectile physics, detects impact, runs `detonate()`, and advances the flash timer.
- `void explosive_render(Renderer *r, Camera *cam);` - called from `main.c` inside the tile batch after `renderer_draw_break_progress`. Draws the in-flight bomb and the explosion flash.

**File-local state (`explosive.c`):**
```c
typedef struct {
    float x, y;       /* world-space center, px */
    float vx, vy;
    float flight;     /* seconds since throw */
    int   active;
} Bomb;

typedef struct {
    float x, y;       /* world-space center, px */
    float timer;      /* seconds since detonation */
    int   active;
} Flash;

static Bomb  s_bomb  = {0};
static Flash s_flash = {0};

static int is_bomb_spare(uint16_t fg);   /* defined near detonate(); see below */
```
A single bomb in flight and a single flash, both file-local. Using statics avoids extending the `Player` or `World` structs (which would break save/load compatibility). This explicitly encodes the single-player assumption already present in the codebase, matching `lava.c`'s `s_lava_damage_accum`. `is_bomb_spare` is a file-local predicate used by the blast loop (defined in the detonate section).

### `explosive_throw` behavior

```
if (s_bomb.active || s_flash.active) return 0;

int cur_wx, cur_wy;
camera_screen_to_world(cam, in->mouse_x, in->mouse_y, &cur_wx, &cur_wy);
float cx = p->x;
float cy = p->y - PLAYER_HEIGHT / 2.0f;
float dx = (float)cur_wx - cx;
float dy = (float)cur_wy - cy;
float len = sqrtf(dx*dx + dy*dy);
if (len < 0.001f) { dx = (p->facing_right ? 1.0f : -1.0f); dy = 0.0f; len = 1.0f; }

s_bomb.x  = cx;
s_bomb.y  = cy;
s_bomb.vx = (dx / len) * BOMB_THROW_SPEED;
s_bomb.vy = (dy / len) * BOMB_THROW_SPEED;
s_bomb.flight = 0.0f;
s_bomb.active = 1;
return 1;
```

- Aim direction = normalized (cursor_world − player_center). Player center uses mid-torso (`p->y − PLAYER_HEIGHT/2`).
- The cursor only sets the aim vector; throw speed is fixed, so distance is controlled by aim angle (higher = farther). Skill-expressive, predictable.
- If the cursor is exactly on the player, throw in the facing direction.
- The `REACH_RADIUS` check from placement is NOT applied to throws (the player can lob bombs farther than they can reach to place blocks).

### `explosive_update` behavior

Runs each frame after `lava_update`:

1. **Advance flash timer** (always, so the flash fades even after the projectile is gone):
   ```
   if (s_flash.active) {
       s_flash.timer += dt;
       if (s_flash.timer >= BOMB_FLASH_S) s_flash.active = 0;
   }
   ```
2. **Projectile step** (only if `s_bomb.active`):
   ```
   s_bomb.vy += BOMB_GRAVITY * dt;
   s_bomb.x  += s_bomb.vx * dt;
   s_bomb.y  += s_bomb.vy * dt;
   s_bomb.flight += dt;

   int tx = (int)(s_bomb.x / TILE_SIZE);
   int ty = (int)(s_bomb.y / TILE_SIZE);

   int detonate_now = 0;
   if (s_bomb.flight >= BOMB_MAX_FLIGHT_S) detonate_now = 1;
   else if (tx < 0 || tx >= w->width || ty < 0 || ty >= w->height) detonate_now = 1;
   else {
       Tile *t = world_get_tile(w, tx, ty);
       if (t && block_is_solid(t->fg)) detonate_now = 1;
   }

   if (detonate_now) {
       /* clamp impact tile to in-bounds so the blast circle stays in the world */
       if (tx < 0) tx = 0; if (tx >= w->width) tx = w->width - 1;
       if (ty < 0) ty = 0; if (ty >= w->height) ty = w->height - 1;
       detonate(w, p, tx, ty);
       s_bomb.active = 0;
   }
   ```
- **No tunneling at normal frame rates:** at 350 px/s the per-frame displacement is ~6 px at 60 fps, far below the 32 px tile size. (At the main-loop `dt` cap of 0.1s a fast-falling bomb could step >32px; this matches the existing player-physics limitation and is acceptable.)
- Order: step physics, then test impact against the new position's tile.

### `detonate` (file-local) behavior

Runs once on impact, tile-space `(cx, cy)`:

**1. Block destruction (circular):**
```
int R = BOMB_RADIUS_TILES;
for (int ty = cy - R; ty <= cy + R; ty++) {
    for (int tx = cx - R; tx <= cx + R; tx++) {
        int dx = tx - cx, dy = ty - cy;
        if (dx*dx + dy*dy > R*R + R) continue;   /* +R softens circle edge */
        Tile *t = world_get_tile(w, tx, ty);
        if (!t) continue;
        if (t->fg == BLOCK_AIR) continue;
        if (is_bomb_spare(t->fg)) continue;          /* Bedrock + all special/interactive blocks */
        interact_cleanup_break(w, tx, ty);           /* safe no-op on ordinary tiles */
        t->fg = BLOCK_AIR;
        t->growth_stage = 0;
        t->growth_timer = 0;
        t->extra_data = 0;
    }
}
```
- **Why an explicit check, not `block_is_interactive`:** the existing `block_is_interactive()` (`block.c`) returns true only for `BLOCK_DOOR`, `BLOCK_SIGN`, and `BLOCK_PORTAL`. The blast must also spare `BLOCK_STORE` (1000 gems!), `BLOCK_LOCK`, `BLOCK_MAILBOX`, and `BLOCK_TREE_CARCASS`. So `explosive.c` defines a file-local helper:
  ```c
  static int is_bomb_spare(uint16_t fg) {
      return fg == BLOCK_BEDROCK
          || fg == BLOCK_STORE
          || fg == BLOCK_LOCK
          || fg == BLOCK_MAILBOX
          || fg == BLOCK_TREE_CARCASS
          || block_is_interactive(fg);   /* Door, Sign, Portal */
  }
  ```
  (Requires `#include "../world/block.h"` for `block_is_interactive` and the block id macros, which come transitively via `world.h`.)
- No drops (purely destructive).
- Foreground only; background tiles untouched.
- `interact_cleanup_break` is a safe no-op on non-sign/non-portal tiles and correctly handles a partner portal if a portal were somehow hit; since all interactive blocks are spared, this is defensive.

**2. Player damage + knockback:**
```
float center_x = (cx + 0.5f) * TILE_SIZE;
float center_y = (cy + 0.5f) * TILE_SIZE;
float pdx = p->x - center_x;
float pdy = (p->y - PLAYER_HEIGHT / 2.0f) - center_y;
float dist_sq = pdx*pdx + pdy*pdy;
float radius_px = (BOMB_RADIUS_TILES + 0.5f) * TILE_SIZE;
if (dist_sq <= radius_px * radius_px) {
    p->health -= BOMB_DAMAGE;
    float dist = sqrtf(dist_sq);
    float scale;
    if (dist < 0.001f) {
        p->vx = 0.0f;
        p->vy = -BOMB_KNOCKBACK;        /* standing on the bomb -> straight up */
    } else {
        scale = 1.0f - (dist / radius_px);
        if (scale < 0.2f) scale = 0.2f;  /* minimum kick even at the edge */
        p->vx = (pdx / dist) * BOMB_KNOCKBACK * scale;
        p->vy = (pdy / dist) * BOMB_KNOCKBACK * scale - 80.0f;  /* slight upward bias */
    }
    p->on_ground = 0;
    if (p->health <= 0) {
        p->health = MAX_HEALTH;
        p->x = w->spawn_x; p->y = w->spawn_y;
        p->vx = 0; p->vy = 0;
        printf("Player killed by bomb, respawning.\n");
    }
}
```
- Knockback is a one-shot velocity set (same pattern as `lava_update`'s bounce), so it does not fight collision resolution.
- The `−80.0f` upward bias guarantees the player leaves the ground even on a horizontal hit, selling the "blast" feel.

**3. Spawn flash:**
```
s_flash.x = (cx + 0.5f) * TILE_SIZE;
s_flash.y = (cy + 0.5f) * TILE_SIZE;
s_flash.timer = 0.0f;
s_flash.active = 1;
```

`explosive.c` includes `<math.h>` for `sqrtf` and `<stdio.h>` for the `printf`.

### Items (`src/world/items.h` / `items.c`)

In `items.h`, add to the constants block near the tool/clothing IDs:
```c
#define ITEM_BOMB   9200
```
The 9200 range sits clear of tools (9000s), clothing (9100s), and gems (9999), and reserves room for future consumables.

In `items.c`, the bomb belongs in the **positional** `ITEM_DEFS[]` array (the one holding Fist/Wrench/Pickaxe/Hat/Shirt/Pants/Gems), NOT in `block_defs[]` or `seed_defs[]` (those use designated initializers keyed by block/seed id and cannot hold id 9200). `item_get_def()` finds misc items by looping `ITEM_DEFS` and matching `.id`, so a positional entry is how this array works.

Bump the count and append the entry:
```c
#define MISC_ITEM_COUNT 8              /* was 7 */

const ItemDef ITEM_DEFS[MISC_ITEM_COUNT] = {
    {0,    "Fist",    ...},             /* existing entries unchanged */
    ...
    {9999, "Gems",    ...},
    {ITEM_BOMB, "Bomb", ITEM_CAT_CONSUMABLE, 50, 50, 25,
     0, 40, 40, 40, 0, 0, 0, 0, 0, 0},
};
```
- `category = ITEM_CAT_CONSUMABLE` (first item to use it; the category enum already exists)
- `max_stack = 50`, `gem_cost = 50`, `sell_cost = 25`
- `sprite_id = 0` and `color_r/g/b = 40, 40, 40` (dark grey); the inventory/store icon renders via the existing non-block color-rect path used by tools and clothing (verify during implementation; no new icon code expected)
- Field order matches the `ItemDef` struct in `items.h`: `id, name, category, max_stack, gem_cost, sell_cost, sprite_id, color_r/g/b, is_seed, seed_grows_into, grow_time_ms, harvest_count, harvest_seed_count, tool_power`. `ITEM_DEF_COUNT` is defined as `MISC_ITEM_COUNT`, so it stays in sync automatically.

### Store (`src/game/store.c`)

In `store_init`, inside the `STORE_CAT_SPECIAL` block (which already holds Sign/Lock/Door/Store/Mailbox/Portal), add after the existing entries:
```c
cat_add(cat, ITEM_BOMB, 50);
```
- Price matches `gem_cost` in `items.c`.
- **Decision: use the existing `STORE_CAT_SPECIAL` category** rather than adding a new one. The store has exactly 5 fixed-size categories (`STORE_CAT_COUNT = 5`) and the store tab labels are rendered from that count; adding a 6th would require enum + tab-label changes and risk tab-layout overflow. SPECIAL already contains one-off, non-minable items (sign/lock/door/mailbox/portal/store) and is a natural home for a consumable. `store_handle_click` and `store_buy` do not check `category == block`, so a consumable buys/sells correctly with no further changes.

### Throw wiring (`src/game/interact.c`)

In `interact_handle_place`, add the bomb branch **immediately after the right-click gate** (`if (!input_is_mouse_clicked(in, 3)) return;`, current line ~269) — BEFORE the `REACH_RADIUS` computation and the null-tile check. This placement is required because the spec mandates that throws bypass `REACH_RADIUS` (the player can lob bombs farther than they can reach to place blocks), and the cursor may legitimately aim at the open sky (a tile above the world would yield a null tile and early-return later in the function). Placing the branch after the gate avoids both early-returns.
```c
/* right-click gate already passed above */
if (held == ITEM_BOMB) {            /* re-read held here, or reuse the value computed below */
    if (explosive_throw(p, cam, in))
        inventory_remove(inv, ITEM_BOMB, 1);
    return;
}
```
Because the bomb branch runs before the existing `held`/`held_count` locals are computed (those appear later, ~line 288), the branch should fetch the held item itself:
```c
uint16_t held = inventory_get_hotbar_item(inv, hotbar_sel);
if (held == ITEM_BOMB) {
    if (explosive_throw(p, cam, in))
        inventory_remove(inv, ITEM_BOMB, 1);
    return;
}
```
(The later `held`/`held_count` declarations at ~line 288 are unaffected — shadowing within this early block is avoided by returning immediately. Alternatively hoist the existing `held` declaration above the bomb branch; either is fine. The implementer picks one and ensures `-Wall -Wextra` stays clean.)
- `explosive_throw` re-derives the cursor world position with its own `camera_screen_to_world` call (it needs pixel world coords; the `mouse_wx/mouse_wy` computed later in the function are tile-divided, so it cannot reuse them). This is intentional.
- Uses the same right-click gate already checked at the top of `interact_handle_place`, so no new input binding.
- Returns early so the bomb never falls through to seed/block placement.
- Add `#include "explosive.h"` to `interact.c` (`camera.h`, `input.h`, `inventory.h`, `items.h` are already included).

### Main loop wiring (`src/main.c`)

- After `lava_update(&g->world, &g->player, dt);` (current line ~338), add:
  ```c
  explosive_update(&g->world, &g->player, dt);
  ```
- In `game_render`, after `renderer_draw_break_progress(...)` (current line ~413, inside the tile batch, before `renderer_end_tile_batch`), add:
  ```c
  explosive_render(&g->renderer, &g->camera);
  ```
- Add `#include "game/explosive.h"` near the other game includes.

### Rendering (`explosive_render`)

Draws in world space using the existing primitives, following the **exact same pattern as `renderer_draw_player`**: convert via `camera_world_to_screen` (which returns view-space coordinates — range `0..g_screen_w/zoom` — matching the projection set up by `renderer_begin_tile_batch`), then draw with `renderer_draw_rect`. Sizes are in world pixels (a value of 32 = one tile) and scale with zoom automatically because the tile-batch projection divides by zoom.
```c
void explosive_render(Renderer *r, Camera *cam) {
    if (s_bomb.active) {
        int sx, sy;
        camera_world_to_screen(cam, s_bomb.x, s_bomb.y, &sx, &sy);
        int s = 12;  /* side length in world px (view units); 12px ~ small bomb */
        renderer_draw_rect(r, sx - s/2, sy - s/2, s, s, 0.16f, 0.16f, 0.16f, 1.0f);
    }
    if (s_flash.active) {
        int sx, sy;
        camera_world_to_screen(cam, s_flash.x, s_flash.y, &sx, &sy);
        float p = s_flash.timer / BOMB_FLASH_S;     /* 0 -> 1 */
        int half = (int)(8.0f + ((BOMB_RADIUS_TILES + 1) * TILE_SIZE - 8.0f) * p);
        float a = 1.0f - p;
        renderer_draw_rect(r, sx - half, sy - half, half*2, half*2,
                           1.0f, 0.78f, 0.24f, a);
    }
}
```
- Uses `camera_world_to_screen` + `renderer_draw_rect` — **no renderer.h/.c changes** (no new primitive needed). This is the proven path used by `renderer_draw_player`.
- Bomb in flight: a small dark grey ~12 px square, matching the inventory/store icon color.
- Explosion flash: an expanding square centered on the blast, orange-yellow `(255, 200, 60)`, alpha fading from 1 → 0 over `BOMB_FLASH_S`. A square flash is on-brand for this game's rect-based procedural aesthetic.
- The flash timer is advanced inside `explosive_update` (not in render), so all state advancement stays in the update step and is decoupled from frame-rate variance in rendering.
- `explosive_render` takes no `World *` because it needs only the file-local `s_bomb` / `s_flash` state and the camera transform.

### Build (`Makefile`)

Add `src/game/explosive.o` to the object list.

## Edge cases and compatibility

- **Throw blocked if a bomb/flash is active** — no overlapping explosions, no ambiguous state.
- **Bomb lands on a non-solid tile** — keeps falling under gravity until it hits a solid block, the world edge, or the 5 s safety timeout (air-burst at the current tile).
- **Detonate on world edge** — impact tile is clamped to the nearest in-bounds coordinate before the blast loop, so the destruction circle never reads out of bounds.
- **Player standing on the bomb at impact** — knockback goes straight up (the `dist < 0.001f` branch), guaranteeing escape from a self-stacked hit.
- **Portal / Sign / Door / Store / Lock / Mailbox / Tree Carcass spared** — the blast loop skips tiles via the file-local `is_bomb_spare(t->fg)` (which covers Bedrock, the four special blocks, and `block_is_interactive`'s Door/Sign/Portal). No portal/sign cleanup is triggered; `interact_cleanup_break` is still called defensively on non-spared tiles (a no-op on plain blocks).
- **Collision jitter from knockback** — knockback is a one-shot velocity set identical in spirit to `lava_update`'s bounce, which is already proven safe against the collision resolver in this codebase.
- **Existing saves (no bombs)** — load fine. The bomb is just an inventory item id; the world format (v3) is unchanged and the inventory profile save already stores arbitrary item ids. Existing players simply have no bombs until they buy them.
- **Renderer no-atlas fallback path** — irrelevant: the bomb and flash use `renderer_draw_rect`, which is independent of the tile atlas.
- **Player profile save** — no struct changes to `Player` or `World`. Projectile/flash state is file-local to `explosive.c` and never persisted.

## Files touched

| File | Change |
|---|---|
| `src/game/explosive.h` | NEW - constants + public API |
| `src/game/explosive.c` | NEW - throw, physics, impact, blast, flash, render |
| `src/world/items.h` | `#define ITEM_BOMB 9200` |
| `src/world/items.c` | `[ITEM_BOMB]` positional entry in `ITEM_DEFS[]`; bump `MISC_ITEM_COUNT` 7→8 |
| `src/game/store.c` | `cat_add(ITEM_BOMB, 50)` in `STORE_CAT_SPECIAL` |
| `src/game/interact.c` | `ITEM_BOMB` branch in `interact_handle_place` (right after the right-click gate, before the reach check); include `explosive.h` |
| `src/main.c` | call `explosive_update` (after `lava_update`) + `explosive_render` (after `renderer_draw_break_progress`); include `explosive.h` |
| `Makefile` | unchanged — `$(wildcard $(SRCDIR)/**/*.c)` auto-discovers `src/game/explosive.c` (same as `lava.c`); do not edit |

## Out of scope

- Multiple bomb variants (Bang / Big Ol' Bang / Big Bertha) - one bomb only
- Multiple simultaneous projectiles / rapid-fire throwing - one at a time
- Item drops from destroyed blocks (purely destructive by design)
- Mob/enemy damage (no mobs exist)
- Sound effects
- Destroying background tiles
- Fuse / air-burst / bounce-then-fuse detonation modes (impact-only)
- Crafting recipes for bombs (store-bought only)
- A reusable generic projectile pool / entity system (the focused module can be generalized later if more thrown items are needed; YAGNI)
