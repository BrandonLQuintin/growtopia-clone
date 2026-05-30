# Seed Splice In-World Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace inventory-based seed splicing with in-world splicing — hold a seed, right-click a growing plant.

**Architecture:** The right-click handler in `main.c` gains a new branch for growing tiles. The existing recipe table and `crafting_splice()` are reused unchanged. The inventory splice code and UI hint are removed.

**Tech Stack:** C11, SDL2, OpenGL (no new dependencies)

---

### Task 1: Remove inventory-based splice from main.c

**Files:**
- Modify: `src/main.c:173-194`

- [ ] **Step 1: Replace the splice block with a simple swap**

In `src/main.c`, replace lines 173-194 (the entire `if (g->ui.state == UI_STATE_INVENTORY && g->ui.drag_from_slot >= 0 ...)` block) with:

```c
        if (g->ui.state == UI_STATE_INVENTORY && g->ui.drag_from_slot >= 0 && g->ui.selected_slot >= 0 &&
            g->ui.drag_from_slot != g->ui.selected_slot) {
            inventory_swap_slots(&g->inventory, g->ui.drag_from_slot, g->ui.selected_slot);
            g->ui.drag_from_slot = -1;
        }
```

- [ ] **Step 2: Remove the crafting.h include**

In `src/main.c`, remove line 19:

```c
#include "game/crafting.h"
```

(We'll re-add it in Task 2 when we add the in-world splice.)

- [ ] **Step 3: Build and verify**

Run: `make clean && make`
Expected: Compiles with zero warnings. Game runs, inventory click-to-swap still works.

- [ ] **Step 4: Commit**

```bash
git add src/main.c
git commit -m "Remove inventory-based seed splicing from main.c"
```

---

### Task 2: Add in-world splice to the right-click handler in main.c

**Files:**
- Modify: `src/main.c:368-397`

- [ ] **Step 1: Re-add the crafting.h include**

At the top of `src/main.c`, add back:

```c
#include "game/crafting.h"
```

- [ ] **Step 2: Rewrite the right-click block to handle growing tiles**

Replace the entire right-click block (the `if (input_is_mouse_clicked(&g->input, 3)) { ... }` block, approximately lines 368-397) with:

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
                            inventory_remove(&g->inventory, held, 1);
                        }
                    }
                }
            }
        }
    }
```

Key logic order: first check if tile is growing and held is seed (splice attempt). If not growing, fall through to the original air-tile planting logic.

- [ ] **Step 3: Build and verify**

Run: `make clean && make`
Expected: Compiles with zero warnings.

- [ ] **Step 4: Commit**

```bash
git add src/main.c
git commit -m "Add in-world seed splicing via right-click on growing plants"
```

---

### Task 3: Remove UI hint and dead code

**Files:**
- Modify: `src/engine/ui.c:297-302`
- Modify: `src/game/crafting.h:13`
- Modify: `src/game/crafting.c:38-43`

- [ ] **Step 1: Remove the splice hint from ui.c**

In `src/engine/ui.c`, remove lines 297-302 (the block that renders "Click 2 seeds to splice them!"):

```c
    {
        const char *hint = "Click 2 seeds to splice them!";
        int hw = renderer_text_width(renderer, hint, 1.0f);
        renderer_draw_text(renderer, hint, panel_x + (panel_w - hw) / 2, panel_y + panel_h + 6, 1.0f, 0.8f, 0.8f, 0.5f);
    }
```

- [ ] **Step 2: Remove crafting_get_recipes from crafting.h**

In `src/game/crafting.h`, remove line 13:

```c
int crafting_get_recipes(Recipe **out_recipes, int *out_count);
```

- [ ] **Step 3: Remove crafting_get_recipes from crafting.c**

In `src/game/crafting.c`, remove lines 38-43:

```c
int crafting_get_recipes(Recipe **out_recipes, int *out_count)
{
    *out_recipes = recipes;
    *out_count = (int)RECIPE_COUNT;
    return 0;
}
```

- [ ] **Step 4: Build and verify**

Run: `make clean && make`
Expected: Compiles with zero warnings.

- [ ] **Step 5: Commit**

```bash
git add src/engine/ui.c src/game/crafting.h src/game/crafting.c
git commit -m "Remove inventory splice hint and unused crafting_get_recipes"
```

---

### Task 4: Update AGENTS.md

**Files:**
- Modify: `AGENTS.md`

- [ ] **Step 1: Update the UI System section**

In `AGENTS.md`, find this line under "### UI System":

```
- Seed splicing: when two seeds are clicked in inventory, `crafting_splice()` checks recipes
```

Replace with:

```
- Seed splicing: hold a seed, right-click a growing plant (growth_stage 1-4) to splice via `crafting_splice()`
```

Also update the Key Files description. Find:

```
- `src/game/crafting.c` - Seed splice recipe table (17 recipes)
```

Replace with:

```
- `src/game/crafting.c` - Seed splice recipe table (17 recipes, used by in-world splicing)
```

- [ ] **Step 2: Commit**

```bash
git add AGENTS.md
git commit -m "Update AGENTS.md for in-world seed splicing"
```

---

### Task 5: Final build verification

- [ ] **Step 1: Clean build**

Run: `make clean && make`
Expected: Zero warnings, zero errors.

- [ ] **Step 2: Run the game**

Run: `make run`
Expected: Game launches. Test: plant a seed, wait for growth stage 1+, hold a different seed, right-click the growing plant. If recipe matches, plant is replaced with spliced seed at stage 1. If no match, nothing happens.
