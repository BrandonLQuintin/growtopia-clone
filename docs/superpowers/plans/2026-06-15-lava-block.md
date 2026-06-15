# Lava Block Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add Growtopia-style lava block behavior (damage, bounce, respawn, mining/placement, world-gen puddles) backed by a generalized multi-atlas animation system.

**Architecture:** New `src/game/lava.{c,h}` module owns damage/bounce/respawn and world-gen puddles. A multi-atlas animation system in `block_texture.c` + `renderer.c` uploads `MAX_ATLAS_FRAMES` atlas textures and binds the time-appropriate one per batch; static blocks render identically across all atlases, only `tex_lava` varies per frame.

**Tech Stack:** C11, SDL2, OpenGL (fixed-function), procedural pixel textures (no asset files). Build with `make clean && make` (must compile with zero warnings under `-Wall -Wextra`). No test framework; verification is build + manual playtest.

**Spec:** `docs/superpowers/specs/2026-06-15-lava-block-design.md`

---

## File Structure

| File | Status | Responsibility |
|---|---|---|
| `src/game/lava.h` | NEW | Public API + constants for lava gameplay |
| `src/game/lava.c` | NEW | `lava_update()` (damage/bounce/respawn) + `lava_generate_pools()` |
| `src/engine/block_texture.h` | MODIFY | Frame-aware signature, `MAX_ATLAS_FRAMES`, `ATLAS_ROWS`, `ATLAS_FRAME_MS` |
| `src/engine/block_texture.c` | MODIFY | Add `frame` param to all `tex_*`; rewrite `tex_lava` for per-frame variation; per-frame atlas builder |
| `src/engine/renderer.h` | MODIFY | Replace single `atlas_texture` with `atlas_textures[MAX_ATLAS_FRAMES]` + `atlas_frame_count` |
| `src/engine/renderer.c` | MODIFY | Multi-atlas generation, per-batch bind, batch-owned `glBindTexture`, shutdown cleanup |
| `src/world/block.c` | MODIFY | `BLOCK_LAVA` entry: break_time=1500, drop=itself, drop_count=1 |
| `src/world/items.c` | MODIFY | Add `[BLOCK_LAVA]` entry to `block_defs[]` |
| `src/game/store.c` | MODIFY | Add `cat_add(cat, BLOCK_LAVA, 25)` in BLOCKS |
| `src/world/world.c` | MODIFY | Call `lava_generate_pools(w)` after cave carving |
| `src/main.c` | MODIFY | Call `lava_update()` after `player_collide()` |
| `Makefile` | unchanged | `$(wildcard $(SRCDIR)/**/*.c)` already picks up `src/game/lava.c` automatically — verify, do not edit |

**Build note:** The Makefile uses `$(wildcard $(SRCDIR)/**/*.c)` which already matches `src/game/lava.c`. No Makefile edit is needed; just confirm `lava.o` appears in the build output.

---

## Task 1: Make Lava a Placeable, Mineable, Purchasable Block

**Goal:** Lava behaves as a normal block in the economy — buyable in the store, placeable from inventory, mineable for itself. No gameplay/animation behavior yet.

**Files:**
- Modify: `src/world/block.c` (line 19, `BLOCK_LAVA` entry)
- Modify: `src/world/items.c` (add entry in `block_defs[]` array, after `[BLOCK_SAND]` at line 15 to keep block ordering sensible)
- Modify: `src/game/store.c` (line 34, end of `STORE_CAT_BLOCKS` category)

- [ ] **Step 1: Update `BLOCK_LAVA` definition in `src/world/block.c`**

Change the entry at `src/world/block.c:19` from:

```c
    {BLOCK_LAVA, "Lava", 0, 0, 0, 0, 0, 0, 0, 13, 220, 80, 20},
```

to:

```c
    {BLOCK_LAVA, "Lava", 0, 0, 1500, 0, BLOCK_LAVA, 1, 0, 13, 220, 80, 20},
```

Field meaning (in `BlockDef` order from `block.h`): id, name, is_solid=0, is_background=0, break_time_ms=1500, rarity=0, drop_item=BLOCK_LAVA, drop_count=1, seed_id=0, tile_sprite_id=13, color 220/80/20.

- [ ] **Step 2: Add `[BLOCK_LAVA]` entry in `src/world/items.c`**

Insert this line into the `block_defs[]` array (place it right after the `[BLOCK_SAND]` line, around line 15 — the array uses designated initializers so order does not matter functionally, but placing it next to other terrain blocks aids readability):

```c
    [BLOCK_LAVA]      = {BLOCK_LAVA,      "Lava",            ITEM_CAT_BLOCK, 200, 25, 12, BLOCK_LAVA,      220,  80,  20, 0, 0, 0, 0, 0, 0},
```

Field meaning (from `ItemDef` in `items.h`): id=BLOCK_LAVA, name="Lava", category=ITEM_CAT_BLOCK, max_stack=200, gem_cost=25, sell_cost=12, sprite_id=BLOCK_LAVA(13), color 220/80/20, then zeros for seed/tool/clothing fields.

- [ ] **Step 3: Add Lava to the Store BLOCKS category in `src/game/store.c`**

In `store_init`, inside the `STORE_CAT_BLOCKS` block (right after `cat_add(cat, BLOCK_SNOW, 10);` at line 34), add:

```c
    cat_add(cat, BLOCK_LAVA, 25);
```

Price (25) matches the `gem_cost` in `items.c`, consistent with how other blocks are priced identically in both places.

- [ ] **Step 4: Build and verify zero warnings**

Run:
```bash
make clean && make 2>&1 | tee /tmp/lava_build_t1.log
```

Expected: `gcc` compiles every file with no warnings/errors under `-Wall -Wextra`, links successfully, produces `./growtopia`. If warnings appear, fix them before proceeding.

- [ ] **Step 5: Playtest verification**

Run `./growtopia`, enter a world, then:
1. Press `B` to open the Store, click the **Blocks** tab, scroll to the end, verify **Lava** appears priced at 25 gems.
2. Click it to buy. Verify it lands in your inventory.
3. Press `E` to open inventory, drag Lava to a hotbar slot.
4. Close inventory, right-click to place Lava on the ground. Verify a static orange-red Lava block appears (no animation yet — that's expected).
5. Hold left mouse on the Lava block to mine it (takes ~1.5s). Verify it drops back into your inventory.
6. Verify the mouse-hover tooltip shows "Lava" when hovering a placed Lava tile.

- [ ] **Step 6: Commit**

```bash
git add src/world/block.c src/world/items.c src/game/store.c
git commit -m "Add Lava as placeable, mineable, purchasable block"
```

---

## Task 2: Multi-Atlas Animation Infrastructure

**Goal:** Refactor the texture atlas system so the renderer owns multiple atlas textures (one per animation frame) and binds the time-appropriate one at batch start. After this task, the game looks identical (no blocks use multiple frames yet — `MAX_ATLAS_FRAMES=4` atlases all contain identical pixels). This is foundational; Task 3 makes lava actually differ per frame.

**Files:**
- Modify: `src/engine/block_texture.h`
- Modify: `src/engine/block_texture.c` (signature change ripples through all `tex_*` functions + dispatch table + atlas builder)
- Modify: `src/engine/renderer.h`
- Modify: `src/engine/renderer.c`

- [ ] **Step 1: Add new constants to `src/engine/block_texture.h`**

After the existing `#define TILE_TEX_SIZE 32` line, add:

```c
#define MAX_ATLAS_FRAMES 4
#define ATLAS_FRAME_MS   150
#define ATLAS_ROWS       (ATLAS_SIZE / TILE_TEX_SIZE)
```

(`ATLAS_ROWS` was missing — `block_texture.h` only had `ATLAS_SIZE`, `ATLAS_COLS`, `TILE_TEX_SIZE`. The renderer struct has an `atlas_rows` field that was never assigned; this defines it.)

- [ ] **Step 2: Change the `tex_fn` typedef in `src/engine/block_texture.c`**

At `src/engine/block_texture.c:460`, change:

```c
typedef void (*tex_fn)(unsigned char *, int);
```

to:

```c
typedef void (*tex_fn)(unsigned char *, int, int);
```

The third parameter is the animation frame index (0 to `MAX_ATLAS_FRAMES - 1`).

- [ ] **Step 3: Add `int frame` parameter to every `tex_*` function definition in `src/engine/block_texture.c`**

There are many `tex_*` functions in this file (tex_dirt, tex_stone, tex_grass, tex_bedrock, tex_wood, tex_wood_bg, tex_leaves, tex_door, tex_brick, tex_glass, tex_sand, tex_water, tex_lava, tex_cloth_bg, tex_rock, tex_limestone, tex_mud, tex_clay, tex_gravel, tex_ice, tex_snow, tex_sign, tex_lock, tex_store, tex_mailbox, tex_portal, tex_tree_carcass, tex_dirt_bg, tex_stone_bg, tex_grass_bg, tex_wood_bg (other), tex_brick_bg, tex_cloth_bg, tex_default).

For **every** `tex_*` function signature, change `static void tex_X(unsigned char *buf, int size)` to `static void tex_X(unsigned char *buf, int size, int frame)`.

For each **static** function (i.e. all of them except `tex_lava` which is handled in Task 3), add a `(void)frame;` line as the first statement inside the function body to suppress `-Wunused-parameter`.

**Do not modify the body of `tex_lava`** beyond adding the parameter — Task 3 rewrites it.

**Recommended approach:** Search for `static void tex_` and update each signature. Search for `(unsigned char *buf, int size)` to catch any helpers that may also need updating; helpers (like `px_fill`, `px_noise`) do NOT take `frame` and should not change.

Sanity check after editing: `grep -c "int size, int frame" src/engine/block_texture.c` should return the count of `tex_*` functions; `grep -c "(void)frame;" src/engine/block_texture.c` should be one less (since `tex_lava` does not get the `(void)frame;` line — Task 3 uses `frame`).

- [ ] **Step 4: Update the dispatch call site in `src/engine/block_texture.c`**

At `src/engine/block_texture.c:519-528`, the current code is:

```c
void block_texture_generate(int sprite_id, unsigned char *buffer)
{
    tex_default(buffer, TILE_TEX_SIZE);
    for (int i = 0; i < (int)TEX_DISPATCH_COUNT; i++) {
        if (tex_dispatch[i].sprite_id == sprite_id) {
            tex_dispatch[i].fn(buffer, TILE_TEX_SIZE);
            return;
        }
    }
}
```

Change `tex_default(buffer, TILE_TEX_SIZE);` to `tex_default(buffer, TILE_TEX_SIZE, frame);` — but `frame` does not exist yet because the function signature has not been changed. So first change the signature, then update both calls:

```c
void block_texture_generate(int sprite_id, int frame, unsigned char *buffer)
{
    tex_default(buffer, TILE_TEX_SIZE, frame);
    for (int i = 0; i < (int)TEX_DISPATCH_COUNT; i++) {
        if (tex_dispatch[i].sprite_id == sprite_id) {
            tex_dispatch[i].fn(buffer, TILE_TEX_SIZE, frame);
            return;
        }
    }
}
```

- [ ] **Step 5: Update the atlas builder in `src/engine/block_texture.c`**

At `src/engine/block_texture.c:530-552`, change the function name from `block_texture_generate_atlas` to `block_texture_generate_atlas_frame` and pass `frame_index`:

```c
void block_texture_generate_atlas_frame(unsigned char *atlas_buffer, int frame_index)
{
    memset(atlas_buffer, 0, ATLAS_SIZE * ATLAS_SIZE * 4);
    for (int i = 0; i < BLOCK_DEF_COUNT; i++) {
        int sid = BLOCK_DEFS[i].tile_sprite_id;
        int col = sid % ATLAS_COLS;
        int row = sid / ATLAS_COLS;
        int ox = col * TILE_TEX_SIZE;
        int oy = row * TILE_TEX_SIZE;
        unsigned char tile_buf[TILE_TEX_SIZE * TILE_TEX_SIZE * 4];
        block_texture_generate(sid, frame_index, tile_buf);
        for (int ty = 0; ty < TILE_TEX_SIZE; ty++) {
            for (int tx = 0; tx < TILE_TEX_SIZE; tx++) {
                int si = (ty * TILE_TEX_SIZE + tx) * 4;
                int di = ((oy + ty) * ATLAS_SIZE + (ox + tx)) * 4;
                atlas_buffer[di + 0] = tile_buf[si + 0];
                atlas_buffer[di + 1] = tile_buf[si + 1];
                atlas_buffer[di + 2] = tile_buf[si + 2];
                atlas_buffer[di + 3] = tile_buf[si + 3];
            }
        }
    }
}
```

- [ ] **Step 6: Update the header declarations in `src/engine/block_texture.h`**

Change:

```c
void block_texture_generate(int sprite_id, unsigned char *buffer);
void block_texture_generate_atlas(unsigned char *atlas_buffer);
```

to:

```c
void block_texture_generate(int sprite_id, int frame, unsigned char *buffer);
void block_texture_generate_atlas_frame(unsigned char *atlas_buffer, int frame_index);
```

- [ ] **Step 7: Update `Renderer` struct in `src/engine/renderer.h`**

At `src/engine/renderer.h:14-22`, replace the existing struct:

```c
typedef struct {
    SDL_Window *window;
    SDL_GLContext gl_context;
    unsigned int atlas_texture;
    int atlas_cols;
    int atlas_rows;
    int fullscreen;
    int windowed_x, windowed_y, windowed_w, windowed_h;
} Renderer;
```

with:

```c
typedef struct {
    SDL_Window *window;
    SDL_GLContext gl_context;
    unsigned int atlas_textures[MAX_ATLAS_FRAMES];
    int atlas_frame_count;
    int atlas_cols;
    int atlas_rows;
    int fullscreen;
    int windowed_x, windowed_y, windowed_w, windowed_h;
} Renderer;
```

The header already pulls in `SDL2/SDL.h` for `SDL_Window`/`SDL_GLContext` — but `MAX_ATLAS_FRAMES` is defined in `block_texture.h`, which is **not** included by `renderer.h`. Add this include at the top of `src/engine/renderer.h` (after the existing `#include "camera.h"` line):

```c
#include "block_texture.h"
```

Verify `block_texture.h` is not already included transitively by another header in the include chain — if `make` complains about redefinition, remove the include. (`camera.h`, `world.h`, `player.h` are already included and do not pull in `block_texture.h`, so this addition should be clean.)

- [ ] **Step 8: Rewrite `renderer_generate_atlas` in `src/engine/renderer.c`**

At `src/engine/renderer.c:969-976`, replace:

```c
void renderer_generate_atlas(Renderer *r) {
    r->atlas_cols = ATLAS_COLS;
    unsigned char *atlas_buf = (unsigned char *)malloc(ATLAS_SIZE * ATLAS_SIZE * 4);
    if (!atlas_buf) return;
    block_texture_generate_atlas(atlas_buf);
    r->atlas_texture = renderer_load_texture(atlas_buf, ATLAS_SIZE, ATLAS_SIZE, 4);
    free(atlas_buf);
}
```

with:

```c
void renderer_generate_atlas(Renderer *r) {
    r->atlas_cols = ATLAS_COLS;
    r->atlas_rows = ATLAS_ROWS;
    r->atlas_frame_count = MAX_ATLAS_FRAMES;
    unsigned char *atlas_buf = (unsigned char *)malloc(ATLAS_SIZE * ATLAS_SIZE * 4);
    if (!atlas_buf) { r->atlas_frame_count = 0; return; }
    for (int f = 0; f < MAX_ATLAS_FRAMES; f++) {
        block_texture_generate_atlas_frame(atlas_buf, f);
        r->atlas_textures[f] = renderer_load_texture(atlas_buf, ATLAS_SIZE, ATLAS_SIZE, 4);
    }
    free(atlas_buf);
}
```

- [ ] **Step 9: Add `renderer_current_atlas` static helper in `src/engine/renderer.c`**

Place this just above `renderer_draw_tile` (around line 824):

```c
static unsigned int renderer_current_atlas(Renderer *r) {
    if (r->atlas_frame_count <= 1) return r->atlas_textures[0];
    Uint32 t = SDL_GetTicks();
    int frame = (t / ATLAS_FRAME_MS) % r->atlas_frame_count;
    return r->atlas_textures[frame];
}
```

- [ ] **Step 10: Bind the current atlas in `renderer_begin_tile_batch` in `src/engine/renderer.c`**

At `src/engine/renderer.c:925-935`, the function currently only sets up the projection. Add a `glBindTexture` call at the end so the current animation frame's atlas is bound for the entire batch:

```c
void renderer_begin_tile_batch(Renderer *r, float zoom) {
    renderer_get_size(r, &g_screen_w, &g_screen_h);
    float view_w = (float)g_screen_w / zoom;
    float view_h = (float)g_screen_h / zoom;
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, view_w, view_h, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glViewport(0, 0, g_screen_w, g_screen_h);
    if (r->atlas_frame_count > 0) {
        glBindTexture(GL_TEXTURE_2D, renderer_current_atlas(r));
    }
}
```

- [ ] **Step 11: Bind atlas frame 0 in `renderer_begin_ui` in `src/engine/renderer.c`**

Inventory/hotbar/store slot icons are drawn via `renderer_draw_tile_scaled` inside `renderer_begin_ui`/`renderer_end_ui` (see `src/engine/ui.c:169`, `src/engine/ui.c:416`). The UI batch needs the atlas bound too. Use frame 0 (static icons do not animate).

At `src/engine/renderer.c:941-949`, the function currently only sets up projection. Add a bind:

```c
void renderer_begin_ui(Renderer *r) {
    renderer_get_size(r, &g_screen_w, &g_screen_h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, g_screen_w, g_screen_h, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glViewport(0, 0, g_screen_w, g_screen_h);
    if (r->atlas_frame_count > 0) {
        glBindTexture(GL_TEXTURE_2D, r->atlas_textures[0]);
    }
}
```

- [ ] **Step 12: Remove per-call `glBindTexture` from `renderer_draw_tile` in `src/engine/renderer.c`**

At `src/engine/renderer.c:824-844`, the current function:

```c
void renderer_draw_tile(Renderer *r, int screen_x, int screen_y, int tile_id, int frame) {
    (void)frame;
    if (r->atlas_texture == 0) {
        float cr, cg, cb;
        get_tile_color(tile_id, &cr, &cg, &cb);
        renderer_draw_rect(r, screen_x, screen_y, TILE_SIZE, TILE_SIZE, cr, cg, cb, 1.0f);
        return;
    }
    float u0, v0, u1, v1;
    renderer_atlas_uv(tile_id, &u0, &v0, &u1, &v1);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, r->atlas_texture);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glBegin(GL_QUADS);
    glTexCoord2f(u0, v0); glVertex2f((float)screen_x, (float)screen_y);
    glTexCoord2f(u1, v0); glVertex2f((float)(screen_x + TILE_SIZE), (float)screen_y);
    glTexCoord2f(u1, v1); glVertex2f((float)(screen_x + TILE_SIZE), (float)(screen_y + TILE_SIZE));
    glTexCoord2f(u0, v1); glVertex2f((float)screen_x, (float)(screen_y + TILE_SIZE));
    glEnd();
    glDisable(GL_TEXTURE_2D);
}
```

becomes (changes: fallback test uses `atlas_frame_count == 0`; `glBindTexture` line removed; `glEnable`/`glDisable` kept):

```c
void renderer_draw_tile(Renderer *r, int screen_x, int screen_y, int tile_id, int frame) {
    (void)frame;
    if (r->atlas_frame_count == 0) {
        float cr, cg, cb;
        get_tile_color(tile_id, &cr, &cg, &cb);
        renderer_draw_rect(r, screen_x, screen_y, TILE_SIZE, TILE_SIZE, cr, cg, cb, 1.0f);
        return;
    }
    float u0, v0, u1, v1;
    renderer_atlas_uv(tile_id, &u0, &v0, &u1, &v1);
    glEnable(GL_TEXTURE_2D);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glBegin(GL_QUADS);
    glTexCoord2f(u0, v0); glVertex2f((float)screen_x, (float)screen_y);
    glTexCoord2f(u1, v0); glVertex2f((float)(screen_x + TILE_SIZE), (float)screen_y);
    glTexCoord2f(u1, v1); glVertex2f((float)(screen_x + TILE_SIZE), (float)(screen_y + TILE_SIZE));
    glTexCoord2f(u0, v1); glVertex2f((float)screen_x, (float)(screen_y + TILE_SIZE));
    glEnd();
    glDisable(GL_TEXTURE_2D);
}
```

- [ ] **Step 13: Apply the same change to `renderer_draw_tile_scaled` in `src/engine/renderer.c`**

At `src/engine/renderer.c:846-866`, mirror the changes from Step 12: replace `if (r->atlas_texture == 0)` with `if (r->atlas_frame_count == 0)`, and delete the `glBindTexture(GL_TEXTURE_2D, r->atlas_texture);` line. Keep the `glEnable`/`glDisable` pair and the `(void)frame;` line.

- [ ] **Step 14: Update `renderer_shutdown` to free all atlas textures in `src/engine/renderer.c`**

Find the existing texture cleanup in `renderer_shutdown` (search around line 787, the current code is):

```c
    if (r->atlas_texture) {
        glDeleteTextures(1, &r->atlas_texture);
        r->atlas_texture = 0;
    }
```

Replace with:

```c
    for (int i = 0; i < r->atlas_frame_count; i++) {
        if (r->atlas_textures[i]) {
            glDeleteTextures(1, &r->atlas_textures[i]);
            r->atlas_textures[i] = 0;
        }
    }
    r->atlas_frame_count = 0;
```

- [ ] **Step 15: Build and verify zero warnings**

Run:
```bash
make clean && make 2>&1 | tee /tmp/lava_build_t2.log
```

Expected: clean build, no warnings. Common warning sources to watch for:
- `(void)frame;` missing from a static `tex_*` function (would be `-Wunused-parameter`)
- Forgetting to update a `tex_*` signature (would be incompatible-function-pointer-types in the dispatch table)
- `renderer_current_atlas` returning uninitialized `r->atlas_textures[0]` if `atlas_frame_count == 0` — guard with the `<= 1` check already in place

If you see a warning about `MAX_ATLAS_FRAMES` redefinition or `ATLAS_ROWS` redefinition, the constant is defined twice. Check for transitive includes; only define each in `block_texture.h`.

- [ ] **Step 16: Playtest verification (visual identical to before)**

Run `./growtopia`, enter a world, then:
1. Verify the world renders normally — all blocks (dirt, grass, stone, wood, leaves, etc.) look identical to before.
2. Verify the hotbar, inventory, and store render their tile icons correctly (no missing textures, no garbage pixels).
3. Buy a Lava block from the store, place it, verify it still renders (static texture, no animation yet — Task 3 adds animation).
4. Walk around for several seconds — confirm no rendering glitches, no crashes, no memory errors. (The renderer is now switching which of 4 atlases is bound every 150ms; visually nothing changes because all 4 atlases are pixel-identical at this point.)

- [ ] **Step 17: Commit**

```bash
git add src/engine/block_texture.h src/engine/block_texture.c src/engine/renderer.h src/engine/renderer.c
git commit -m "Refactor texture system to support multi-frame atlas animation"
```

---

## Task 3: Animate the Lava Texture

**Goal:** `tex_lava` now draws visibly different patterns per frame, so placed Lava blocks visibly animate (bubbling/flickering effect).

**Files:**
- Modify: `src/engine/block_texture.c` (rewrite body of `tex_lava`)

- [ ] **Step 1: Rewrite `tex_lava` in `src/engine/block_texture.c`**

The current `tex_lava` (around line 245) has signature `static void tex_lava(unsigned char *buf, int size, int frame)` after Task 2. Replace its body with a frame-aware version. The existing implementation uses `prng_seed(&p, 800)` with a fixed seed — change the seed to depend on `frame` so each frame produces a different bubble pattern:

```c
static void tex_lava(unsigned char *buf, int size, int frame)
{
    Prng p;
    prng_seed(&p, 800u + (unsigned)frame * 7919u);
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            float t = (float)y / (float)(size - 1);
            unsigned char r = (unsigned char)(255.0f * (1.0f - t * 0.35f));
            unsigned char g = (unsigned char)(130.0f * (1.0f - t * 0.55f));
            unsigned char b = (unsigned char)(30.0f  * (1.0f - t * 0.50f));
            int idx = (y * size + x) * 4;
            buf[idx + 0] = r;
            buf[idx + 1] = g;
            buf[idx + 2] = b;
            buf[idx + 3] = 255;
        }
    }
    for (int i = 0; i < 8; i++) {
        int cx = prng_range(&p, 3, size - 4);
        int cy = prng_range(&p, 3, size - 4);
        int br = prng_range(&p, 2, 4);
        for (int dy = -br; dy <= br; dy++) {
            for (int dx = -br; dx <= br; dx++) {
                if (dx * dx + dy * dy <= br * br) {
                    int px_x = cx + dx;
                    int px_y = cy + dy;
                    if (px_x >= 0 && px_x < size && px_y >= 0 && px_y < size) {
                        int idx = (px_y * size + px_x) * 4;
                        buf[idx + 0] = 255;
                        buf[idx + 1] = 220;
                        buf[idx + 2] = 80;
                        buf[idx + 3] = 255;
                    }
                }
            }
        }
    }
}
```

This:
- Draws a vertical orange-to-dark-red gradient (top brighter, bottom darker) — gives depth.
- Uses `prng_seed(&p, 800u + frame * 7919u)` so each frame produces a different bubble pattern deterministically.
- Scatters 8 bright yellow-orange bubbles in different positions per frame.
- When the renderer cycles frames 0→1→2→3→0 every 600ms, the bubbles visibly shift around, producing a bubbling effect.

(`Prng`/`prng_seed`/`prng_range` are already used in this file — no new includes needed.)

- [ ] **Step 2: Build and verify zero warnings**

Run:
```bash
make clean && make 2>&1 | tee /tmp/lava_build_t3.log
```

Expected: clean build.

- [ ] **Step 3: Playtest verification (animation visible)**

Run `./growtopia`, enter a world, then:
1. Open store → Blocks → buy Lava → place several Lava tiles in a row.
2. Watch the Lava tiles for ~2 seconds. Verify the bubbles visibly shift/flicker (cycling through 4 frames at ~600ms total cycle).
3. Verify static blocks (dirt, stone, etc.) still do not animate — only Lava moves.
4. Open inventory. Verify the Lava inventory icon shows a static image (frame 0, since UI binds `atlas_textures[0]`).

- [ ] **Step 4: Commit**

```bash
git add src/engine/block_texture.c
git commit -m "Animate lava texture with per-frame bubbling pattern"
```

---

## Task 4: World Generation — Lava Pools in Caves

**Goal:** Newly-generated worlds contain 5–8 sparse lava puddles at the bottom of caves. Existing saves are unaffected (lava only appears when a new world is generated).

**Files:**
- Create: `src/game/lava.h`
- Create: `src/game/lava.c` (only `lava_generate_pools` for now; `lava_update` comes in Task 5)
- Modify: `src/world/world.c` (add include + call from `world_generate`)

- [ ] **Step 1: Create `src/game/lava.h`**

Full file contents:

```c
#ifndef LAVA_H
#define LAVA_H

#include "../world/world.h"
#include "../game/player.h"

#define LAVA_DAMAGE_PER_SEC     20.0f
#define LAVA_BOUNCE_VELOCITY  -550.0f

#define LAVA_POOL_MIN            5
#define LAVA_POOL_MAX            8
#define LAVA_POOL_MAX_TILES     24
#define LAVA_POOL_DEPTH_MIN     40
#define LAVA_POOL_DEPTH_MAX     56

void lava_update(World *w, Player *p, float dt);
void lava_generate_pools(World *w);

#endif
```

(`lava_update` is declared here even though it is implemented in Task 5 — this keeps the header complete and lets `world.c` include the file without forward-declaring anything.)

- [ ] **Step 2: Create `src/game/lava.c` with `lava_generate_pools`**

Full file contents:

```c
#include "lava.h"
#include "../engine/prng.h"
#include "../engine/renderer.h"
#include <stdio.h>

void lava_generate_pools(World *w)
{
    int num_pools = prng_range(&w->rng, LAVA_POOL_MIN, LAVA_POOL_MAX);
    for (int n = 0; n < num_pools; n++) {
        int cx = prng_range(&w->rng, 0, w->width - 1);
        int cy_start = prng_range(&w->rng, LAVA_POOL_DEPTH_MIN, LAVA_POOL_DEPTH_MAX);

        int floor_y = -1;
        for (int y = cy_start; y < w->height - 1 && y < cy_start + 10; y++) {
            if (world_is_solid(w, cx, y + 1)) {
                floor_y = y;
                break;
            }
        }
        if (floor_y < 0) continue;

        int stack_x[LAVA_POOL_MAX_TILES];
        int stack_y[LAVA_POOL_MAX_TILES];
        int stack_n = 0;
        int placed = 0;

        Tile *start = world_get_tile(w, cx, floor_y);
        if (!start || start->fg != BLOCK_AIR) continue;
        start->fg = BLOCK_LAVA;
        stack_x[stack_n] = cx;
        stack_y[stack_n] = floor_y;
        stack_n++;
        placed++;

        while (stack_n > 0 && placed < LAVA_POOL_MAX_TILES) {
            stack_n--;
            int x = stack_x[stack_n];
            int y = stack_y[stack_n];

            const int dxs[3] = {0, -1, 1};
            for (int i = 0; i < 3 && placed < LAVA_POOL_MAX_TILES; i++) {
                int nx = x + dxs[i];
                int ny = (dxs[i] == 0) ? y + 1 : y;
                if (nx < 0 || nx >= w->width) continue;
                if (ny < 27 || ny >= w->height) continue;
                Tile *t = world_get_tile(w, nx, ny);
                if (!t || t->fg != BLOCK_AIR) continue;
                Tile *below_target = world_get_tile(w, nx, ny + 1);
                if (!below_target || !block_is_solid_with_data(below_target->fg, below_target->extra_data)) continue;
                t->fg = BLOCK_LAVA;
                stack_x[stack_n] = nx;
                stack_y[stack_n] = ny;
                stack_n++;
                placed++;
            }
        }
    }
}

void lava_update(World *w, Player *p, float dt)
{
    (void)w;
    (void)p;
    (void)dt;
}
```

Notes:
- The `lava_update` stub here is a placeholder so the file compiles standalone. Task 5 replaces it with the real implementation.
- `<stdio.h>` is included explicitly for the `printf` in Task 5's `lava_update` (rather than relying on a fragile transitive include chain through SDL headers).
- The cellular fill spreads **down first** (dx=0, ny=y+1) then **sideways** (dx=±1, ny=y), using a single uniform rule for both: the target tile must be air AND the tile below the target must be solid. This naturally produces shallow settled puddles on cave floors — downward flow stops when it hits the cave bottom, sideways flow only fills along solid floors.
- Cap is `LAVA_POOL_MAX_TILES` (24) per pool.

- [ ] **Step 3: Wire `lava_generate_pools` into `world_generate` in `src/world/world.c`**

At the top of `src/world/world.c` (with the other includes, around line 6), add:

```c
#include "../game/lava.h"
```

In `world_generate` (around line 86, after the cave-carving `for (int c = 0; c < num_caves; c++) { ... }` loop ends and before the tree-placement `for (int x = 2; ...)` loop begins), insert:

```c
    lava_generate_pools(w);
```

- [ ] **Step 4: Build and verify zero warnings**

Run:
```bash
make clean && make 2>&1 | tee /tmp/lava_build_t4.log
```

Expected: clean build. Confirm `build/game/lava.o` appears in the build output (the Makefile wildcard should pick it up automatically).

If you see "no rule to make target build/game/lava.o" or similar, the Makefile wildcard did not match — run `make clean` and `make` again (sometimes the wildcard expansion needs a clean rebuild).

- [ ] **Step 5: Playtest verification (lava in caves)**

Existing worlds won't have lava (they were generated before this change). To test:

1. Delete an existing test world save so it regenerates: `rm -f res/worlds/test.wld` (substitute your test world name).
2. Run `./growtopia`, enter the deleted world name — it regenerates with caves + lava pools.
3. Walk to the underground (dig down through dirt/stone). Explore caves around y=40-56.
4. Verify you find some lava puddles (5-8 per world, sparse — you may need to explore a few caves).
5. Verify the lava is rendered with the animated texture from Task 3.
6. Verify lava does NOT appear above y=27 (no lava on the surface or in trees).
7. Save and reload the world (`F5` then re-enter). Verify lava positions persist.

- [ ] **Step 6: Commit**

```bash
git add src/game/lava.h src/game/lava.c src/world/world.c
git commit -m "Generate sparse lava pools in cave bottoms during world gen"
```

---

## Task 5: Lava Damage, Bounce, and Respawn

**Goal:** Walking into lava deals 20 HP/sec damage, bounces the player up at -550 px/s, and dying (health ≤ 0) respawns the player at world spawn with full health.

**Files:**
- Modify: `src/game/lava.c` (replace the `lava_update` stub)
- Modify: `src/main.c` (add include + call from `game_update`)

- [ ] **Step 1: Replace the `lava_update` stub in `src/game/lava.c`**

Replace the stub at the bottom of `src/game/lava.c`:

```c
void lava_update(World *w, Player *p, float dt)
{
    (void)w;
    (void)p;
    (void)dt;
}
```

with the real implementation:

```c
void lava_update(World *w, Player *p, float dt)
{
    static float s_lava_damage_accum = 0.0f;

    float hw = PLAYER_WIDTH / 2.0f;
    int tile_left   = (int)((p->x - hw) / TILE_SIZE);
    int tile_right  = (int)((p->x + hw) / TILE_SIZE);
    int tile_top    = (int)((p->y - PLAYER_HEIGHT) / TILE_SIZE);
    int tile_bottom = (int)(p->y / TILE_SIZE);

    int in_lava = 0;
    for (int ty = tile_top; ty <= tile_bottom; ty++) {
        for (int tx = tile_left; tx <= tile_right; tx++) {
            Tile *t = world_get_tile(w, tx, ty);
            if (t && t->fg == BLOCK_LAVA) {
                in_lava = 1;
                break;
            }
        }
        if (in_lava) break;
    }

    if (!in_lava) {
        s_lava_damage_accum = 0.0f;
        return;
    }

    s_lava_damage_accum += LAVA_DAMAGE_PER_SEC * dt;
    int dmg = (int)s_lava_damage_accum;
    if (dmg > 0) {
        p->health -= dmg;
        s_lava_damage_accum -= (float)dmg;
    }

    p->vy = LAVA_BOUNCE_VELOCITY;
    p->on_ground = 0;

    if (p->health <= 0) {
        p->health = MAX_HEALTH;
        p->x = w->spawn_x;
        p->y = w->spawn_y;
        p->vx = 0;
        p->vy = 0;
        s_lava_damage_accum = 0.0f;
        printf("Player died in lava, respawning.\n");
    }
}
```

This matches the spec section "lava_update behavior":
1. AABB tile-range overlap test (same arithmetic as `player_collide` in `player.c:78-81`).
2. Damage accumulator (`s_lava_damage_accum`, file-local static to avoid extending `Player` struct and breaking save compat).
3. Bounce: `p->vy = LAVA_BOUNCE_VELOCITY` (-550) once per frame, always upward.
4. Respawn on death at world spawn with full health, zero velocity, accumulator cleared.

(`MAX_HEALTH`, `PLAYER_WIDTH`, `PLAYER_HEIGHT`, `TILE_SIZE` are all already in scope via the includes — `player.h` defines the player constants and includes `world.h` which defines `TILE_SIZE` via `renderer.h`.)

If `printf` requires an include, confirm `<stdio.h>` is transitively available — it is, via `world.h` → `stdio.h`. No new include needed in `lava.c`.

- [ ] **Step 2: Wire `lava_update` into the main loop in `src/main.c`**

At the top of `src/main.c`, with the other game includes (around line 22, after `#include "game/interact.h"`), add:

```c
#include "game/lava.h"
```

In `game_update`, find the line `player_collide(&g->player, &g->world);` (around `src/main.c:336`). Immediately **after** that line, add:

```c
    lava_update(&g->world, &g->player, dt);
```

(Placing it after `player_collide` is critical: collision resolution has already run for this frame, so setting `p->vy` here applies cleanly on the next frame's `player_update`.)

- [ ] **Step 3: Build and verify zero warnings**

Run:
```bash
make clean && make 2>&1 | tee /tmp/lava_build_t5.log
```

Expected: clean build.

- [ ] **Step 4: Playtest verification (damage, bounce, respawn)**

Run `./growtopia`, enter a world with lava (regenerate if needed per Task 4 Step 5), then:

1. **Bounce test:** Find a lava pool in a cave. Walk into it from the side. Verify the player gets launched upward (~2-3 tiles) and falls back, bouncing repeatedly. Each contact should re-bounce.
2. **Damage test:** Let yourself bounce on lava for ~5 seconds. Watch the health HUD in the top-left — verify it decreases (at ~20 HP/sec during contact, but actual loss depends on how long you're in contact each bounce). Verify health does NOT decrease when you're not touching lava.
3. **Respawn test:** Stay in the lava until health hits 0. Verify the player teleports to the world spawn point (surface, near world center), health returns to 100, and the console prints `Player died in lava, respawning.`. Verify velocity is zeroed (you don't keep flying).
4. **Escape test:** While bouncing on lava, hold A or D to walk horizontally off the pool. Verify you stop bouncing and stop taking damage once off the lava.
5. **Placed-lava test:** Place a Lava block on the surface (from Task 1). Walk into it. Verify the same damage/bounce/respawn behavior applies to player-placed lava (not just world-gen lava).
6. **Edge case — placing lava on yourself:** Place lava directly where you're standing. Verify you take damage and bounce, no crash, no infinite loop.
7. **Edge case — spawn safety:** Verify the spawn point is never in lava (it shouldn't be, since spawn is at y=10 which is surface, and lava only generates at y≥27). Force a respawn by dying; verify you don't immediately take lava damage at spawn.

- [ ] **Step 5: Commit**

```bash
git add src/game/lava.c src/main.c
git commit -m "Add lava damage, bounce, and respawn behavior"
```

---

## Task 6: Final Verification

**Goal:** Whole feature builds cleanly, plays correctly end-to-end, follows project conventions per AGENTS.md.

- [ ] **Step 1: Clean build with zero warnings**

```bash
make clean && make 2>&1 | tee /tmp/lava_build_final.log
```

Confirm:
- No warnings, no errors.
- `build/game/lava.o` is in the link line.
- Binary `./growtopia` is produced.

- [ ] **Step 2: AGENTS.md convention checks**

Re-read `AGENTS.md` and verify each applies:

| Convention | Verification |
|---|---|
| C11, `-Wall -Wextra` | Step 1 confirms |
| No comments in code | `grep -nE "^\s*//|/\*" src/game/lava.c src/game/lava.h src/engine/block_texture.c src/engine/renderer.c` — should return nothing in the new/changed code |
| Header include guards | `lava.h` uses `#ifndef LAVA_H` ✓ |
| Module-prefixed function names | `lava_update`, `lava_generate_pools` ✓ |
| snake_case | All new identifiers ✓ |
| No hardcoded `SCREEN_WIDTH`/`SCREEN_HEIGHT` | No new references to either; existing `g_screen_w`/`g_screen_h` usage unchanged |
| Font scales ≥ 1.0 | No new text rendering added |
| Physics doesn't break grounding | Bounce sets `p->on_ground = 0` explicitly; `player_collide` runs first as before; no jitter expected (verify in playtest Step 4) |
| Save/load compatibility | No struct field added to `Player`, `Tile`, or `World`. `BLOCK_LAVA` is just an `fg` value, already serializable. v1/v2/v3 loads unaffected. |
| UI click/render position parity | No new UI slots added |

- [ ] **Step 3: End-to-end playtest**

Run the full feature flow:
1. Launch game. Enter a world name you've never used before (to force fresh generation).
2. Walk around the surface — confirm no lava spawns above ground, no rendering glitches, no animation on non-lava blocks.
3. Dig down to the cave layer. Find a lava pool. Verify animation is visible.
4. Touch the lava. Verify damage + bounce. Die. Verify respawn at spawn point with full health.
5. Open the store, buy Lava, place it on the surface. Verify it animates and damages you.
6. Mine the placed lava with the pickaxe. Verify it drops back into inventory.
7. Save the world (F5), exit to menu (ESC → Y), re-enter the world. Verify all placed/world-gen lava persists and still animates/damages.
8. Open inventory during gameplay — verify Lava inventory icon shows (static frame 0).

- [ ] **Step 4: Optional — dispatch code reviewer**

Per AGENTS.md "Code Review Workflow", optionally dispatch a code reviewer subagent to check the diff against conventions. (This is not strictly required since the plan was reviewer-approved, but it's a good practice for non-trivial features.)

If the reviewer finds issues, address them in a follow-up commit. If clean, the feature is done.

- [ ] **Step 5: Final state confirmation**

```bash
git log --oneline -10
```

Expected commits (in order, newest first):
- `Add lava damage, bounce, and respawn behavior`
- `Generate sparse lava pools in cave bottoms during world gen`
- `Animate lava texture with per-frame bubbling pattern`
- `Refactor texture system to support multi-frame atlas animation`
- `Add Lava as placeable, mineable, purchasable block`

Confirm the working tree is clean (`git status` shows no modified files).

Feature complete.

---

## Self-Review Notes (for plan author)

**Spec coverage check** — every spec section maps to a task:
- Damage/bounce/death/respawn → Task 5
- `lava.c` module + `lava.h` constants → Task 4 (header) + Task 5 (update) + Task 4 (generate)
- Multi-atlas animation system → Task 2
- Animated `tex_lava` → Task 3
- `BLOCK_LAVA` def updates (break_time, drop) → Task 1
- Items entry → Task 1
- Store entry → Task 1
- World-gen wiring (`world.c` calls `lava_generate_pools`) → Task 4
- Main-loop wiring (`main.c` calls `lava_update` after `player_collide`) → Task 5
- Save/load compat — no struct changes, verified in Task 6 Step 2 ✓
- Edge cases (spawn safety, bounce loop, existing saves) → Task 5 Step 4 + Task 6 Step 3

**Type/signature consistency** — verified:
- `lava_update(World *, Player *, float)` — declared in `lava.h` Task 4, defined in `lava.c` Task 5, called in `main.c` Task 5 ✓
- `lava_generate_pools(World *)` — declared in `lava.h` Task 4, defined in `lava.c` Task 4, called in `world.c` Task 4 ✓
- `block_texture_generate(int, int, unsigned char *)` — declared in `block_texture.h` Task 2 Step 6, defined Task 2 Step 4, called from `block_texture_generate_atlas_frame` Task 2 Step 5 ✓
- `block_texture_generate_atlas_frame(unsigned char *, int)` — declared Task 2 Step 6, defined Task 2 Step 5, called from `renderer_generate_atlas` Task 2 Step 8 ✓
- `Renderer.atlas_textures[MAX_ATLAS_FRAMES]` — defined Task 2 Step 7, written Task 2 Step 8, read Task 2 Steps 9-11, freed Task 2 Step 14 ✓

**No placeholders** — every step has either exact code or an exact command.
