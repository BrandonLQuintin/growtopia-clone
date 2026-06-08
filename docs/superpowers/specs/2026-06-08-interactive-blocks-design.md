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
- `interact_alloc_sign(World*, int tx, int ty)` -- finds or allocates a sign text slot, returns index
- `interact_link_portals(World*, int x1, int y1, int x2, int y2)` -- bidirectional link
- `interact_is_portal_linked(World*, int tx, int ty)` -- returns 1 if extra_data != 0 (unlinked)

## Changes to Existing Modules

### block.c/h

- Add `block_is_solid_with_data(uint16_t block_id, uint32_t extra_data)` that checks door state
- Existing `block_is_solid` continues to work for non-interactive blocks (returns static is_solid value)
- Door's static `is_solid` in BLOCK_DEFS changes to 1 (closed by default)

### world.c/h

- World struct gains `sign_texts` array: `char sign_texts[64][33]`
- World struct gains `int sign_count` tracking how many slots are used
- `world_save`: append sign data after tile array, bump version to 2
- `world_load`: detect version, load sign data if version >= 2, default empty if version 1
- New save format: `GROW | version(2) | width | height | tiles[] | sign_count | sign_texts[sign_count][]`
- Backward compatible: version 1 loads fine, signs default to empty

### ui.c/h

- Add `UI_STATE_SIGN_EDIT` to UIState enum (value 3)
- UI struct gains: `int sign_edit_x, sign_edit_y` (target sign tile coords), `char sign_edit_text[33]` (editing buffer)
- Sign edit rendering: centered panel with text input field, cursor blinking, "Enter to save / Esc to cancel" prompt
- Sign edit update: capture keyboard input (letters, numbers, backspace, enter, escape)
- `ui_init_sign_edit(UI*, World*, int tx, int ty)` populates edit buffer with existing sign text
- `ui_finish_sign_edit(UI*, World*)` writes buffer back to world sign table and closes UI

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

Version 2 (new): `GROW(4) | version=2(4) | width(4) | height(4) | tiles[width*height] | sign_count(4) | sign_texts[sign_count](sign_count * 33)`

Loading logic: read version, if 1 then sign_count = 0 (no sign data follows), if 2 then read sign_count and sign_texts array.

## Edge Cases

- Breaking a door/sign/portal with LMB hold while not clicking: should still be breakable by holding LMB. The punch interaction only fires on click (not hold). On the first frame of clicking an interactive block, the interaction fires. If the player keeps holding, breaking starts normally. This means `interact_punch` is called once on click, and breaking only proceeds if the player keeps holding past the interaction frame.
- Breaking interactive blocks: When punching an interactive block (LMB), the interaction fires immediately on click (door toggle, sign read, portal teleport). The block is never broken by punching. To remove an interactive block, the player must hold the Pickaxe tool and left-click-hold on it, which triggers normal break progress (no interaction fires). The wrench does not break blocks.
- Portal linking to itself: rejected (no-op)
- Portal linking to non-portal tile: rejected (no-op)
- Sign text buffer full (64 signs): reject new sign text allocation, wrench click shows no effect
- World with version 1 save: loads fine, all interactive blocks work with empty/default state (doors closed, signs empty, portals unlinked)

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
- `Makefile` -- add interact.o to build
