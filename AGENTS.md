# AGENTS.md

## Build Commands

```bash
make              # Build the project
make clean        # Clean build artifacts
make run          # Build and run
```

## Dependencies

- SDL2 (`libsdl2-dev`)
- OpenGL (`libgl-dev`)
- GCC with C11 support
- Make

## Project Architecture

This is a Growtopia-inspired 2D sandbox game in C using SDL2 + OpenGL.

### Key Files

- `src/main.c` - Game loop, all gameplay logic (movement, collision, breaking, placing, store buying)
- `src/engine/renderer.c` - OpenGL rendering, bitmap font, drawing primitives
- `src/engine/camera.c` - 2D camera with smooth follow, world/screen coordinate transforms
- `src/engine/ui.c` - HUD, hotbar, inventory screen, store screen rendering and interaction
- `src/engine/input.c` - Keyboard/mouse state (pressed vs held distinction)
- `src/world/world.c` - World grid, procedural generation, binary save/load
- `src/world/block.c` - Block definitions array (30+ blocks with colors, properties)
- `src/world/items.c` - Item definitions (blocks, seeds, tools, clothing, currency)
- `src/game/player.c` - Player physics (gravity, velocity, animation)
- `src/game/inventory.c` - 36-slot inventory with add/remove/swap/save/load
- `src/game/farming.c` - Seed planting, growth tick, harvesting
- `src/game/store.c` - Offline shop with 5 categories
- `src/game/crafting.c` - Seed splice recipe table (17 recipes, used by in-world splicing)

### Rendering

- All blocks are procedural colored rectangles (no texture files needed)
- Text uses a built-in 5x7 bitmap font rendered via GL_QUADS
- Font scale of 1.0 = ~5x7px per character. Readable scales start at 1.0+
- Screen size is dynamic: `g_screen_w` / `g_screen_h` (from `renderer.h`), updated each frame

### Physics & Collision

- Player position (x,y) = bottom-center of the 24x48px player rectangle
- `player_update()` applies gravity only when not grounded, moves by velocity*dt
- Collision resolution happens in `main.c` game_update via minimum-overlap AABB vs tile grid
- `on_ground` flag is set by collision resolution, NOT by player_update

### World Format

- 100x60 tile grid, each tile: foreground block, background block, growth stage, growth timer
- Binary save format: "GROW" magic + version(1) + width + height + raw tile array

### UI System

- `UI_STATE_NONE` / `UI_STATE_INVENTORY` / `UI_STATE_STORE`
- Slots have both click detection (`ui_update`) and rendering (`ui_render_*`) - positions MUST match
- Seed splicing: hold a seed, right-click a growing plant (growth_stage 1-4) to splice via `crafting_splice()`

### Important Constants

- `TILE_SIZE = 32` pixels
- `PLAYER_WIDTH = 24`, `PLAYER_HEIGHT = 48`
- `INVENTORY_SIZE = 36`, `HOTBAR_SIZE = 9`
- Block IDs: 0-127 foreground, 128-255 background, 256-320 seeds
- Tool IDs: 9000+ (Wrench=9000, Pickaxe=9001)
- Clothing IDs: 9100+ (Hat=9100, Shirt=9101, Pants=9102)
- Gems ID: 9999

## Code Review Workflow

Before presenting any changes to the user, follow this review process:

### 1. Spec Review (if a spec or plan exists)

If changes were made based on a spec (`docs/superpowers/specs/`) or a written plan, summon a spec reviewer agent first. It should verify:

- All requirements from the spec/plan are implemented
- No spec items were missed or partially implemented
- The implementation matches the spec's architecture and data flow
- No contradictions between the spec and the actual code

### 2. Code Review

Summon a code reviewer agent to check the diff. It should verify:

- `make clean && make` compiles with zero warnings (`-Wall -Wextra`)
- No regressions in existing functionality
- New code follows project conventions (see Coding Conventions above)
- UI click detection positions match rendering positions (common bug source in this project)
- Font scales are readable (1.0+ for any displayed text)
- Collision/physics changes don't break player grounding or cause jitter
- Save/load compatibility isn't broken by struct changes
- No hardcoded `SCREEN_WIDTH`/`SCREEN_HEIGHT` - use `g_screen_w`/`g_screen_h`

### 3. Present to User

Only after both reviews pass (or issues are fixed), present the changes to the user.

## Coding Conventions

- C11 standard, compiled with `-Wall -Wextra`
- No comments in code
- All headers use `#ifndef` include guards
- Functions prefixed by module name (e.g. `renderer_*`, `camera_*`, `inventory_*`)
- Use `snake_case` for everything
- Keep main.c as the orchestrator; game logic in `src/game/`, engine in `src/engine/`, data in `src/world/`
