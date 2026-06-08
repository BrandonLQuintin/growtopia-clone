# Interactive Blocks Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add interactive Door, Sign, and Portal blocks that respond to punch (toggle/read/teleport) and wrench (edit/link) actions.

**Architecture:** New `src/game/interact.c/h` module dispatches block-specific logic. Uses existing `extra_data` on tiles for state. `block_is_solid_with_data` makes door solidity dynamic. Sign text stored in world-level string table. Portals link via packed coordinates in `extra_data`. Save format bumps to version 2.

**Tech Stack:** C11, SDL2, OpenGL (fixed-function), existing codebase conventions

**Spec:** `docs/superpowers/specs/2026-06-08-interactive-blocks-design.md`

---

## File Structure

| File | Action | Responsibility |
|------|--------|----------------|
| `src/game/interact.c` | Create | Interactive block dispatch: punch, wrench, cleanup, sign/portal helpers |
| `src/game/interact.h` | Create | Public API for interact module |
| `src/world/world.c` | Modify | Sign table in World, save/load v2, tile-aware `world_is_solid` |
| `src/world/world.h` | Modify | Sign table fields in World struct |
| `src/world/block.c` | Modify | `block_is_solid_with_data`, `block_is_interactive`, door is_solid=1 |
| `src/world/block.h` | Modify | New function declarations |
| `src/engine/ui.c` | Modify | Sign edit UI state rendering and input handling |
| `src/engine/ui.h` | Modify | `UI_STATE_SIGN_EDIT` enum, new struct fields, new function declarations |
| `src/engine/block_texture.c` | Modify | Door, sign, portal texture dispatch entries |
| `src/main.c` | Modify | Game struct additions, punch/wrench dispatch, sign overlay, portal link state, sign edit key handler |

---

### Task 1: Block layer changes (block.c/h)

**Files:**
- Modify: `src/world/block.c`
- Modify: `src/world/block.h`

- [ ] **Step 1: Add new function declarations to block.h**

Add after the existing declarations (after line 29):

```c
int block_is_solid_with_data(uint16_t block_id, uint32_t extra_data);
int block_is_interactive(uint16_t block_id);
```

- [ ] **Step 2: Change door is_solid to 1 in block.c**

In `block.c` line 14, the BLOCK_DOOR entry has `is_solid = 0` (third field). Change it to `1`:

```c
{BLOCK_DOOR, "Door", 1, 0, 500, 2, BLOCK_DOOR, 1, 0, 8, 160, 110, 50},
```

- [ ] **Step 3: Add block_is_solid_with_data and block_is_interactive to block.c**

Add at the end of `block.c` (after line 125):

```c
int block_is_solid_with_data(uint16_t block_id, uint32_t extra_data) {
    if (block_id == BLOCK_AIR) return 0;
    if (block_id == BLOCK_DOOR) {
        return !(extra_data & 1);
    }
    return block_is_solid(block_id);
}

int block_is_interactive(uint16_t block_id) {
    return block_id == BLOCK_DOOR || block_id == BLOCK_SIGN || block_id == BLOCK_PORTAL;
}
```

- [ ] **Step 4: Build and verify**

Run: `make clean && make`
Expected: Compiles with zero warnings. The new functions are defined but not yet called, so no link errors.

- [ ] **Step 5: Commit**

```bash
git add src/world/block.c src/world/block.h
git commit -m "feat: add block_is_solid_with_data and block_is_interactive for interactive blocks"
```

---

### Task 2: World layer changes -- sign table and tile-aware solidity (world.c/h)

**Files:**
- Modify: `src/world/world.h`
- Modify: `src/world/world.c`

- [ ] **Step 1: Add sign table fields and new constants to world.h**

Add after `#define GROWTH_COMPLETE 5` (after line 72):

```c
#define SIGN_TABLE_SIZE 64
#define SIGN_TEXT_MAX_LEN 32
#define PORTAL_UNLINKED 0
```

Add to the `World` struct (after the `name` field, before the closing `}`):

```c
    char sign_texts[SIGN_TABLE_SIZE][SIGN_TEXT_MAX_LEN + 1];
    int sign_count;
```

- [ ] **Step 2: Update world_is_solid to use tile-aware check in world.c**

Replace the `world_is_solid` function (lines 187-191) with:

```c
int world_is_solid(World *w, int x, int y) {
    Tile *t = world_get_tile(w, x, y);
    if (!t) return 1;
    return block_is_solid_with_data(t->fg, t->extra_data);
}
```

- [ ] **Step 3: Update world_save to version 2 with sign data**

Replace the `world_save` function (lines 138-155) with:

```c
int world_save(World *w, const char *path) {
    FILE *f = fopen(path, "wb");
    if (!f) return -1;

    const char magic[4] = {'G', 'R', 'O', 'W'};
    uint32_t version = 2;

    if (fwrite(magic, 1, 4, f) != 4) { fclose(f); return -1; }
    if (fwrite(&version, sizeof(uint32_t), 1, f) != 1) { fclose(f); return -1; }
    if (fwrite(&w->width, sizeof(int), 1, f) != 1) { fclose(f); return -1; }
    if (fwrite(&w->height, sizeof(int), 1, f) != 1) { fclose(f); return -1; }

    size_t tile_count = (size_t)w->width * w->height;
    if (fwrite(w->tiles, sizeof(Tile), tile_count, f) != tile_count) { fclose(f); return -1; }

    uint32_t sc = (uint32_t)w->sign_count;
    if (fwrite(&sc, sizeof(uint32_t), 1, f) != 1) { fclose(f); return -1; }
    for (int i = 0; i < w->sign_count; i++) {
        if (fwrite(w->sign_texts[i], 1, SIGN_TEXT_MAX_LEN + 1, f) != SIGN_TEXT_MAX_LEN + 1) { fclose(f); return -1; }
    }

    fclose(f);
    return 0;
}
```

- [ ] **Step 4: Update world_load to handle version 1 and 2**

Replace the `world_load` function (lines 157-185) with:

```c
int world_load(World *w, const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return -1;

    char magic[4];
    uint32_t version;
    int width, height;

    if (fread(magic, 1, 4, f) != 4) { fclose(f); return -1; }
    if (memcmp(magic, "GROW", 4) != 0) { fclose(f); return -1; }

    if (fread(&version, sizeof(uint32_t), 1, f) != 1) { fclose(f); return -1; }
    if (version < 1 || version > 2) { fclose(f); return -1; }

    if (fread(&width, sizeof(int), 1, f) != 1) { fclose(f); return -1; }
    if (fread(&height, sizeof(int), 1, f) != 1) { fclose(f); return -1; }

    if (world_init(w, width, height) != 0) { fclose(f); return -1; }

    size_t tile_count = (size_t)width * height;
    if (fread(w->tiles, sizeof(Tile), tile_count, f) != tile_count) {
        world_free(w);
        fclose(f);
        return -1;
    }

    memset(w->sign_texts, 0, sizeof(w->sign_texts));
    w->sign_count = 0;

    if (version >= 2) {
        uint32_t sc;
        if (fread(&sc, sizeof(uint32_t), 1, f) == 1 && sc <= SIGN_TABLE_SIZE) {
            w->sign_count = (int)sc;
            for (int i = 0; i < w->sign_count; i++) {
                if (fread(w->sign_texts[i], 1, SIGN_TEXT_MAX_LEN + 1, f) != SIGN_TEXT_MAX_LEN + 1) {
                    break;
                }
            }
        }
    }

    fclose(f);
    return 0;
}
```

- [ ] **Step 5: Build and verify**

Run: `make clean && make`
Expected: Compiles with zero warnings.

- [ ] **Step 6: Commit**

```bash
git add src/world/world.c src/world/world.h
git commit -m "feat: add sign text table to world, tile-aware solidity, save/load v2"
```

---

### Task 3: Interact module (interact.c/h)

**Files:**
- Create: `src/game/interact.h`
- Create: `src/game/interact.c`

- [ ] **Step 1: Create interact.h**

```c
#ifndef INTERACT_H
#define INTERACT_H

#include <stdint.h>
#include "../world/world.h"
#include "player.h"

int interact_punch(World *w, Player *p, int tx, int ty);
int interact_wrench(World *w, int tx, int ty, int *pending_x, int *pending_y, int *pending);
const char *interact_get_sign_text(World *w, int tx, int ty);
int interact_alloc_sign(World *w, int tx, int ty);
void interact_cleanup_break(World *w, int tx, int ty);
int interact_is_portal_linked(World *w, int tx, int ty);

#endif
```

- [ ] **Step 2: Create interact.c**

```c
#include "interact.h"
#include "../world/block.h"
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
    int slot = w->sign_count;
    if (slot >= SIGN_TABLE_SIZE) return 0;
    w->sign_texts[slot][0] = '\0';
    w->sign_count++;
    t->extra_data = (uint32_t)(slot + 1);
    return (int)t->extra_data;
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
```

- [ ] **Step 3: Build and verify**

Run: `make clean && make`
Expected: Compiles with zero warnings.

- [ ] **Step 4: Commit**

```bash
git add src/game/interact.c src/game/interact.h
git commit -m "feat: add interact module with door, sign, portal logic"
```

---

### Task 4: UI sign edit state (ui.c/h)

**Files:**
- Modify: `src/engine/ui.h`
- Modify: `src/engine/ui.c`

- [ ] **Step 1: Add UI_STATE_SIGN_EDIT and new fields to ui.h**

Change the `UIState` enum (lines 12-17) to add the new state:

```c
typedef enum {
    UI_STATE_NONE,
    UI_STATE_INVENTORY,
    UI_STATE_STORE,
    UI_STATE_CRAFTING,
    UI_STATE_SIGN_EDIT
} UIState;
```

Add new fields to the `UI` struct (after `inventory_scroll` at line 48, before the closing `}`):

```c
    int sign_edit_x, sign_edit_y;
    char sign_edit_text[SIGN_TEXT_MAX_LEN + 1];
    int sign_edit_cursor;
    float sign_edit_cursor_timer;
```

Add new function declarations (after `ui_render_hud` at line 62, before `ui_get_hotbar_selection`):

```c
void ui_init_sign_edit(UI *ui, World *w, int tx, int ty);
void ui_finish_sign_edit(UI *ui, World *w);
void ui_render_sign_edit(UI *ui, Renderer *renderer);
void ui_update_sign_edit(UI *ui, Input *input);
```

- [ ] **Step 2: Add sign edit functions to ui.c**

Read `src/engine/ui.c` fully to understand existing patterns, then add these functions at the end of the file before the final closing (or after the last existing function):

```c
void ui_init_sign_edit(UI *ui, World *w, int tx, int ty) {
    ui->state = UI_STATE_SIGN_EDIT;
    ui->sign_edit_x = tx;
    ui->sign_edit_y = ty;
    memset(ui->sign_edit_text, 0, sizeof(ui->sign_edit_text));
    ui->sign_edit_cursor = 0;
    ui->sign_edit_cursor_timer = 0;
    const char *existing = interact_get_sign_text(w, tx, ty);
    if (existing) {
        int len = (int)strlen(existing);
        if (len > SIGN_TEXT_MAX_LEN) len = SIGN_TEXT_MAX_LEN;
        memcpy(ui->sign_edit_text, existing, len);
        ui->sign_edit_cursor = len;
    }
}

void ui_finish_sign_edit(UI *ui, World *w) {
    Tile *t = world_get_tile(w, ui->sign_edit_x, ui->sign_edit_y);
    if (!t || t->fg != BLOCK_SIGN) {
        ui->state = UI_STATE_NONE;
        return;
    }
    if (ui->sign_edit_text[0] != '\0') {
        int idx = interact_alloc_sign(w, ui->sign_edit_x, ui->sign_edit_y);
        if (idx > 0) {
            memcpy(w->sign_texts[idx - 1], ui->sign_edit_text, SIGN_TEXT_MAX_LEN + 1);
        }
    } else {
        if (t->extra_data != 0) {
            int idx = (int)t->extra_data - 1;
            if (idx >= 0 && idx < SIGN_TABLE_SIZE) {
                memset(w->sign_texts[idx], 0, SIGN_TEXT_MAX_LEN + 1);
            }
            t->extra_data = 0;
        }
    }
    ui->state = UI_STATE_NONE;
}

void ui_update_sign_edit(UI *ui, Input *input) {
    ui->sign_edit_cursor_timer += 1.0f / 60.0f;
    if (ui->sign_edit_cursor_timer >= 1.0f) ui->sign_edit_cursor_timer -= 1.0f;
}

void ui_render_sign_edit(UI *ui, Renderer *renderer) {
    int panel_w = 300;
    int panel_h = 120;
    int px = (g_screen_w - panel_w) / 2;
    int py = (g_screen_h - panel_h) / 2;

    renderer_draw_rect(renderer, px, py, panel_w, panel_h, 0.0f, 0.0f, 0.0f, 0.85f);

    renderer_draw_text(renderer, "Edit Sign", px + 10, py + 8, 2.0f, 1.0f, 1.0f, 1.0f);

    renderer_draw_rect(renderer, px + 20, py + 40, 260, 30, 0.2f, 0.2f, 0.2f, 1.0f);
    renderer_draw_rect(renderer, px + 19, py + 39, 262, 32, 0.6f, 0.6f, 0.6f, 1.0f);
    renderer_draw_rect(renderer, px + 20, py + 40, 260, 30, 0.15f, 0.15f, 0.15f, 1.0f);

    renderer_draw_text(renderer, ui->sign_edit_text, px + 24, py + 44, 1.5f, 1.0f, 1.0f, 1.0f);

    if (ui->sign_edit_cursor_timer < 0.5f) {
        int text_w = renderer_text_width(renderer, ui->sign_edit_text, 1.5f);
        renderer_draw_rect(renderer, px + 24 + text_w, py + 42, 2, 24, 1.0f, 1.0f, 1.0f, 1.0f);
    }

    renderer_draw_text(renderer, "Enter: Save  Esc: Cancel", px + 20, py + 90, 1.0f, 0.7f, 0.7f, 0.7f);
}
```

Note: `ui.c` must include `../world/world.h` and `../game/interact.h` at the top. Check existing includes and add if missing:

```c
#include "../game/interact.h"
```

- [ ] **Step 3: Build and verify**

Run: `make clean && make`
Expected: Compiles with zero warnings. (Sign edit UI functions are defined but not yet called from main.c.)

- [ ] **Step 4: Commit**

```bash
git add src/engine/ui.c src/engine/ui.h
git commit -m "feat: add sign edit UI state with text input rendering"
```

---

### Task 5: Block textures for door, sign, portal (block_texture.c)

**Files:**
- Modify: `src/engine/block_texture.c`

- [ ] **Step 1: Add texture functions for door, sign, portal**

Add these three texture functions before the `tex_default` function (before line 382):

```c
static void tex_door(unsigned char *buf, int size)
{
    Prng p;
    prng_seed(&p, 888);
    px_fill(buf, size, 160, 110, 50, 255);
    for (int y = 0; y < size; y++) {
        px_set(buf, size, 2, y, 130, 85, 35, 255);
        px_set(buf, size, 3, y, 130, 85, 35, 255);
        px_set(buf, size, size - 3, y, 130, 85, 35, 255);
        px_set(buf, size, size - 4, y, 130, 85, 35, 255);
    }
    for (int x = 4; x < size - 4; x++) {
        px_set(buf, size, x, 4, 140, 95, 40, 255);
        px_set(buf, size, x, size / 2, 140, 95, 40, 255);
    }
    px_set(buf, size, size - 7, size / 2 - 2, 200, 180, 60, 255);
    px_set(buf, size, size - 7, size / 2 - 1, 200, 180, 60, 255);
    px_set(buf, size, size - 7, size / 2, 200, 180, 60, 255);
    px_noise(buf, size, &p, 160, 110, 50, 10, 0.15f);
}

static void tex_sign(unsigned char *buf, int size)
{
    Prng p;
    prng_seed(&p, 222);
    px_fill(buf, size, 0, 0, 0, 0);
    for (int y = size / 2; y < size; y++) {
        px_set(buf, size, size / 2 - 1, y, 140, 95, 40, 255);
        px_set(buf, size, size / 2, y, 140, 95, 40, 255);
    }
    for (int y = 2; y < size / 2 - 1; y++) {
        for (int x = 4; x < size - 4; x++) {
            px_set(buf, size, x, y, 220, 200, 160, 255);
        }
    }
    for (int x = 3; x < size - 3; x++) {
        px_set(buf, size, x, 2, 140, 95, 40, 255);
        px_set(buf, size, x, size / 2 - 2, 140, 95, 40, 255);
    }
    for (int y = 2; y < size / 2 - 1; y++) {
        px_set(buf, size, 4, y, 140, 95, 40, 255);
        px_set(buf, size, size - 5, y, 140, 95, 40, 255);
    }
    px_noise(buf, size, &p, 220, 200, 160, 12, 0.2f);
}

static void tex_portal(unsigned char *buf, int size)
{
    Prng p;
    prng_seed(&p, 6666);
    px_fill(buf, size, 140, 50, 200, 255);
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            int dx = x - size / 2;
            int dy = y - size / 2;
            int dist = dx * dx + dy * dy;
            if (dist < 64) {
                px_set(buf, size, x, y, 180, 80, 255, 255);
            } else if (dist < 144) {
                px_set(buf, size, x, y, 160, 60, 230, 255);
            }
        }
    }
    for (int i = 0; i < 5; i++) {
        int cx = prng_range(&p, 4, size - 4);
        int cy = prng_range(&p, 4, size - 4);
        for (int j = 0; j < 6; j++) {
            px_set(buf, size, cx + j, cy, 200, 120, 255, 255);
        }
    }
    px_noise(buf, size, &p, 140, 50, 200, 25, 0.3f);
}
```

- [ ] **Step 2: Update tex_dispatch table to use new textures**

In the `tex_dispatch` array, change entries for sprite IDs 8 (door), 22 (sign), and 26 (portal):

Change line `{8,   tex_wood},` to `{8,   tex_door},`
Change line `{22,  tex_wood},` to `{22,  tex_sign},`
Change line `{26,  tex_stone},` to `{26,  tex_portal},`

- [ ] **Step 3: Build and verify**

Run: `make clean && make`
Expected: Compiles with zero warnings.

- [ ] **Step 4: Commit**

```bash
git add src/engine/block_texture.c
git commit -m "feat: add procedural textures for door, sign, portal blocks"
```

---

### Task 6: Main.c integration -- game struct, punch/wrench dispatch, overlays (main.c)

**Files:**
- Modify: `src/main.c`

This is the largest task. It wires everything together.

- [ ] **Step 1: Add interact.h include**

Add after the existing includes (after line 19):

```c
#include "game/interact.h"
```

- [ ] **Step 2: Add fields to Game struct**

Add after `break_progress` (after line 41) in the Game struct:

```c
    float sign_overlay_timer;
    int sign_overlay_x, sign_overlay_y;
    int sign_overlay_active;
    int portal_link_pending;
    int portal_link_x, portal_link_y;
```

- [ ] **Step 3: Add sign edit key handler in game_handle_events**

In `game_handle_events`, after the F11 handler (after line 135, before the closing `}` of the while loop), add:

```c
        if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_RETURN) {
            if (g->ui.state == UI_STATE_SIGN_EDIT) {
                ui_finish_sign_edit(&g->ui, &g->world);
            }
        }
        if (e.type == SDL_TEXTINPUT && g->ui.state == UI_STATE_SIGN_EDIT) {
            if (g->ui.sign_edit_cursor < SIGN_TEXT_MAX_LEN) {
                int len = (int)strlen(e.text.text);
                if (len > 0 && g->ui.sign_edit_cursor + len <= SIGN_TEXT_MAX_LEN) {
                    g->ui.sign_edit_text[g->ui.sign_edit_cursor] = e.text.text[0];
                    g->ui.sign_edit_cursor++;
                }
            }
        }
        if (e.type == SDL_KEYDOWN && g->ui.state == UI_STATE_SIGN_EDIT) {
            if (e.key.keysym.sym == SDLK_BACKSPACE && g->ui.sign_edit_cursor > 0) {
                g->ui.sign_edit_cursor--;
                g->ui.sign_edit_text[g->ui.sign_edit_cursor] = '\0';
            }
        }
```

Note: also need to enable SDL text input. Add after `renderer_init` in `game_init` (after line 79):

```c
    SDL_StartTextInput();
```

- [ ] **Step 4: Add sign edit UI state handling in game_update**

In `game_update`, inside the `if (g->ui.state != UI_STATE_NONE)` block (after the inventory swap logic, before `input_update`), add:

```c
        if (g->ui.state == UI_STATE_SIGN_EDIT) {
            ui_update_sign_edit(&g->ui, &g->input);
        }
```

- [ ] **Step 5: Modify LMB handler for interactive block dispatch**

Replace the existing LMB breaking section (lines 299-351). The new version checks for interactive blocks on click, then falls through to breaking if holding a pickaxe:

```c
    if (input_is_mouse_clicked(&g->input, 1)) {
        int mouse_wx, mouse_wy;
        camera_screen_to_world(&g->camera, g->input.mouse_x, g->input.mouse_y, &mouse_wx, &mouse_wy);
        mouse_wx /= TILE_SIZE;
        mouse_wy /= TILE_SIZE;
        int dist_x = mouse_wx - px;
        int dist_y = mouse_wy - py;
        if (dist_x * dist_x + dist_y * dist_y <= 36) {
            Tile *t = world_get_tile(&g->world, mouse_wx, mouse_wy);
            if (t && block_is_interactive(t->fg)) {
                int tool_id = inventory_get_hotbar_item(&g->inventory, g->ui.hotbar_selection);
                if (tool_id != ITEM_PICKAXE) {
                    int result = interact_punch(&g->world, &g->player, mouse_wx, mouse_wy);
                    if (result == 2) {
                        const char *txt = interact_get_sign_text(&g->world, mouse_wx, mouse_wy);
                        if (txt) {
                            g->sign_overlay_active = 1;
                            g->sign_overlay_timer = 3.0f;
                            g->sign_overlay_x = mouse_wx;
                            g->sign_overlay_y = mouse_wy;
                        }
                    }
                    g->player.breaking = 0;
                    g->player.break_timer = 0;
                    input_update(&g->input);
                    goto skip_break;
                }
            }
        }
    }

    if (input_is_mouse_down(&g->input, 1)) {
        int mouse_wx, mouse_wy;
        camera_screen_to_world(&g->camera, g->input.mouse_x, g->input.mouse_y, &mouse_wx, &mouse_wy);
        mouse_wx /= TILE_SIZE;
        mouse_wy /= TILE_SIZE;
        int dist_x = mouse_wx - px;
        int dist_y = mouse_wy - py;
        if (dist_x * dist_x + dist_y * dist_y <= 36) {
            Tile *t = world_get_tile(&g->world, mouse_wx, mouse_wy);
            if (t && t->fg != BLOCK_AIR && t->fg != BLOCK_BEDROCK) {
                int tool_id = inventory_get_hotbar_item(&g->inventory, g->ui.hotbar_selection);
                if (block_is_interactive(t->fg) && tool_id != ITEM_PICKAXE) {
                    /* skip */
                } else if (g->player.breaking && g->player.break_x == mouse_wx && g->player.break_y == mouse_wy) {
                    g->player.break_timer += (int)(dt * 1000);
                    int break_time = block_get_break_time(t->fg);
                    int power = item_get_tool_power(tool_id);
                    if (power > 0) {
                        g->player.break_timer += (int)(power * dt * 1000);
                    }
                    if (g->player.break_timer >= break_time) {
                        uint16_t drop = block_get_drop(t->fg);
                        int count = block_get_drop_count(t->fg);
                        if (drop != 0 && count > 0) {
                            inventory_add(&g->inventory, drop, count);
                        }
                        if (t->growth_stage >= GROWTH_COMPLETE) {
                            uint16_t drops[8];
                            int dcounts[8];
                            int ndrops = 0;
                            farming_harvest(&g->world, mouse_wx, mouse_wy, drops, dcounts, &ndrops);
                            for (int i = 0; i < ndrops; i++) {
                                inventory_add(&g->inventory, drops[i], dcounts[i]);
                            }
                            int gem_drop = 1 + (rand() % 3);
                            g->player.gems += gem_drop;
                        }
                        interact_cleanup_break(&g->world, mouse_wx, mouse_wy);
                        t->fg = BLOCK_AIR;
                        t->growth_stage = 0;
                        t->growth_timer = 0;
                        t->extra_data = 0;
                        g->player.breaking = 0;
                        g->player.break_timer = 0;
                    }
                } else {
                    g->player.breaking = 1;
                    g->player.break_x = mouse_wx;
                    g->player.break_y = mouse_wy;
                    g->player.break_timer = 0;
                }
            }
        }
    } else {
        g->player.breaking = 0;
        g->player.break_timer = 0;
    }
    skip_break:
```

Note: `ITEM_PICKAXE` is defined as 9001 in `store.c`. Add a define at the top of main.c or reference it. Add near the top of main.c after the includes:

```c
#define ITEM_WRENCH 9000
#define ITEM_PICKAXE 9001
```

- [ ] **Step 6: Modify RMB handler for wrench dispatch**

Replace the existing RMB handler (lines 353-389). Add wrench detection before the existing place logic:

```c
    if (input_is_mouse_clicked(&g->input, 3)) {
        int mouse_wx, mouse_wy;
        camera_screen_to_world(&g->camera, g->input.mouse_x, g->input.mouse_y, &mouse_wx, &mouse_wy);
        mouse_wx /= TILE_SIZE;
        mouse_wy /= TILE_SIZE;
        int dist_x = mouse_wx - px;
        int dist_y = mouse_wy - py;
        if (dist_x * dist_x + dist_y * dist_y <= 36) {
            Tile *t = world_get_tile(&g->world, mouse_wx, mouse_wy);
            if (t) {
                int hotbar_slot = g->ui.hotbar_selection;
                uint16_t held = inventory_get_hotbar_item(&g->inventory, hotbar_slot);
                int held_count = inventory_get_hotbar_count(&g->inventory, hotbar_slot);

                if (held == ITEM_WRENCH && t->fg != BLOCK_AIR) {
                    int result = interact_wrench(&g->world, mouse_wx, mouse_wy,
                        &g->portal_link_x, &g->portal_link_y, &g->portal_link_pending);
                    if (result == 1) {
                        ui_init_sign_edit(&g->ui, &g->world, mouse_wx, mouse_wy);
                    }
                    input_update(&g->input);
                    return;
                }

                if (held != 0 && held_count > 0) {
                    const ItemDef *def = item_get_def(held);
                    if (def && def->is_seed && t->growth_stage >= GROWTH_STAGE_1 && t->growth_stage < GROWTH_COMPLETE) {
                        uint16_t tile_seed = (uint16_t)t->extra_data;
                        uint16_t result;
                        if (crafting_splice(held, tile_seed, &result) == 0) {
                            farming_plant_seed(&g->world, mouse_wx, mouse_wy, result);
                            inventory_remove(&g->inventory, held, 1);
                        }
                    } else if (def && t->fg == BLOCK_AIR) {
                        if (def->is_seed) {
                            if (farming_can_plant(&g->world, mouse_wx, mouse_wy)) {
                                farming_plant_seed(&g->world, mouse_wx, mouse_wy, held);
                                inventory_remove(&g->inventory, held, 1);
                            }
                        } else if (def->category == ITEM_CAT_BLOCK) {
                            t->fg = held;
                            t->extra_data = 0;
                            inventory_remove(&g->inventory, held, 1);
                        }
                    }
                }
            }
        }
    }
```

- [ ] **Step 7: Update sign overlay timer**

After the camera update and before autosave (after line 401, before line 404), add:

```c
    if (g->sign_overlay_active) {
        g->sign_overlay_timer -= dt;
        if (g->sign_overlay_timer <= 0) {
            g->sign_overlay_active = 0;
        }
    }
```

- [ ] **Step 8: Add sign overlay rendering in game_render**

In `game_render`, inside the `if (g->ui.state == UI_STATE_NONE)` block (after the block tooltip rendering, before the closing `}`), add sign overlay rendering:

```c
        if (g->sign_overlay_active) {
            const char *txt = interact_get_sign_text(&g->world, g->sign_overlay_x, g->sign_overlay_y);
            if (txt) {
                int sox, soy;
                camera_world_to_screen(&g->camera, g->sign_overlay_x * TILE_SIZE, g->sign_overlay_y * TILE_SIZE, &sox, &soy);
                int tw = renderer_text_width(&g->renderer, txt, 1.5f);
                int th = 16;
                int label_x = sox + TILE_SIZE / 2 - tw / 2;
                int label_y = soy - th - 8;
                renderer_draw_rect(&g->renderer, label_x - 4, label_y - 2, tw + 8, th + 6, 0.0f, 0.0f, 0.0f, 0.8f);
                renderer_draw_text(&g->renderer, txt, label_x, label_y, 1.5f, 1.0f, 1.0f, 1.0f);
            }
        }
```

- [ ] **Step 9: Add sign edit screen rendering**

In `game_render`, add a new `else if` after the store rendering block (after line 515):

```c
    } else if (g->ui.state == UI_STATE_SIGN_EDIT) {
        ui_render_sign_edit(&g->ui, &g->renderer);
```

- [ ] **Step 10: Add portal animation in tile rendering**

In the tile rendering loop in `game_render`, after drawing the portal sprite (inside the `else` block where `t->growth_stage == 0`), add portal color modulation. Find where `renderer_draw_tile` is called for non-growing foreground blocks and add after the draw:

```c
                    if (t->fg == BLOCK_PORTAL) {
                        float pulse = 0.5f + 0.5f * sinf((float)SDL_GetTicks() / 300.0f);
                        renderer_draw_rect(&g->renderer, sx, sy, TILE_SIZE, TILE_SIZE,
                            0.6f * pulse, 0.2f * pulse, 0.9f * pulse, 0.3f);
                    }
```

This requires `#include <math.h>` which is already included in main.c.

- [ ] **Step 11: Build and verify**

Run: `make clean && make`
Expected: Compiles with zero warnings.

- [ ] **Step 12: Test interactively**

Run: `./growtopia`
Test:
1. Buy a door from store (B key, Special tab), place it, left-click to toggle open/close
2. Buy a sign, place it, wrench + click to edit text, type text, Enter to save. Left-click to read.
3. Buy two portals, place them, wrench + click first, wrench + click second to link. Left-click to teleport.
4. Break each interactive block with pickaxe equipped.

- [ ] **Step 13: Commit**

```bash
git add src/main.c
git commit -m "feat: wire up interactive blocks in game loop (punch, wrench, overlays, portal animation)"
```

---

### Task 7: Code review and polish

**Files:**
- All modified files

- [ ] **Step 1: Build clean**

Run: `make clean && make`
Expected: Zero warnings, zero errors.

- [ ] **Step 2: Dispatch code review subagent**

Dispatch a code review agent with the full diff to check:
- UI click detection positions match rendering positions
- Font scales are readable (1.0+)
- Collision changes don't break grounding
- Save/load compatibility
- No hardcoded screen dimensions

- [ ] **Step 3: Fix any issues found**

Address reviewer feedback.

- [ ] **Step 4: Final commit**

```bash
git add -A
git commit -m "fix: address code review feedback for interactive blocks"
```
