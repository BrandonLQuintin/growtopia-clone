# Growtopia Clone - Design Specification

## Overview

An offline 2D sandbox game inspired by Growtopia, built in C with SDL2 and OpenGL. The player explores a grid-based world, breaks and places blocks, farms seeds, earns currency, and shops at an offline store. All data persists to disk.

## Architecture

### Technology Stack
- **Language:** C11
- **Window/Input:** SDL2
- **Rendering:** OpenGL 2.1+ (2D textured quads via sprite atlas)
- **Build:** Makefile with gcc
- **Data:** Binary file persistence for worlds, JSON-like plain text for item DB

### Project Structure
```
src/
  main.c            - Entry point, game loop
  engine/
    renderer.h/c    - OpenGL init, texture loading, sprite batch drawing
    camera.h/c      - 2D camera following player, world-to-screen transforms
    input.h/c       - Keyboard/mouse state tracking
    ui.h/c          - UI primitives: buttons, panels, text, inventory grid
  world/
    world.h/c       - World grid, block access, save/load to binary file
    block.h/c       - Block type definitions, block properties
    items.h/c       - Item database: all block types, seeds, tools, clothing
  game/
    player.h/c      - Player entity, position, velocity, animation state
    inventory.h/c   - Inventory slots, add/remove items, hotbar
    farming.h/c     - Seed planting, growth stages, harvesting, drops
    store.h/c       - Offline shop: buy/sell items with gems
    crafting.h/c    - Splicing seeds, combining items
  data/
    items_db.h/c    - Static item definitions (generated from data file)
res/
  textures/         - Sprite atlas PNGs
  worlds/           - Saved world files
```

## Core Systems

### 1. Rendering Engine
- Initialize SDL2 OpenGL context at 1280x720
- Load sprite atlas texture(s) as OpenGL textures
- Batch-render all visible tiles as textured quads
- Render UI overlay on top (orthographic projection)
- Sprite atlas: single texture with all block/item sprites arranged in a grid

### 2. Camera
- Follows player with smooth lerp
- Viewport shows ~40x24 tiles at 32px tile size
- Clamps to world boundaries

### 3. World System
- Grid: 100 tiles wide x 60 tiles tall (Growtopia-like proportions)
- Each cell: block ID (uint16) + block variant/flags (uint8) + growth timer (uint32)
- Main world + a "base" layer (background blocks)
- World generation: dirt background, grass on top, stone below, bedrock at bottom
- Save format: binary blob with header (magic, version, width, height) + block data
- Auto-save every 60 seconds; manual save on exit

### 4. Block/Item System
Every item has:
- ID (uint16), name, category (block/seed/tool/clothing/consumable)
- For blocks: isSolid, breakTime, dropItem, rarity
- For seeds: growTime, harvestItem, harvestCount, treeType
- For tools: power, speed modifier
- Items defined in a static C header as const arrays (no external parsing needed)

### 5. Player
- 2-tile-tall sprite (like Growtopia character)
- Grid-aligned movement with smooth interpolation
- Physics: gravity, jumping, collision with solid blocks
- Punch (break block) and place actions with cooldowns
- Can interact with blocks (open door, harvest tree)

### 6. Inventory
- 36-slot inventory (4 rows of 9, like Growtopia)
- Hotbar: bottom row of 9 slots, selectable with 1-9 keys
- Each slot: itemID + count
- Stack sizes: blocks stack to 200, seeds to 200, gems to 9999
- Drag-and-drop in inventory screen
- Inventory saved as part of player data

### 7. Farming System
- Place seed on valid soil (dirt/blank block)
- Seed grows through stages: planted -> sprout -> sapling -> tree (4 stages)
- Each stage has a distinct sprite
- Growth time varies per seed type (30s-5min)
- When fully grown, punching tree drops harvest items + 0-3 seeds
- Seed splicing: combine two seeds to create new block seed types

### 8. Store System
- Accessible via a special "store" block or hotkey
- Categories: Blocks, Seeds, Tools, Clothing, Special
- Currency: Gems (earned from breaking blocks, harvesting, daily bonus)
- Buy items at set prices; sell items at 50% of buy price
- All items available (no lockout/rotation - it's offline)
- Store UI: scrollable grid of items with gem costs

### 9. UI System
- HUD: hotbar at bottom, gem count, health (optional)
- Inventory screen: grid overlay, item tooltips
- Store screen: categorized shop with buy/sell
- Text rendering via bitmap font texture
- Mouse click handling for UI elements
- ESC closes open screens

### 10. World Saving/Loading
- Worlds saved to `res/worlds/<name>.wld`
- Player data (position, inventory, gems) saved alongside world
- On startup: load last world or generate new
- World names: player can have multiple saved worlds

## Data Definitions

### Item Categories
1. **Blocks:** Dirt, Stone, Wood Door, Lava, Water, Grass, Brick, Glass, etc.
2. **Seeds:** All block seeds (dirt seed, stone seed, wood seed, etc.)
3. **Tools:** Fist (default), Wrench, Pickaxe variants
4. **Clothing:** Hat, Shirt, Pants (cosmetic only for v1)
5. **Currency:** Gems

### Block Properties
- Solid vs non-solid (pass-through)
- Foreground vs background layer
- Breakable vs unbreakable (bedrock)
- Interactable (doors, signs, store block)

## Game Loop
```
1. Process input (SDL events)
2. Update game state:
   a. Player physics & movement
   b. Block interactions (breaking/placing)
   c. Farming tick (update growth timers)
   d. Camera update
   e. Auto-save timer
3. Render:
   a. Clear screen
   b. Render background blocks
   c. Render foreground blocks + growing trees
   d. Render player
   e. Render UI (HUD, open screens)
4. Swap buffers, cap at 60 FPS
```

## Scope Boundaries
- **In scope:** Single player, offline, all core sandbox mechanics, farming, store, world save/load
- **Out of scope for v1:** Multiplayer, accounts, procedural world events, music/sound, mod support
