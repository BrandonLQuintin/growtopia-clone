# Clothing Equip System

**Date:** 2026-06-14
**Status:** Approved
**Scope:** Make the existing Hat/Shirt/Pants clothing items functional: equippable via inventory UI, rendered as colored overlays on the player, and persisted across sessions.

## Problem

Clothing items (Hat=9100, Shirt=9101, Pants=9102) are fully defined in `items.c`, sold in the store (`store.c:57-60`), and have an `item_is_clothing()` helper (`items.c:152`) — but none of the equip/wear/render/persist half exists. Players can spend gems buying clothing that just permanently occupies an inventory slot with no way to wear, use, or benefit from it. `item_is_clothing()` is dead code with zero call sites outside `items.c`.

## Goals

- Players can equip/unequip clothing through the inventory screen using the existing select-then-click-to-swap pattern.
- Equipped clothing renders on the player character as colored overlay rectangles.
- Equipped items persist in `player.dat` across sessions.
- `item_is_clothing()` is used by the new validation logic (no longer dead code).
- Store clothing ID constants are shared from `items.h` instead of duplicated in `store.c`.

## Non-Goals

- No gameplay/stat effects (purely cosmetic).
- No new clothing items beyond existing Hat/Shirt/Pants.
- No right-click-to-equip shortcut (equip only through inventory screen).
- No pixel-art sprites (colored overlays only).
- No changes to world save format (`.wld`).
- No test harness (verification is build + manual playtest).

## Approach

**Approach A: Equip state on the Player struct.** Add three `uint16_t` equipped-item fields to `Player`. `renderer_draw_player()` already receives `Player*`, so rendering needs no signature change. The inventory UI extends its slot array to include 3 equip slots alongside the 36 inventory slots. Profile save/load appends 3 fields with backward-compatible defaults.

## Design

### Data Model

**Player struct** (`src/game/player.h`):

Add three fields to the `Player` struct:

```c
uint16_t equipped_hat;    /* 0 = unequipped, else item_id (9100) */
uint16_t equipped_shirt;  /* 0 = unequipped, else item_id (9101) */
uint16_t equipped_pants;  /* 0 = unequipped, else item_id (9102) */
```

`player_init()` sets all three to 0.

**Item constants** (`src/world/items.h`):

Add alongside existing `ITEM_WRENCH`/`ITEM_PICKAXE`:

```c
#define ITEM_HAT    9100
#define ITEM_SHIRT  9101
#define ITEM_PANTS  9102
```

**New helper** (`src/world/items.h` declaration, `src/world/items.c` implementation):

```c
int item_clothing_slot(uint16_t item_id);
/* Returns 0=hat, 1=shirt, 2=pants, -1=not clothing */
```

Implementation: if `!item_is_clothing(item_id)` return -1. Otherwise map by item ID: `ITEM_HAT`→0, `ITEM_SHIRT`→1, `ITEM_PANTS`→2. Falls back to -1 for unknown clothing IDs (future-proofing).

### UI Equip Slots

**Slot indices**: The UI `slots[]` array (`UI_MAX_SLOTS=64`) currently uses indices 0-35 for inventory. Indices 36-38 are reserved for equip slots:
- 36 = Hat slot
- 37 = Shirt slot
- 38 = Pants slot

**Layout**: The inventory panel widens to the left to accommodate a vertical column of 3 equip slots (each `SLOT_SIZE=48px`). The panel width increases by `SLOT_SIZE + panel_pad`. Equip slots are stacked vertically to the left of the grid, starting from the top row. Each slot displays a small label letter (H/S/P) when empty.

```
┌─────────────────────────────────────────┐
│              Inventory                   │
│  ┌──┐  ┌──┬──┬──┬──┬──┬──┬──┬──┐       │
│  │H │  │  │  │  │  │  │  │  │  │  ...   │
│  ├──┤  ├──┼──┼──┼──┼──┼──┼──┼──┤       │
│  │S │  │  │  │  │  │  │  │  │  │  ...   │
│  ├──┤  ├──┼──┼──┼──┼──┼──┼──┼──┤       │
│  │P │  │  │  │  │  │  │  │  │  │  ...   │
│  └──┘  └──┴──┴──┴──┴──┴──┴──┴──┘       │
│  equip    inventory grid (9x4)          │
└─────────────────────────────────────────┘
```

**Rendering** (`ui_render_inventory_screen` in `src/engine/ui.c`):

After rendering the 36 inventory slots, render 3 equip slots:
1. Compute `equip_x = grid_x - SLOT_SIZE - panel_pad` and `equip_y = grid_y`
2. For each equip slot (i=0..2): set `ui->slots[36+i]` with position, size, item_id from player equipped fields, count=1 (or 0 if unequipped)
3. Increment `ui->slot_count` to 39
4. Draw slot background (same `render_slot_bg`), draw item color swatch + label letter
5. Tooltips: same as inventory slots — hovering shows item name via `item_get_name()`

The function signature gains a parameter to pass equipped items:

```c
void ui_render_inventory_screen(UI *ui, Renderer *renderer,
    uint16_t *inv_items, int *inv_counts, int inv_size,
    uint16_t equipped[3]);
```

**Click handling** (`ui_update` in `src/engine/ui.c`):

The existing click detection loop (`ui_update` lines 59-74) already iterates `ui->slot_count` slots and sets `selected_slot`/`drag_from_slot`. No change needed there — equip slots at indices 36-38 are automatically included.

**Swap logic** (`main.c` lines 241-244):

The existing swap handler must be extended to detect equip slot involvement:

```c
if (g->ui.state == UI_STATE_INVENTORY && g->ui.drag_from_slot >= 0 &&
    g->ui.selected_slot >= 0 &&
    g->ui.drag_from_slot != g->ui.selected_slot) {

    int a = g->ui.drag_from_slot;
    int b = g->ui.selected_slot;

    if (a >= 36 || b >= 36) {
        /* Equip/unequip swap involving an equip slot */
        if (handle_equip_swap(&g->inventory, &g->player, a, b) < 0) {
            /* Validation failed (wrong item type) */
            g->ui.selected_slot = -1;
        }
    } else {
        inventory_swap_slots(&g->inventory, a, b);
    }

    g->ui.drag_from_slot = -1;
}
```

**`handle_equip_swap` logic** (new static function in `main.c`, returns 0 on success, -1 on validation failure):

Given slot indices `a` and `b`, where one is an inventory slot (0-35) and the other is an equip slot (36-38):

1. Determine which is inventory (`inv_idx`) and which is equip (`equip_idx`).
2. Get the item in the inventory slot: `inv_item = inventory.items[inv_idx]`.
3. Get the item in the equip slot: `equip_item = player->equipped_{hat|shirt|pants}` (mapped from `equip_idx - 36`).
4. **Validation**: The item that will end up in the equip slot after the swap is `inv_item`. If `inv_item != 0` and `item_clothing_slot(inv_item) != (equip_idx - 36)`, return -1 (non-matching type). This covers both directions: trying to equip a wrong-type item, or trying to unequip into a slot whose item would become the new equip (e.g., dirt block into hat slot).
5. **Swap**: 
   - Set the player equipped field to `inv_item` (will be 0 if inventory slot was empty = unequip).
   - Set `inventory.items[inv_idx] = equip_item`, count = 1 (or 0 if equip_item was 0).
   - Return 0.

If both `a` and `b` are equip slots (both >= 36), ignore — no swap between equip slots.

### Player Rendering

**`renderer_draw_player()`** (`src/engine/renderer.c:1058-1065`):

Draw layers in order (each layer only if the equipped field is non-zero):

1. **Body base** — existing skin rectangle (24x48, color 1.0/0.8/0.6)
2. **Pants overlay** — bottom 12px of player: `rect(psx, psy+36, 24, 12, pants_color)`. Drawn over the body's lower portion.
3. **Shirt overlay** — middle 16px: `rect(psx, psy+20, 24, 16, shirt_color)`. Covers torso area.
4. **Hat overlay** — top 8px: `rect(psx, psy, 24, 8, hat_color)`. Drawn BEFORE eyes so it forms a colored band on the head.
5. **Eyes** — existing two black 6x6 rects at `psy+4`. Drawn LAST so they remain visible on top of the hat.

Color lookup uses `item_get_color(equipped_id, &r, &g, &b)` from `items.c`, normalized to 0.0-1.0 for the renderer (divide by 255).

No changes to function signature needed — `renderer_draw_player` already receives `Player*`.

**Item color values** (from `items.c:56-58`):
- Hat: RGB(50, 50, 200) → dark blue
- Shirt: RGB(200, 50, 50) → red
- Pants: RGB(50, 50, 150) → navy

### Save/Load

**Profile format change** (`src/game/inventory.h` + `inventory.c`):

New signatures:

```c
int inventory_save_profile(Inventory *inv, int gems, int health,
                           uint16_t equipped[3], const char *path);
int inventory_load_profile(Inventory *inv, int *gems, int *health,
                           uint16_t equipped[3], const char *path);
```

`equipped[0]`=hat, `[1]`=shirt, `[2]`=pants.

**Write** (`inventory_save_profile`): After writing the existing 36 slots + gems + health, write 3 `uint16_t` values (6 bytes total).

**Read** (`inventory_load_profile`): After reading gems + health, attempt to read 3 `uint16_t` values. If `fread` fails (old file without this data), set all three to 0 and return success. This silently upgrades old saves on next write.

**Format layout**:
```
Bytes 0-215:   36 × (uint16_t item_id + int count) = 216 bytes
Bytes 216-219: int gems
Bytes 220-223: int health
Bytes 224-229: uint16_t equipped_hat + equipped_shirt + equipped_pants (NEW)
```

Old files are 224 bytes. New files are 230 bytes.

**main.c wiring**: Where `inventory_save_profile`/`load_profile` are called, build the `equipped[3]` array from `player.equipped_hat/shirt/pants` on save, and write back to the player struct on load.

### Store Cleanup

**`src/game/store.c`**: Remove local `#define ITEM_HAT 9100` / `ITEM_SHIRT 9101` / `ITEM_PANTS 9102` (lines 7-9). Use the shared constants from `items.h` instead.

## File Change Summary

| File | Change |
|------|--------|
| `src/game/player.h` | Add 3 `equipped_*` fields to `Player` struct |
| `src/game/player.c` | Initialize equipped fields to 0 in `player_init()` |
| `src/world/items.h` | Add `ITEM_HAT`/`ITEM_SHIRT`/`ITEM_PANTS` defines, `item_clothing_slot()` declaration |
| `src/world/items.c` | Implement `item_clothing_slot()` |
| `src/engine/ui.h` | Update `ui_render_inventory_screen` signature to accept `equipped[3]` |
| `src/engine/ui.c` | Render equip slots, extend `slot_count` to 39, tooltips for equip slots |
| `src/engine/renderer.c` | Draw clothing overlays in `renderer_draw_player()` |
| `src/game/inventory.h` | Update `inventory_save_profile`/`load_profile` signatures |
| `src/game/inventory.c` | Read/write equipped fields in profile save/load |
| `src/game/store.c` | Use shared item constants instead of local defines |
| `src/main.c` | Equip swap logic, wire equipped fields through save/load and UI rendering |

## Verification

1. `make clean && make` compiles with zero warnings (`-Wall -Wextra`)
2. Manual playtest:
   - Buy a Hat/Shirt/Pants from the store
   - Open inventory, click a clothing item, click the matching equip slot — item moves to equip slot
   - Verify player character renders with the colored overlay
   - Click a filled equip slot, click an empty inventory slot — item moves back
   - Try putting a hat in the shirt slot — should be rejected
   - Exit world and re-enter — equipped items should persist
   - Old `player.dat` (pre-clothing) loads without error, defaults to unequipped
