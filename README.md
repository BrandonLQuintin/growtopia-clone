# Growtopia Clone

An offline 2D sandbox game inspired by Growtopia, built in C with SDL2 and OpenGL.

## Features

- **2D tile-based world** with procedural generation (caves, trees, terrain layers)
- **Block breaking/placing** with break progress animation
- **30+ block types** with unique procedural pixel-art textures and properties
- **17 seed types** for farming with growth stages and harvest mechanics
- **Seed splicing** - combine two seeds in inventory to create new seed types (17 recipes)
- **Offline store** with 5 categories (Blocks, Seeds, Tools, Clothing, Special)
- **Full inventory** (36 slots) with click-to-swap and hotbar
- **World/inventory/player save/load** to binary files with auto-save
- **Resizable window** and fullscreen support
- **Procedural pixel-art textures** - generated at startup, no external image files needed

## Controls

| Key | Action |
|-----|--------|
| WASD / Arrows | Move |
| Space / W | Jump |
| Left Mouse | Break block (hold) |
| Right Mouse | Place block / Plant seed |
| 1-9 | Select hotbar slot |
| E | Toggle inventory |
| B | Toggle store |
| F5 | Manual save |
| F11 | Toggle fullscreen |
| ESC | Close menu / Quit |

## Seed Splicing

Open your inventory (E), click one seed, then click another seed. If the combination matches a recipe, both seeds are consumed and you get a new seed type. Some recipes:

- Dirt Seed + Grass Seed = Wood Seed
- Dirt Seed + Stone Seed = Brick Seed
- Sand Seed + Sand Seed = Cactus Seed
- Grass Seed + Grass Seed = Flower Seed
- Wood Seed + Wood Seed = Leaves Seed

## Build & Run

### Prerequisites

- GCC (C11 support)
- SDL2 development libraries
- OpenGL development libraries
- Make

### Install dependencies (Arch Linux)

```bash
sudo pacman -S sdl2 mesa
```

### Install dependencies (Ubuntu/Debian)

```bash
sudo apt install libsdl2-dev libgl-dev gcc make
```

### Build

```bash
make
```

### Run

```bash
./growtopia
```

### Clean build

```bash
make clean
```

## Project Structure

```
src/
  main.c              Game loop, input handling, rendering
  engine/
    renderer.h/c      OpenGL init, drawing primitives, bitmap font, texture atlas
    camera.h/c        2D camera with smooth follow
    input.h/c         Keyboard/mouse state tracking
    ui.h/c            HUD, inventory, and store screen rendering
    block_texture.h/c Procedural 32x32 pixel-art block textures
    prng.h/c          Seeded deterministic PRNG
  world/
    world.h/c         World grid, save/load, procedural generation
    block.h/c         Block type definitions and properties
    items.h/c         Item database with all items, seeds, tools
  game/
    player.h/c        Player physics and collision
    inventory.h/c     Inventory slot management
    farming.h/c       Seed planting, growth, and harvesting
    store.h/c         Offline shop with categories and pricing
    crafting.h/c      Seed splice recipes
res/
  worlds/             Saved world and player data
```

## Save Data

World data is stored in `res/worlds/`:
- `main.wld` - World tile data (binary)
- `main.inv` - Player inventory (binary)
- `main.player` - Player position, gems, health (binary)

The game auto-saves every 60 seconds and on exit.

## License

MIT
