# Interactive Blocks Design: Doors, Signs, Portals

## Overview

Add three interactive block types to the game that respond to player actions beyond simple break/place. These blocks use the existing `extra_data` field on tiles for state storage and introduce a wrench-activation pattern for advanced interactions.

## Blocks

### Door (BLOCK_DOOR = 8)

**Behavior:**
- Left-click (punch) toggles open/closed state
- When closed: solid block, player cannot walk through
- When open: non-solid, player walks through freely
- A single tile, not multi-tile

**Data storage:**
- `extra_data` bit 0: 0 = closed, 1 = open

**Rendering:**
- Closed: full door texture (brown wooden door rectangle)
- Open: thin side strip on one edge (4px wide), rest is air

**Collision:**
- `block_is_solid` must be tile-aware for doors: check `extra_data & 1`
- New function `world_is_solid_for_tile(World*, x, y)` that reads the tile's extra_data

### Sign (BLOCK_SIGN = 22)

**Behavior:**
- Left-click (punch) to read: displays text as a tooltip overlay above the block for 3 seconds
- Wrench + click to edit: opens sign text editing UI
- Maximum 32 characters per sign, using the existing bitmap font character set

**Data storage:**
- `extra_data` stores an index into a world-level string table
- World struct gains: `char sign_texts[64][33]` (64 signs max, 32 chars + null terminator)
- Index 0 means no text (unused sign)

**Rendering:**
- Sign post: brown wooden post with small square sign board on top
- Text rendered on the sign board surface when reading (tooltip overlay mode)

### Portal (BLOCK_PORTAL = 26)

**Behavior:**
- Left-click (punch) on a linked portal: teleports player to the linked partner
- Wrench + click on first portal: stores pending link state
- Wrench + click on second portal: links both portals bidirectionally
- Unlinked portals (extra_data == 0): punch does nothing, shows "Unlinked Portal" tooltip
- Cannot link a portal to itself
- Cannot link to a tile that doesn't contain a portal

**Data storage:**
- `extra_data` packed as: `((x + 1) << 16) | (y + 1)` where x,y are tile coordinates (0-based, offset by 1 so 0 means unlinked)
- `0` means unlinked (no valid portal has x=-1 or y=-1 after the offset)
- Bidirectional: both tiles store each other's coordinates

**Rendering:**
- Purple portal block with oscillating brightness (sine wave on color intensity)
- Animated swirl effect: cycle the purple hue between (140,50,200) and (180,80,255) using sin(time)
- Unlinked portals rendered slightly dimmer than linked ones

## New Module: src/game/interact.c/h

Functions:
- `interact_punch(World*, Player*, int tx, int ty)` -- handles left-click on interactive blocks (door toggle, sign read, portal teleport)
- `interact_wrench(World*, UI*, int tx, int ty)` -- handles wrench click on interactive blocks (sign edit, portal link)
- `interact_get_sign_text(World*, int tx, int ty)` -- returns const char* to sign text at tile, or NULL
- `interact_alloc_sign(World*, int tx, int ty)` -- finds or allocates a sign text slot, returns 1-based index (1-64) stored in extra_data; internally accesses sign_texts[index-1]; returns 0 on failure (all slots used)
- `interact_link_portals(World*, int x1, int y1, int x2, int y2)` -- bidirectional link
- `interact_is_portal_linked(World*, int tx, int ty)` -- returns 1 if extra_data != 0 (linked)
- `interact_cleanup_break(World*, int tx, int ty)` -- handles sign text deallocation and portal unlinking when an interactive block is broken
- `block_is_interactive(uint16_t block_id)` -- returns 1 if block_id is BLOCK_DOOR, BLOCK_SIGN, or BLOCK_PORTAL

## Changes to Existing Modules

### block.c/h

- Add `block_is_solid_with_data(uint16_t block_id, uint32_t extra_data)` that checks door state
- Existing `block_is_solid` continues to work for non-interactive blocks (returns static is_solid value)
- Door's static `is_solid` in BLOCK_DEFS changes to 1 (closed by default)
- `world_is_solid` in world.c must be updated to call `block_is_solid_with_data` instead of `block_is_solid`, reading the tile's extra_data for the solidity check
- The collision loop in main.c (line 230) already calls `world_is_solid`, so updating that function to be tile-aware is sufficient -- no change needed in main.c collision code

### world.c/h

- World struct gains `sign_texts` array: `char sign_texts[64][33]`
- World struct gains `int sign_count` tracking how many slots are used
- `world_save`: append sign data after tile array, bump version to 2
- `world_load`: detect version, change version check from `!= 1` to `> 2` to accept both v1 and v2, load sign data if version >= 2, default empty if version 1
- New save format: `GROW | version(2) | width | height | tiles[] | sign_count | sign_texts[sign_count][]`
- Backward compatible: version 1 loads fine, signs default to empty

### ui.c/h

- Add `UI_STATE_SIGN_EDIT` to UIState enum (value 4, after existing CRAFTING=3)
- UI struct gains: `int sign_edit_x, sign_edit_y` (target sign tile coords), `char sign_edit_text[33]` (editing buffer), `int sign_edit_cursor` (character position), `float sign_edit_cursor_timer` (blink timer, 500ms interval)
- Sign edit rendering layout:
  - Panel: 300x120 pixels, centered on screen (`(g_screen_w - 300) / 2, (g_screen_h - 120) / 2`)
  - Dark semi-transparent background (0,0,0,0.85)
  - Title "Edit Sign" at top (x+10, y+8), font scale 2.0f
  - Text field: x+20, y+40, width 260, height 30, dark gray background (0.2,0.2,0.2,1.0), light border (0.6,0.6,0.6,1.0)
  - Text content rendered inside field at (x+24, y+44), font scale 1.5f, max 21 visible characters at this scale
  - Cursor: 2px wide white vertical bar at end of text position, blinking (visible for 500ms, hidden for 500ms)
  - Prompt "Enter: Save  Esc: Cancel" at (x+20, y+90), font scale 1.0f
- Sign edit update: capture keyboard input (A-Z, a-z, 0-9, space, punctuation from bitmap font, backspace, enter, escape)
- `ui_init_sign_edit(UI*, World*, int tx, int ty)` populates edit buffer with existing sign text, resets cursor
- `ui_finish_sign_edit(UI*, World*)` writes buffer back to world sign table and closes UI to UI_STATE_NONE

### main.c

**Punch handler (LMB on interactive blocks):**
In the breaking code block, before starting a break: check if the target block is DOOR, SIGN, or PORTAL. If so, call `interact_punch` instead of breaking.

Specifically:
- DOOR: toggle and skip breaking entirely
- SIGN: display text overlay, skip breaking
- PORTAL: teleport if linked, skip breaking

**Wrench handler (RMB while holding Wrench tool):**
In the right-click handler, check if the held item is the Wrench (id 9000). If so, check the target tile:
- SIGN: open sign edit UI (`UI_STATE_SIGN_EDIT`)
- PORTAL: initiate or complete portal linking
- Other blocks: no action

The wrench detection goes before the existing place/seed logic.

**Sign text overlay:**
When `interact_get_sign_text` returns non-NULL, render the text above the clicked sign tile for 3 seconds. Track overlay state with a timer in Game struct.

**Portal link state:**
Game struct gains `int portal_link_pending` flag and `int portal_link_x, portal_link_y` for the first portal in a link operation.

### renderer/block_texture.c

- Door closed texture: brown wood rectangle with panel lines
- Door open texture: thin vertical strip on left edge
- Sign texture: brown post + small light square board
- Portal texture: purple base with lighter swirl pattern

Rendering dispatch in game_render: after drawing the tile sprite, check for interactive states:
- Door with extra_data & 1: draw open variant instead
- Portal: modulate color with sin(time)

## Data Flow

### Punch (LMB) on tile with interactive block:

```
Player clicks LMB on tile
  -> main.c checks tile fg block type
  -> if DOOR: call interact_punch -> toggle extra_data bit 0 -> skip breaking
  -> if SIGN: call interact_punch -> set sign overlay timer + text -> skip breaking
  -> if PORTAL: call interact_punch -> check linked -> teleport player -> skip breaking
  -> else: normal break logic (unchanged)
```

### Wrench (RMB with Wrench held) on tile:

```
Player clicks RMB while holding Wrench
  -> main.c detects wrench tool (item_id == 9000)
  -> if SIGN: ui_init_sign_edit -> UI_STATE_SIGN_EDIT
  -> if PORTAL and not pending: store first coords, set pending flag
  -> if PORTAL and pending: interact_link_portals(bidirectional), clear pending
  -> else: no action
```

### Sign Edit UI:

```
UI_STATE_SIGN_EDIT active
  -> ui_update captures keypresses into sign_edit_text buffer
  -> Enter: ui_finish_sign_edit writes to world, closes to UI_STATE_NONE
  -> Escape: discard changes, close to UI_STATE_NONE
```

## Save Format Change

Version 1 (current): `GROW(4) | version=1(4) | width(4) | height(4) | tiles[width*height]`

Version 2 (new): `GROW(4) | version=2(4) | width(4) | height(4) | tiles[width*height] | sign_count(4, uint32_t) | sign_texts[sign_count](sign_count * 33 fixed bytes per entry)`

Loading logic: read version, if 1 then sign_count = 0 (no sign data follows), if 2 then read sign_count and sign_texts array.

## Edge Cases

- Breaking a door/sign/portal: See the detailed mechanic above. Summary: click fires interaction, hold with Pickaxe breaks.
- Breaking interactive blocks: The punch interaction fires on `input_is_mouse_clicked(button 1)` (first frame only). If the player holds the Pickaxe tool and continues holding LMB, break progress proceeds via `input_is_mouse_down` on subsequent frames -- the interaction does NOT fire again. If no Pickaxe is held, holding LMB on an interactive block does nothing beyond the initial interaction. The wrench does not break blocks. Implementation: in the break handler, check `input_is_mouse_clicked` for interactive block dispatch, then check `input_is_mouse_down` for break progress only if player holds Pickaxe and target is an interactive block.
- Portal linking to itself: rejected (no-op)
- Portal linking to non-portal tile: rejected (no-op)
- Sign text buffer full (64 signs): reject new sign text allocation, wrench click shows no effect
- World with version 1 save: loads fine, all interactive blocks work with empty/default state (doors closed, signs empty, portals unlinked)

## Cleanup on Break

When an interactive block is broken (fg set to BLOCK_AIR), cleanup must occur:

- **Sign**: if `extra_data != 0`, clear `sign_texts[extra_data - 1]` to empty string and set tile's `extra_data = 0`
- **Portal**: if linked (`extra_data != 0`), decode partner coordinates `(px, py)` from the breaking tile's extra_data, then set the partner tile's `extra_data = 0` (unlink partner bidirectionally), then set breaking tile's `extra_data = 0`
- **Door**: no cleanup needed (no external state)

This cleanup is called from `interact_cleanup_break(World*, tx, ty)` in the break completion code in main.c, after the tile fg is set to BLOCK_AIR.

## Sign Text Overlay

Game struct gains: `float sign_overlay_timer`, `int sign_overlay_x`, `int sign_overlay_y`, `int sign_overlay_active`

When `interact_punch` is called on a sign:
1. Set `game.sign_overlay_x = tx`, `game.sign_overlay_y = ty`
2. Set `game.sign_overlay_timer = 3.0f` (3 seconds)
3. Set `game.sign_overlay_active = 1`

In `game_update`: decrement `sign_overlay_timer` by dt, set `sign_overlay_active = 0` when timer reaches 0.

In `game_render` (after tile rendering, before UI): if `sign_overlay_active`, get sign text via `interact_get_sign_text`, convert sign tile world position to screen coordinates, render text above the tile:
- Background: semi-transparent black box (0,0,0,0.8) centered above the tile, sized to fit text
- Text: font scale 1.5f, white color, positioned so text is centered horizontally above the tile

## Sign Edit Key Handling in main.c

In `game_handle_events`, add special handling for `UI_STATE_SIGN_EDIT`:
- Enter key: call `ui_finish_sign_edit(&ui, &world)` to save text and close
- Escape key: already handled by the generic `ui_close_all` at line 119-122 (discards changes)
- Other keys are captured by `ui_update` during the sign edit state

## Constants

```
SIGN_TEXT_MAX_LEN  32
SIGN_TABLE_SIZE    64
PORTAL_UNLINKED    0
INTERACTIVE_BLOCKS: BLOCK_DOOR, BLOCK_SIGN, BLOCK_PORTAL
```

## Files Modified

- `src/game/interact.c` (new) -- interactive block logic
- `src/game/interact.h` (new) -- interact function declarations
- `src/world/world.c` -- sign table, save/load v2
- `src/world/world.h` -- sign table in World struct, new function
- `src/world/block.c` -- dynamic solidity function
- `src/world/block.h` -- new function declaration
- `src/engine/ui.c` -- sign edit UI state
- `src/engine/ui.h` -- UI_STATE_SIGN_EDIT, new struct fields
- `src/engine/block_texture.c` -- door/sign/portal textures
- `src/main.c` -- interaction dispatch in punch/wrench handlers, sign overlay, portal link state
- `Makefile` -- no change needed; `$(wildcard $(SRCDIR)/**/*.c)` auto-discovers new files
