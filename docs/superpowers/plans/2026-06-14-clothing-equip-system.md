# Clothing Equip System Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make Hat/Shirt/Pants clothing items equippable, visually rendered on the player, and persisted across sessions.

**Architecture:** Equip state stored on Player struct as three uint16_t fields. UI uses existing slot system extended to 39 slots (36 inventory + 3 equip). Swap logic routes through a dedicated handler when equip slots are involved. Save format extended with backward-compatible 6-byte append.

**Tech Stack:** C11, SDL2, OpenGL, Make

**Spec:** `docs/superpowers/specs/2026-06-14-clothing-equip-system-design.md`

---

## File Structure

| File | Responsibility |
|------|---------------|
| `src/game/player.h` | Add `equipped_hat/shirt/pants` fields to Player struct |
| `src/world/items.h` | Add `ITEM_HAT/SHIRT/PANTS` constants, `item_clothing_slot()` declaration |
| `src/world/items.c` | Implement `item_clothing_slot()` |
| `src/game/inventory.h` | Update profile save/load signatures with `equipped[3]` param |
| `src/game/inventory.c` | Read/write equipped fields, backward-compatible load |
| `src/engine/renderer.c` | Draw clothing overlays in `renderer_draw_player()` |
| `src/engine/ui.h` | Update `ui_render_inventory_screen` signature with `equipped[3]` param |
| `src/engine/ui.c` | Render equip slot column, widen panel, update close button click area |
| `src/game/store.c` | Remove local `#define`s, use shared constants from `items.h` |
| `src/main.c` | `handle_equip_swap()`, swap logic routing, wire equipped through save/load/UI |

---

## Task 1: Data Model + Constants + Store Cleanup

**Files:**
- Modify: `src/game/player.h:14-28`
- Modify: `src/world/items.h:52-57`
- Modify: `src/world/items.h:45`
- Modify: `src/world/items.c:152-155`
- Modify: `src/game/store.c:7-9`

This task adds the foundational data structures and helpers. No behavior change yet — just types, constants, and one helper function.

- [ ] **Step 1: Add equipped fields to Player struct**

In `src/game/player.h`, add three fields after `walking` and before the closing brace:

```c
    int walking;
    uint16_t equipped_hat;
    uint16_t equipped_shirt;
    uint16_t equipped_pants;
} Player;
```

Note: `player_init()` already uses `memset(p, 0, sizeof(*p))` which zeroes these fields, so no change needed in `player.c`.

- [ ] **Step 2: Add clothing item constants to items.h**

In `src/world/items.h`, add defines after `ITEM_PICKAXE`:

```c
#define ITEM_WRENCH  9000
#define ITEM_PICKAXE 9001
#define ITEM_HAT     9100
#define ITEM_SHIRT   9101
#define ITEM_PANTS   9102
```

- [ ] **Step 3: Add item_clothing_slot declaration to items.h**

In `src/world/items.h`, add the declaration after `item_is_clothing`:

```c
int item_is_clothing(uint16_t item_id);
int item_clothing_slot(uint16_t item_id);
int item_is_tool(uint16_t item_id);
```

- [ ] **Step 4: Implement item_clothing_slot in items.c**

In `src/world/items.c`, add the function after `item_is_clothing` (after line 155):

```c
int item_is_clothing(uint16_t item_id) {
    const ItemDef *def = item_get_def(item_id);
    return def ? def->category == ITEM_CAT_CLOTHING : 0;
}

int item_clothing_slot(uint16_t item_id) {
    if (!item_is_clothing(item_id))
        return -1;
    if (item_id == ITEM_HAT)
        return 0;
    if (item_id == ITEM_SHIRT)
        return 1;
    if (item_id == ITEM_PANTS)
        return 2;
    return -1;
}
```

- [ ] **Step 5: Remove duplicated local defines in store.c**

In `src/game/store.c`, delete lines 7-9 (the local `#define`s). The file already includes `items.h` at line 4, so `ITEM_HAT`/`ITEM_SHIRT`/`ITEM_PANTS` will resolve to the shared constants.

Delete these three lines:
```c
#define ITEM_HAT 9100
#define ITEM_SHIRT 9101
#define ITEM_PANTS 9102
```

- [ ] **Step 6: Build and verify**

Run: `make clean && make`
Expected: Compiles with zero warnings.

- [ ] **Step 7: Commit**

```bash
git add src/game/player.h src/world/items.h src/world/items.c src/game/store.c
git commit -m "Add clothing equip data model, constants, and item_clothing_slot helper"
```

---

## Task 2: Save/Load Persistence

**Files:**
- Modify: `src/game/inventory.h:23-24`
- Modify: `src/game/inventory.c:93-127`
- Modify: `src/main.c:66`
- Modify: `src/main.c:101-104`

This task extends the player profile format to store equipped clothing, with backward compatibility for old save files.

- [ ] **Step 1: Update inventory.h signatures**

In `src/game/inventory.h`, replace the two profile function declarations:

Old:
```c
int inventory_save_profile(Inventory *inv, int gems, int health, const char *path);
int inventory_load_profile(Inventory *inv, int *gems, int *health, const char *path);
```

New:
```c
int inventory_save_profile(Inventory *inv, int gems, int health, uint16_t equipped[3], const char *path);
int inventory_load_profile(Inventory *inv, int *gems, int *health, uint16_t equipped[3], const char *path);
```

- [ ] **Step 2: Update inventory_save_profile in inventory.c**

In `src/game/inventory.c`, replace the save function (lines 93-106):

```c
int inventory_save_profile(Inventory *inv, int gems, int health, uint16_t equipped[3], const char *path)
{
    FILE *f = fopen(path, "wb");
    if (!f)
        return -1;
    for (int i = 0; i < INVENTORY_SIZE; i++) {
        fwrite(&inv->items[i], sizeof(uint16_t), 1, f);
        fwrite(&inv->counts[i], sizeof(int), 1, f);
    }
    fwrite(&gems, sizeof(int), 1, f);
    fwrite(&health, sizeof(int), 1, f);
    fwrite(equipped, sizeof(uint16_t), 3, f);
    fclose(f);
    return 0;
}
```

- [ ] **Step 3: Update inventory_load_profile in inventory.c**

In `src/game/inventory.c`, replace the load function (lines 108-127):

```c
int inventory_load_profile(Inventory *inv, int *gems, int *health, uint16_t equipped[3], const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f)
        return -1;
    for (int i = 0; i < INVENTORY_SIZE; i++) {
        if (fread(&inv->items[i], sizeof(uint16_t), 1, f) != 1 ||
            fread(&inv->counts[i], sizeof(int), 1, f) != 1) {
            fclose(f);
            return -1;
        }
    }
    if (fread(gems, sizeof(int), 1, f) != 1 ||
        fread(health, sizeof(int), 1, f) != 1) {
        fclose(f);
        return -1;
    }
    if (fread(equipped, sizeof(uint16_t), 3, f) != 3) {
        equipped[0] = 0;
        equipped[1] = 0;
        equipped[2] = 0;
    }
    fclose(f);
    return 0;
}
```

- [ ] **Step 4: Update game_save_all in main.c**

In `src/main.c`, replace line 66:

Old:
```c
    inventory_save_profile(&g->inventory, g->player.gems, g->player.health, PROFILE_PATH);
```

New:
```c
    uint16_t equipped[3] = {g->player.equipped_hat, g->player.equipped_shirt, g->player.equipped_pants};
    inventory_save_profile(&g->inventory, g->player.gems, g->player.health, equipped, PROFILE_PATH);
```

- [ ] **Step 5: Update game_enter_world in main.c**

In `src/main.c`, replace lines 101-104:

Old:
```c
    int profile_loaded = 0;
    if (inventory_load_profile(&g->inventory, &g->player.gems, &g->player.health, PROFILE_PATH) == 0) {
        profile_loaded = 1;
    }
```

New:
```c
    int profile_loaded = 0;
    uint16_t equipped[3] = {0, 0, 0};
    if (inventory_load_profile(&g->inventory, &g->player.gems, &g->player.health, equipped, PROFILE_PATH) == 0) {
        g->player.equipped_hat = equipped[0];
        g->player.equipped_shirt = equipped[1];
        g->player.equipped_pants = equipped[2];
        profile_loaded = 1;
    }
```

- [ ] **Step 6: Build and verify**

Run: `make clean && make`
Expected: Compiles with zero warnings.

- [ ] **Step 7: Commit**

```bash
git add src/game/inventory.h src/game/inventory.c src/main.c
git commit -m "Add equipped clothing to player profile save/load"
```

---

## Task 3: Player Rendering Overlays

**Files:**
- Modify: `src/engine/renderer.c:3` (add include)
- Modify: `src/engine/renderer.c:1058-1065`

This task draws colored overlay rectangles on the player when clothing is equipped. No interaction yet — just visual feedback.

- [ ] **Step 1: Add items.h include to renderer.c**

In `src/engine/renderer.c`, add after line 3 (`#include "../world/block.h"`):

```c
#include "../world/items.h"
```

- [ ] **Step 2: Add clothing overlays to renderer_draw_player**

In `src/engine/renderer.c`, replace the `renderer_draw_player` function (lines 1058-1065):

```c
void renderer_draw_player(Renderer *r, Player *p, Camera *cam) {
    int psx, psy;
    camera_world_to_screen(cam, p->x - PLAYER_WIDTH / 2, p->y - PLAYER_HEIGHT, &psx, &psy);
    renderer_draw_rect(r, psx, psy, PLAYER_WIDTH, PLAYER_HEIGHT,
        1.0f, 0.8f, 0.6f, 1.0f);
    if (p->equipped_pants) {
        int cr, cg, cb;
        item_get_color(p->equipped_pants, &cr, &cg, &cb);
        renderer_draw_rect(r, psx, psy + 36, PLAYER_WIDTH, 12,
            cr / 255.0f, cg / 255.0f, cb / 255.0f, 1.0f);
    }
    if (p->equipped_shirt) {
        int cr, cg, cb;
        item_get_color(p->equipped_shirt, &cr, &cg, &cb);
        renderer_draw_rect(r, psx, psy + 20, PLAYER_WIDTH, 16,
            cr / 255.0f, cg / 255.0f, cb / 255.0f, 1.0f);
    }
    if (p->equipped_hat) {
        int cr, cg, cb;
        item_get_color(p->equipped_hat, &cr, &cg, &cb);
        renderer_draw_rect(r, psx, psy, PLAYER_WIDTH, 8,
            cr / 255.0f, cg / 255.0f, cb / 255.0f, 1.0f);
    }
    renderer_draw_rect(r, psx + 4, psy + 4, 6, 6, 0.0f, 0.0f, 0.0f, 1.0f);
    renderer_draw_rect(r, psx + 14, psy + 4, 6, 6, 0.0f, 0.0f, 0.0f, 1.0f);
}
```

Layer order: body → pants (bottom 12px) → shirt (middle 16px) → hat (top 8px) → eyes (last, always visible on top).

- [ ] **Step 3: Build and verify**

Run: `make clean && make`
Expected: Compiles with zero warnings.

- [ ] **Step 4: Commit**

```bash
git add src/engine/renderer.c
git commit -m "Render clothing overlays on player character"
```

---

## Task 4: UI Equip Slot Rendering

**Files:**
- Modify: `src/engine/ui.h:67`
- Modify: `src/engine/ui.c:75-76` (close button click area)
- Modify: `src/engine/ui.c:228-304` (inventory screen render)
- Modify: `src/main.c:401`

This task adds the visual equip slot column to the inventory screen. The slots are clickable (via existing `ui_update` loop) but the swap logic won't handle them yet — that comes in Task 5.

- [ ] **Step 1: Update ui_render_inventory_screen signature in ui.h**

In `src/engine/ui.h`, replace line 67:

Old:
```c
void ui_render_inventory_screen(UI *ui, Renderer *renderer, uint16_t *inv_items, int *inv_counts, int inv_size);
```

New:
```c
void ui_render_inventory_screen(UI *ui, Renderer *renderer, uint16_t *inv_items, int *inv_counts, int inv_size, uint16_t equipped[3]);
```

- [ ] **Step 2: Update close button click area in ui_update**

In `src/engine/ui.c`, replace lines 75-76 inside the `UI_STATE_INVENTORY` block of `ui_update`:

Old:
```c
        int close_x = (g_screen_w + INV_COLS * SLOT_SIZE) / 2 + 4;
        int close_y = (g_screen_h - INV_ROWS * SLOT_SIZE) / 2 - 44;
```

New:
```c
        int cp_w = INV_COLS * SLOT_SIZE + SLOT_SIZE + 30;
        int cp_h = INV_ROWS * SLOT_SIZE + 56;
        int close_x = (g_screen_w - cp_w) / 2 + cp_w - 28;
        int close_y = (g_screen_h - cp_h) / 2 + 6;
```

These values must match the panel dimensions used in `ui_render_inventory_screen` (see next step). `cp_w = grid_w + SLOT_SIZE + 3*panel_pad` and `cp_h = grid_h + title_h + 2*panel_pad`.

- [ ] **Step 3: Rewrite ui_render_inventory_screen with equip slots**

In `src/engine/ui.c`, replace the entire `ui_render_inventory_screen` function (lines 228-304):

```c
void ui_render_inventory_screen(UI *ui, Renderer *renderer, uint16_t *inv_items, int *inv_counts, int inv_size, uint16_t equipped[3])
{
    renderer_draw_rect(renderer, 0, 0, g_screen_w, g_screen_h, 0.0f, 0.0f, 0.0f, 0.6f);

    int grid_w = INV_COLS * SLOT_SIZE;
    int grid_h = INV_ROWS * SLOT_SIZE;
    int panel_pad = 10;
    int title_h = 36;
    int panel_w = grid_w + SLOT_SIZE + panel_pad * 3;
    int panel_h = grid_h + title_h + panel_pad * 2;
    int panel_x = (g_screen_w - panel_w) / 2;
    int panel_y = (g_screen_h - panel_h) / 2;

    renderer_draw_rect(renderer, panel_x, panel_y, panel_w, panel_h,
        0.1f, 0.1f, 0.1f, 0.95f);
    renderer_draw_rect(renderer, panel_x, panel_y, panel_w, 2, 0.4f, 0.4f, 0.4f, 1.0f);
    renderer_draw_rect(renderer, panel_x, panel_y + panel_h - 2, panel_w, 2, 0.0f, 0.0f, 0.0f, 1.0f);
    renderer_draw_rect(renderer, panel_x, panel_y, 2, panel_h, 0.4f, 0.4f, 0.4f, 1.0f);
    renderer_draw_rect(renderer, panel_x + panel_w - 2, panel_y, 2, panel_h, 0.0f, 0.0f, 0.0f, 1.0f);

    int title_w = renderer_text_width(renderer, "Inventory", 2.0f);
    renderer_draw_text(renderer, "Inventory",
        panel_x + (panel_w - title_w) / 2, panel_y + 6, 2.0f, 1.0f, 1.0f, 1.0f);

    int close_x = panel_x + panel_w - 28;
    int close_y = panel_y + 6;
    renderer_draw_rect(renderer, close_x, close_y, 20, 20, 0.6f, 0.15f, 0.15f, 1.0f);
    renderer_draw_text(renderer, "X", close_x + 6, close_y + 3, 1.5f, 1.0f, 1.0f, 1.0f);

    int equip_x = panel_x + panel_pad;
    int grid_x = panel_x + SLOT_SIZE + panel_pad * 2;
    int grid_y = panel_y + title_h + panel_pad;

    ui->slot_count = 0;
    for (int i = 0; i < inv_size && ui->slot_count < UI_MAX_SLOTS; i++) {
        int col = i % INV_COLS;
        int row = i / INV_COLS;
        int sx = grid_x + col * SLOT_SIZE;
        int sy = grid_y + row * SLOT_SIZE;

        UISlot *s = &ui->slots[ui->slot_count];
        s->x = sx;
        s->y = sy;
        s->size = SLOT_SIZE;
        s->item_id = inv_items[i];
        s->count = inv_counts[i];
        s->selected = (ui->drag_from_slot == i);
        ui->slot_count++;

        int is_sel = (ui->drag_from_slot == i);
        render_slot_bg(renderer, sx, sy, SLOT_SIZE, is_sel, s->hovered);
        render_slot_item(renderer, sx, sy, SLOT_SIZE, inv_items[i], inv_counts[i]);
    }

    static const char *equip_labels[3] = {"H", "S", "P"};
    for (int i = 0; i < 3 && ui->slot_count < UI_MAX_SLOTS; i++) {
        int sx = equip_x;
        int sy = grid_y + i * SLOT_SIZE;

        UISlot *s = &ui->slots[ui->slot_count];
        s->x = sx;
        s->y = sy;
        s->size = SLOT_SIZE;
        s->item_id = equipped[i];
        s->count = equipped[i] ? 1 : 0;
        s->selected = (ui->drag_from_slot == 36 + i);
        ui->slot_count++;

        int is_sel = (ui->drag_from_slot == 36 + i);
        render_slot_bg(renderer, sx, sy, SLOT_SIZE, is_sel, s->hovered);
        if (equipped[i]) {
            render_slot_item(renderer, sx, sy, SLOT_SIZE, equipped[i], 1);
        } else {
            int lw = renderer_text_width(renderer, equip_labels[i], 2.0f);
            renderer_draw_text(renderer, equip_labels[i],
                sx + (SLOT_SIZE - lw) / 2, sy + (SLOT_SIZE - 14) / 2,
                2.0f, 0.5f, 0.5f, 0.5f, 1.0f);
        }
    }

    for (int i = 0; i < ui->slot_count; i++) {
        if (ui->slots[i].hovered && ui->slots[i].item_id != 0) {
            const char *name = item_get_name((uint16_t)ui->slots[i].item_id);
            if (name) {
                char label[64];
                if (item_is_seed((uint16_t)ui->slots[i].item_id)) {
                    snprintf(label, sizeof(label), "%s (Seed)", name);
                } else {
                    snprintf(label, sizeof(label), "%s", name);
                }
                int tw = renderer_text_width(renderer, label, 1.5f);
                int th = 12;
                int tx = ui->slots[i].x + SLOT_SIZE / 2 - tw / 2;
                int ty = ui->slots[i].y - th - 8;
                if (tx < 2) tx = 2;
                if (tx + tw + 6 > g_screen_w) tx = g_screen_w - tw - 6;
                if (ty < 2) ty = ui->slots[i].y + SLOT_SIZE + 4;
                renderer_draw_rect(renderer, tx - 4, ty - 2, tw + 8, th + 6, 0.0f, 0.0f, 0.0f, 0.85f);
                renderer_draw_text(renderer, label, tx, ty, 1.5f, 1.0f, 1.0f, 1.0f);
            }
            break;
        }
    }
}
```

- [ ] **Step 4: Update ui_render_inventory_screen call site in main.c**

In `src/main.c`, replace line 401:

Old:
```c
        ui_render_inventory_screen(&g->ui, &g->renderer, g->inventory.items, g->inventory.counts, INVENTORY_SIZE);
```

New:
```c
        uint16_t equipped[3] = {g->player.equipped_hat, g->player.equipped_shirt, g->player.equipped_pants};
        ui_render_inventory_screen(&g->ui, &g->renderer, g->inventory.items, g->inventory.counts, INVENTORY_SIZE, equipped);
```

- [ ] **Step 5: Build and verify**

Run: `make clean && make`
Expected: Compiles with zero warnings.

- [ ] **Step 6: Commit**

```bash
git add src/engine/ui.h src/engine/ui.c src/main.c
git commit -m "Render equip slot column in inventory screen"
```

---

## Task 5: Equip/Unequip Interaction

**Files:**
- Modify: `src/main.c:241-244` (swap logic) and add `handle_equip_swap` function

This task wires up the equip/unequip swap logic so clicking equip slots actually moves items between inventory and player equipped fields.

- [ ] **Step 1: Add handle_equip_swap static function in main.c**

In `src/main.c`, add this static function before `game_save_all` (before line 60). It handles swapping items between an inventory slot (0-35) and an equip slot (36-38):

```c
static int handle_equip_swap(Inventory *inv, Player *p, int a, int b)
{
    if (a >= 36 && b >= 36)
        return 0;

    int inv_idx, equip_idx;
    if (a >= 36) {
        equip_idx = a;
        inv_idx = b;
    } else {
        equip_idx = b;
        inv_idx = a;
    }

    uint16_t inv_item = inv->items[inv_idx];
    uint16_t equip_item;
    if (equip_idx == 36)
        equip_item = p->equipped_hat;
    else if (equip_idx == 37)
        equip_item = p->equipped_shirt;
    else
        equip_item = p->equipped_pants;

    if (inv_item != 0 && item_clothing_slot(inv_item) != (equip_idx - 36))
        return -1;

    if (equip_idx == 36)
        p->equipped_hat = inv_item;
    else if (equip_idx == 37)
        p->equipped_shirt = inv_item;
    else
        p->equipped_pants = inv_item;

    inv->items[inv_idx] = equip_item;
    inv->counts[inv_idx] = equip_item ? 1 : 0;

    return 0;
}
```

- [ ] **Step 2: Extend swap logic in main.c**

In `src/main.c`, replace the swap processing block (lines 241-244):

Old:
```c
        if (g->ui.state == UI_STATE_INVENTORY && g->ui.drag_from_slot >= 0 && g->ui.selected_slot >= 0 &&
            g->ui.drag_from_slot != g->ui.selected_slot) {
            inventory_swap_slots(&g->inventory, g->ui.drag_from_slot, g->ui.selected_slot);
            g->ui.drag_from_slot = -1;
        }
```

New:
```c
        if (g->ui.state == UI_STATE_INVENTORY && g->ui.drag_from_slot >= 0 && g->ui.selected_slot >= 0 &&
            g->ui.drag_from_slot != g->ui.selected_slot) {
            int a = g->ui.drag_from_slot;
            int b = g->ui.selected_slot;
            if (a >= 36 || b >= 36) {
                if (handle_equip_swap(&g->inventory, &g->player, a, b) < 0) {
                    g->ui.selected_slot = -1;
                }
            } else {
                inventory_swap_slots(&g->inventory, a, b);
            }
            g->ui.drag_from_slot = -1;
        }
```

- [ ] **Step 3: Build and verify**

Run: `make clean && make`
Expected: Compiles with zero warnings.

- [ ] **Step 4: Manual playtest**

Run: `make run`

Test the following:
1. Open store (place a Store block, click it), buy a Hat/Shirt/Pants from the CLOTHES tab
2. Press E to open inventory — verify 3 equip slots (H/S/P labels) appear to the left of the grid
3. Click the Hat item in inventory, then click the H slot — item should move to equip slot, player should show blue band on head
4. Repeat for Shirt (red torso) and Pants (navy lower body)
5. Click a filled equip slot, then click an empty inventory slot — item should move back to inventory, overlay disappears
6. Try clicking a Hat while the Shirt slot is selected — should be rejected (no swap)
7. Try putting a Dirt block in an equip slot — should be rejected
8. Exit world (ESC, confirm) and re-enter — equipped items should persist
9. Delete `res/worlds/player.dat`, start game — old save loads fine, equipped defaults to empty

- [ ] **Step 5: Commit**

```bash
git add src/main.c
git commit -m "Implement equip/unequip swap logic for clothing"
```
