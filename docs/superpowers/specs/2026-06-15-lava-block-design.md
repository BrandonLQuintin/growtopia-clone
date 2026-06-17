# Lava Block (Growtopia-style)

## Summary

Add gameplay behavior to the existing `BLOCK_LAVA` (id 13), matching Growtopia: lava is a non-solid animated block that damages players on contact, bounces them upward, and can spawn in caves or be placed/mined by the player. Implement a generalized multi-atlas animation system in the renderer that supports per-frame texture atlases for any block (only lava uses it initially).

## Requirements

- Player takes damage while in contact with lava (20 HP/second, applied continuously via an accumulator)
- Player bounces upward on any contact with lava (top, sides, or bottom) at -550 px/s, once per frame
- Player dies (health <= 0) respawns at world spawn with full health, zero velocity, no item drop
- Lava is non-solid (players pass through it; existing behavior, unchanged)
- Lava is mineable: 1500ms break time, drops 1 Lava block
- Lava is placeable from inventory like any other block
- Lava is sold in the Store (BLOCKS category) for 25 gems and sells back for 12
- Lava spawns naturally: 5-8 sparse puddles per world, deep in the cave layer (y=40-56)
- Lava texture is animated using a multi-frame atlas system (4 frames at 150ms each = 600ms cycle)
- The animation system is generalized: any block can opt in by providing frame-aware texture generation; only lava opts in for now
- All blocks (static + animated) share the same sprite_id slot across all atlas frames; only the bound atlas texture changes per animation frame
- Existing save/load format (v3) is unchanged - lava placement is just `t->fg = BLOCK_LAVA`
- Existing single-player assumption is preserved (one Player, one lava-damage accumulator)

## Architecture

### New module: `src/game/lava.h` / `lava.c`

Encapsulates all gameplay logic for lava, mirroring the convention of `farming.c`, `store.c`, `crafting.c`.

**Constants (`lava.h`):**
```c
#define LAVA_DAMAGE_PER_SEC     20.0f
#define LAVA_BOUNCE_VELOCITY  -550.0f
#define LAVA_POOL_MIN            5
#define LAVA_POOL_MAX            8
#define LAVA_POOL_MAX_TILES     24
#define LAVA_POOL_DEPTH_MIN     40
#define LAVA_POOL_DEPTH_MAX     56
```

**Public functions:**
- `void lava_update(World *w, Player *p, float dt);` - called from `main.c` once per frame after `player_collide()`. Detects player-vs-lava overlap, applies damage, applies bounce, handles death/respawn.
- `void lava_generate_pools(World *w);` - called from `world_generate()` after cave carving. Places sparse lava puddles at cave bottoms.

**File-local state (`lava.c`):**
```c
static float s_lava_damage_accum = 0.0f;
```
A single accumulator for the one Player. Using a static avoids extending the `Player` struct (which would break save/load compatibility for the profile). This explicitly encodes the single-player assumption already present in the codebase.

### `lava_update` behavior

Runs each frame after `player_collide`:

1. **Overlap test.** Compute player AABB tile range using the same arithmetic as `player_collide`:
   - `tile_left = (int)((p->x - PLAYER_WIDTH/2.0f) / TILE_SIZE)`
   - `tile_right = (int)((p->x + PLAYER_WIDTH/2.0f) / TILE_SIZE)`
   - `tile_top = (int)((p->y - PLAYER_HEIGHT) / TILE_SIZE)`
   - `tile_bottom = (int)(p->y / TILE_SIZE)`
   - Iterate the range; if any tile has `fg == BLOCK_LAVA`, the player is in lava this frame.

2. **Damage tick (when in lava).**
   - A file-local `static int s_was_in_lava = 0;` flag tracks whether the previous frame was also in lava.
   - **First-contact damage:** On the first frame of a new contact (`!s_was_in_lava`), deal 1 HP immediately (`p->health -= 1; s_was_in_lava = 1;`). This guarantees that even a 1-frame bounce contact deals damage (matching Growtopia's "touch = ouch" feel).
   - **Sustained damage** via accumulator:
     - `s_lava_damage_accum += LAVA_DAMAGE_PER_SEC * dt;`
     - `int dmg = (int)s_lava_damage_accum;`
     - `p->health -= dmg; s_lava_damage_accum -= (float)dmg;`
   - When NOT in lava this frame: `s_lava_damage_accum = 0.0f; s_was_in_lava = 0;` (no lingering damage, and the next contact will trigger first-touch damage again).
   - On death/respawn: both `s_lava_damage_accum` and `s_was_in_lava` reset to 0.

3. **Bounce (when in lava).** At most once per frame regardless of overlap count:
   - `p->vy = LAVA_BOUNCE_VELOCITY;`
   - `p->on_ground = 0;`
   - Always bounces straight up regardless of contact side (Growtopia behavior). Because `player_update`/`player_collide` already ran this frame, the bounce velocity is applied on the next frame's integration - no fighting with collision resolution.

4. **Death + respawn.** After damage, if `p->health <= 0`:
   - `p->health = MAX_HEALTH;`
   - `p->x = w->spawn_x; p->y = w->spawn_y;`
   - `p->vx = 0; p->vy = 0;`
   - `s_lava_damage_accum = 0.0f;`
   - `printf("Player died in lava, respawning.\n");` (matches existing `printf("Game saved.\n")` debug-feedback style)
   - No item drop, no death screen - keeps scope minimal.

### `lava_generate_pools` behavior

Called from `world_generate()` after cave carving (before tree placement, which is surface-only and won't interfere).

**Algorithm (bounded cellular fill, no malloc):**

1. `int num_pools = prng_range(&w->rng, LAVA_POOL_MIN, LAVA_POOL_MAX);`
2. For each pool:
   - `cx = prng_range(&w->rng, 0, w->width - 1);`
   - `cy = prng_range(&w->rng, LAVA_POOL_DEPTH_MIN, LAVA_POOL_DEPTH_MAX);`
   - **Find floor:** scan downward from `(cx, cy)` up to 10 tiles. Stop when `world_is_solid(w, cx, y+1)` returns true. If no floor found in range, skip this pool.
   - **Cellular fill with a fixed-size worklist stack:**
     ```c
     int stack_x[LAVA_POOL_MAX_TILES];
     int stack_y[LAVA_POOL_MAX_TILES];
     int stack_n = 0;
     int placed = 0;
     /* push floor tile, mark placed, then while stack non-empty and placed < cap: */
     /*   pop (x,y); for each neighbor (down, left, right): */
     /*     if in-bounds, fg == BLOCK_AIR, neighbor-below is solid, and y >= 27: */
     /*       set fg = BLOCK_LAVA, push, placed++ */
     ```
   - Never overwrite `BLOCK_BEDROCK`. Never place lava at y < 27 (above the cave layer).
   - Cap each pool at `LAVA_POOL_MAX_TILES` (24) tiles.

This produces shallow settled puddles on cave floors, not cascading waterfalls.

### Block definitions (`src/world/block.c`)

Change the `BLOCK_LAVA` entry from:
```c
{BLOCK_LAVA, "Lava", 0, 0, 0, 0, 0, 0, 0, 13, 220, 80, 20},
```
to:
```c
{BLOCK_LAVA, "Lava", 0, 0, 1500, 0, BLOCK_LAVA, 1, 0, 13, 220, 80, 20},
```
- `is_solid = 0` (unchanged)
- `is_background = 0` (unchanged)
- `break_time_ms = 1500` (was 0 - mineable now)
- `rarity = 0` (no rarity tier)
- `drop_item = BLOCK_LAVA`, `drop_count = 1` (drops itself)
- `seed_id = 0` (no seed)
- `tile_sprite_id = 13` (unchanged)
- color unchanged

### Items (`src/world/items.c`)

Add to `block_defs[]` array (between existing entries, order does not matter for designated initializers):
```c
[BLOCK_LAVA] = {BLOCK_LAVA, "Lava", ITEM_CAT_BLOCK, 200, 25, 12, BLOCK_LAVA, 220, 80, 20, 0, 0, 0, 0, 0, 0},
```
- `max_stack = 200` (matches other blocks)
- `gem_cost = 25`, `sell_cost = 12`
- `sprite_id = BLOCK_LAVA` (13)
- All seed/tool/clothing fields = 0

Once this entry exists, `interact_handle_break` and `interact_handle_place` work without changes:
- `interact_handle_break` reads `block_get_break_time(BLOCK_LAVA)` (1500ms), and on completion calls `inventory_add(inv, BLOCK_LAVA, 1)` via `block_get_drop`/`block_get_drop_count`.
- `interact_handle_place` checks `item_is_block(BLOCK_LAVA)` (true because the def's `category == ITEM_CAT_BLOCK`), then sets the target tile's `fg = BLOCK_LAVA` directly via a fetched `Tile *`. Placed lava behaves identically to world-gen lava (damage/bounce/animation) because all checks key off `t->fg == BLOCK_LAVA`.

### Store (`src/game/store.c`)

In `store_init`, inside the `STORE_CAT_BLOCKS` block, add after the existing blocks:
```c
cat_add(cat, BLOCK_LAVA, 25);
```
Price matches the `gem_cost` in `items.c` (consistent with how other blocks are priced identically in both places, e.g. `BLOCK_ROCK`).

### Multi-atlas animation system

**Constants (`src/engine/block_texture.h`):**
```c
#define MAX_ATLAS_FRAMES 4
#define ATLAS_FRAME_MS   150
```

**`block_texture.h` signature changes:**
- `void block_texture_generate(int sprite_id, unsigned char *buffer);`
  becomes
  `void block_texture_generate(int sprite_id, int frame, unsigned char *buffer);`
- `void block_texture_generate_atlas(unsigned char *atlas_buffer);`
  becomes
  `void block_texture_generate_atlas_frame(unsigned char *atlas_buffer, int frame_index);`

**`block_texture.c` changes:**

1. All `tex_*` function signatures change from `tex_*(unsigned char *buffer, int size)` to `tex_*(unsigned char *buffer, int size, int frame)`. Static blocks add `(void)frame;` as their first statement to suppress `-Wunused-parameter`. Their drawing logic is otherwise unchanged.

2. New `tex_lava(unsigned char *buffer, int size, int frame)` replaces the dispatch entry for lava (sprite id 13):
   - Base fill: orange-red gradient (top row brighter `[255, 140, 30]`, bottom darker `[160, 40, 10]`) - gives depth.
   - Per-frame bubbles: scatter ~6-8 circles of brighter yellow-orange `[255, 220, 80]` at deterministic positions derived from `frame`. Use a deterministic hash, e.g.:
     ```c
     unsigned seed = (unsigned)frame * 2654435761u;
     for (int i = 0; i < 7; i++) {
         seed = seed * 1103515245u + 12345u;
         int bx = (seed >> 16) & 31;
         seed = seed * 1103515245u + 12345u;
         int by = (seed >> 16) & 31;
         int br = 2 + ((seed >> 24) & 3);
         /* fill circle centered (bx,by) radius br */
     }
     ```
     Different `frame` values produce visibly different bubble patterns; cycling 0->1->2->3->0 reads as bubbling.
   - No `rand()` calls (project uses `prng.h` for game logic; here a deterministic hash is simpler and reproducible across runs).

3. `block_texture_generate(sprite_id, frame, buffer)` dispatches to `tex_dispatch[i].fn(buffer, TILE_TEX_SIZE, frame)`.

4. `block_texture_generate_atlas_frame(atlas_buffer, frame_index)` is identical to the old `block_texture_generate_atlas` except the inner call is `block_texture_generate(sid, frame_index, tile_buf);`.

**`renderer.h` struct change:**
Replace:
```c
unsigned int atlas_texture;
int atlas_cols;
int atlas_rows;
```
with:
```c
unsigned int atlas_textures[MAX_ATLAS_FRAMES];
int atlas_frame_count;
int atlas_cols;
int atlas_rows;
```
(Audit confirms `atlas_texture` is only referenced by `renderer_draw_tile`/`renderer_draw_tile_scaled`/`renderer_shutdown`/`renderer_generate_atlas` - safe to replace. Note: `atlas_rows` exists in the struct today but is never assigned; `ATLAS_ROWS` is not currently defined in `block_texture.h`. Implementer should add `#define ATLAS_ROWS (ATLAS_SIZE / TILE_TEX_SIZE)` to `block_texture.h` and assign `r->atlas_rows = ATLAS_ROWS;` in `renderer_generate_atlas` - or drop the field entirely. Either is fine; just don't paste the spec snippet without resolving this or it will not compile.)

**`renderer.c` changes:**

1. `renderer_generate_atlas()`:
   ```c
   r->atlas_cols = ATLAS_COLS;
   r->atlas_rows = ATLAS_ROWS;
   r->atlas_frame_count = MAX_ATLAS_FRAMES;
   unsigned char *atlas_buf = malloc(ATLAS_SIZE * ATLAS_SIZE * 4);
   if (!atlas_buf) { r->atlas_frame_count = 0; return; }
   for (int f = 0; f < MAX_ATLAS_FRAMES; f++) {
       block_texture_generate_atlas_frame(atlas_buf, f);
       r->atlas_textures[f] = renderer_load_texture(atlas_buf, ATLAS_SIZE, ATLAS_SIZE, 4);
   }
   free(atlas_buf);
   ```

2. New internal helper:
   ```c
   static unsigned int renderer_current_atlas(Renderer *r) {
       if (r->atlas_frame_count <= 1) return r->atlas_textures[0];
       Uint32 t = SDL_GetTicks();
       int frame = (t / ATLAS_FRAME_MS) % r->atlas_frame_count;
       return r->atlas_textures[frame];
   }
   ```

3. `renderer_begin_tile_batch(r, zoom)`: bind `renderer_current_atlas(r)` via `glBindTexture(GL_TEXTURE_2D, ...)`. (If the existing function does not bind the texture today - verify during implementation - the bind goes inside the batch start. The chosen atlas is fixed for the duration of the batch, so all tiles in one frame render against the same animated frame, which is the desired behavior.)

4. `renderer_draw_tile` / `renderer_draw_tile_scaled`:
   - Replace `if (r->atlas_texture == 0)` with `if (r->atlas_frame_count == 0)` (fallback color-rect path).
   - Remove the per-call `glBindTexture(GL_TEXTURE_2D, r->atlas_texture);` line - the batch owns the bind now.
   - **Keep the per-call `glEnable(GL_TEXTURE_2D);` ... draw ... `glDisable(GL_TEXTURE_2D);` pair** as-is. These bracket each tile draw and are independent of which texture is bound. Moving them out to batch scope is risky (changes GL state across many call sites including UI inventory icons) and out of scope. The bind is the only thing moving to batch scope.
   - The existing `frame` parameter stays as `(void)frame;` to avoid churning all call sites.

5. `renderer_shutdown`: loop and `glDeleteTextures(1, &r->atlas_textures[i])` for each i in `[0, atlas_frame_count)`.

**UI batch (`renderer_begin_ui` / inventory rendering):**
Inventory slots use `renderer_draw_tile_scaled` outside the tile batch (inside `renderer_begin_ui`/`renderer_end_ui`). If the UI batch does not bind an atlas, inventory icons will render incorrectly. Implementation step: bind `r->atlas_textures[0]` (static frame) inside `renderer_begin_ui` for inventory/hotbar tile icons. Inventory icons do not need to animate - showing frame 0 is acceptable. Verify the existing UI batch's bind state during implementation and add the bind if missing.

### World generation wiring (`src/world/world.c`)

In `world_generate`, after the cave-carving loop and before the tree-placement loop, add:
```c
lava_generate_pools(w);
```
Requires adding `#include "../game/lava.h"` to `world.c`. Note: this couples `src/world/` to `src/game/`. Alternative: call `lava_generate_pools` from `game_enter_world` in `main.c` after `world_generate`. **Decision:** call from `world_generate` - world generation is a cohesive unit, and the existing `world_generate` already calls helper logic. The `src/world/ -> src/game/` include is acceptable since the project already has cross-directory includes (e.g. `main.c` includes both).

### Main loop wiring (`src/main.c`)

In `game_update`, immediately after `player_collide(&g->player, &g->world);`, add:
```c
lava_update(&g->world, &g->player, dt);
```
Requires adding `#include "game/lava.h"`.

### Build (`Makefile`)

Add `src/game/lava.o` to the object list.

## Edge cases and compatibility

- **Existing saves (no lava):** load fine. Existing worlds will not retroactively receive lava - `world_load` succeeds and skips `world_generate`. Users can delete `res/worlds/<name>.wld` to regenerate with lava. This matches existing behavior for caves and trees.
- **Player spawns into lava:** does not happen. `spawn_y = 10.0 * TILE_SIZE` (surface), caves start at y=27.
- **Bounce loop:** when bouncing on lava, player takes damage during each brief contact and bounces again on the way down. They escape by moving horizontally off the pool - matches Growtopia.
- **Static blocks in animated atlases:** all 4 atlas frames contain identical pixels for every static block. Only lava's slot (id 13) differs per frame. Switching the bound atlas visually animates only lava.
- **Save format (v3):** no changes. Lava is stored as `t->fg == BLOCK_LAVA` per tile. v1/v2 save loading is unaffected.
- **Player profile save:** no struct changes to `Player`. The damage accumulator is file-local to `lava.c`, reset implicitly on session restart (player spawns at full health anyway).
- **Renderer no-atlas fallback path** (`atlas_frame_count == 0`): still works via the color-rect path. Lava falls back to its solid color (`220, 80, 20`) - no animation, but functional.

## Files touched

| File | Change |
|---|---|
| `src/game/lava.h` | NEW - public API + constants |
| `src/game/lava.c` | NEW - damage/bounce/respawn + puddle generation |
| `src/engine/block_texture.h` | `MAX_ATLAS_FRAMES`, `ATLAS_FRAME_MS`; signature changes |
| `src/engine/block_texture.c` | frame-aware texture gen; new `tex_lava`; atlas builder per-frame |
| `src/engine/renderer.h` | `atlas_textures[]` array, `atlas_frame_count` |
| `src/engine/renderer.c` | multi-atlas gen, current-frame bind, UI bind, shutdown cleanup |
| `src/world/block.c` | `BLOCK_LAVA` def: break_time/drop/drop_count |
| `src/world/items.c` | `[BLOCK_LAVA]` entry in `block_defs[]` |
| `src/game/store.c` | `cat_add(BLOCK_LAVA, 25)` in BLOCKS |
| `src/world/world.c` | call `lava_generate_pools()` after cave carve; include `lava.h` |
| `src/main.c` | call `lava_update()` after `player_collide()`; include `lava.h` |
| `Makefile` | add `src/game/lava.o` |

## Out of scope

- Flowing/fluid simulation for lava (Growtopia lava is static)
- Lava-water interaction (cooling, obsidian, steam) - not in vanilla Growtopia
- Light emission from lava (Growtopia has flat lighting)
- Item drop on death (existing system has no item-drop mechanic)
- Death screen / respawn invulnerability timer (kept minimal - instant respawn)
- Sound effects
- Animated blocks beyond lava (system supports them; none added now)
