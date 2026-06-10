# World Search Screen Design

## Overview

A Growtopia-style world search feature. On startup, the player sees a search screen where they type a world name. If the world exists on disk, it loads. If not, a new world is generated with that name. The player can return to the search screen from gameplay via ESC.

## Architecture: GameState Machine

A new `GameState` enum wraps the existing game loop:

```c
typedef enum {
    GAME_STATE_MENU,
    GAME_STATE_PLAYING
} GameState;
```

The main loop checks `game_state`:
- **MENU**: Renders the world search UI, handles text input, world enumeration. No gameplay processing.
- **PLAYING**: The existing game loop (movement, physics, UI, etc.) unchanged.

### Transitions

- Game starts in `GAME_STATE_MENU`
- Selecting/creating a world -> `GAME_STATE_PLAYING` (load world + shared player profile)
- ESC during gameplay when `UI_STATE_NONE` -> confirmation dialog -> save world + player -> `GAME_STATE_MENU`
- ESC during inventory/store/sign edit -> closes UI (existing behavior, unchanged)
- ESC from menu -> quit game

## World Search Screen UI

Centered panel with dark background overlay:

**Top:** Title text "SEARCH WORLD" in large font

**Middle:** Text input box:
- Alphanumeric only (A-Z, a-z, 0-9), forced to lowercase on input
- Max 20 characters
- Blinking cursor (reuses sign-edit cursor blink pattern)
- Shows typed text in real-time
- Backspace to delete

**Below input:** Recent worlds (up to 8 clickable buttons):
- Loaded from `res/worlds/recent.txt` (one name per line, newest first)
- Clicking a button fills the input box with that world name
- Buttons rendered as outlined rectangles with centered text

**Bottom:** "ENTER" button
- If `res/worlds/<name>.wld` exists: load world, load shared player profile, transition to PLAYING
- If not found: generate new world with that name, save it, create player profile if needed, transition to PLAYING

## File System

### Shared Player Profile: `res/worlds/player.dat`

Single binary file for the shared player state across all worlds:
- 36 inventory slots: for each slot, uint16_t item_id + int count (216 bytes)
- int gems (4 bytes)
- int health (4 bytes)
- Total: 224 bytes

### Per-World Files: `res/worlds/<name>.wld`

Extended binary format:
```
"GROW"              4 bytes magic
uint32_t version    version = 3
int width           world width (100)
int height          world height (60)
char name[64]       world name string (NEW in v3)
float spawn_x       player spawn X (NEW in v3)
float spawn_y       player spawn Y (NEW in v3)
Tile[width*height]  raw tile array
uint32_t sign_count
sign_texts[]        sign text entries
```

Version 3 adds: name (64 bytes) + spawn position (8 bytes) between the header and tile array.
Loading version 2 files still works: name derived from filename, spawn at surface center (y=25 * TILE_SIZE, x=50 * TILE_SIZE).

### Recent Worlds: `res/worlds/recent.txt`

Plain text, one world name per line, max 8 entries, newest first. Updated each time a world is entered.

### World Name Validation

- Alphanumeric only (A-Z, a-z, 0-9), forced to lowercase on input
- 1-20 characters
- Empty input is ignored (ENTER does nothing)
- Names are case-insensitive (stored and matched as lowercase)

## ESC Return & Exit Confirmation

When in `GAME_STATE_PLAYING` and `UI_STATE_NONE`:
- ESC press -> show in-game confirmation dialog overlay: "EXIT WORLD? (Y/N)"
- Y -> save current world + shared player profile -> transition to `GAME_STATE_MENU`
- N or ESC again -> dismiss dialog, resume playing

When any UI is open (inventory, store, sign edit):
- ESC -> close UI (existing behavior, unchanged)

When in `GAME_STATE_MENU`:
- ESC -> quit the game entirely

The confirmation dialog is a simple boolean flag (`int exit_confirm_active`) in the game struct, not a new UI state. It renders a centered overlay with the Y/N prompt.

## Startup Flow

1. SDL + Renderer init (window, OpenGL)
2. Start in `GAME_STATE_MENU`
3. Render world search screen with empty input, load recent worlds from `recent.txt`
4. Player types a name, presses ENTER or clicks ENTER button
5. Validate name (alphanumeric, 1-20 chars)
6. Check if `res/worlds/<name>.wld` exists
7. If exists: load world, load shared player profile from `player.dat`. If `player.dat` doesn't exist but world does: create profile with starter items + 999999 gems + 100 health. Set player spawn from world's spawn_x/spawn_y (or surface center for v2 files).
8. If not found: generate new world, set world name, save it. If `player.dat` doesn't exist: create with starter items + 999999 gems + 100 health. Player spawns at surface center.
9. Add world name to top of `recent.txt` (deduplicated, max 8)
10. Ensure `res/worlds/` directory exists before any file operations
11. Transition to `GAME_STATE_PLAYING`

## New Files

- `src/engine/world_select.h` - World search screen interface
- `src/engine/world_select.c` - World search screen rendering, input handling, world enumeration, recent worlds management

## Modified Files

### `src/main.c`
- Add `GameState` enum and `game_state` variable
- Add `char current_world_name[64]` to Game struct (set in `game_enter_world()`, used by save/shutdown/autosave)
- Add `int exit_confirm_active` flag to Game struct
- Restructure main loop: menu path vs gameplay path based on `game_state`
- Add ESC confirmation dialog logic for PLAYING state
- Create `game_enter_world(const char *name)` function that: calls `world_free()` on existing world if any, loads/generates world by name, loads shared player profile (or creates starter), sets player spawn from world data, re-initializes camera bounds with new world dimensions, snaps camera to spawn position, stores name in `current_world_name`, adds to recent list, sets `game_state = GAME_STATE_PLAYING`
- Update `game_save_all()` to use `current_world_name` for dynamic paths instead of hardcoded defines, and update `world.spawn_x = player.x`, `world.spawn_y = player.y` before saving
- Update `game_shutdown()` and autosave timer to use dynamic paths
- Remove hardcoded `WORLD_PATH`, `INVENTORY_PATH`, `PLAYER_PATH` defines
- Ensure `res/worlds/` directory is created on startup

### `src/world/world.h`
- Add `float spawn_x, spawn_y` to World struct

### `src/world/world.c`
- Update `world_save()` to write version 3 format (name + spawn pos), and update version guard in `world_load()` from `version > 2` to `version > 3`
- Update `world_load()` to handle version 3 (read name + spawn pos between header and tile array)
- Add `world_exists(const char *name)` - checks if `res/worlds/<name>.wld` exists
- Add `world_build_path(char *buf, int buf_size, const char *name, const char *ext)` - constructs `res/worlds/<name>.<ext>`
- Add `world_set_name(World *w, const char *name)` - copies name into `w->name[64]`

### `src/game/inventory.c`
- Add `inventory_save_profile(Inventory *inv, int gems, int health, const char *path)` - saves to `player.dat`
- Add `inventory_load_profile(Inventory *inv, int *gems, int *health, const char *path)` - loads from `player.dat`

### `src/engine/renderer.h` / `src/engine/renderer.c`
- No changes needed. `renderer_draw_rect()` already handles filled rectangles (int x, int y, int w, int h, float colors). Outlines drawn with 1px-wide filled rects per existing pattern in ui.c.

### `Makefile`
- No change needed. `SOURCES = $(wildcard $(SRCDIR)/**/*.c)` already picks up new files in `src/engine/`

## Unchanged Modules

- Player physics (`src/game/player.c`)
- Camera (`src/engine/camera.c`)
- Input system (`src/engine/input.c`)
- Block definitions (`src/world/block.c`)
- Item definitions (`src/world/items.c`)
- Farming (`src/game/farming.c`)
- Store (`src/game/store.c`)
- Crafting (`src/game/crafting.c`)
- Block interaction (`src/game/interact.c`)
- Procedural textures (`src/engine/block_texture.c`)
- PRNG (`src/engine/prng.c`)
