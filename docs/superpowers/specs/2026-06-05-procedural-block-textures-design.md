# Procedural Block Texture Atlas

## Overview

Replace flat colored rectangle blocks with medium-detail procedural pixel-art textures, generated at startup into a single GPU texture atlas. Each block type gets a unique 32x32 pixel pattern drawn entirely in code — no external image files needed.

## Architecture

### Texture Atlas

- Single power-of-2 texture: 512x512 pixels
- Grid layout: 16 columns, each cell is 32x32 pixels
- ~55 block definitions fit in ~4 rows
- Each `BlockDef.tile_sprite_id` maps to an atlas slot: `col = sprite_id % 16`, `row = sprite_id / 16`
- UV coordinates computed from sprite ID:
  ```
  u0 = (sprite_id % atlas_cols) * 32 / 512
  v0 = (sprite_id / atlas_cols) * 32 / 512
  u1 = u0 + 32 / 512
  v1 = v0 + 32 / 512
  ```

### Generation

- `renderer_generate_atlas()` runs once at startup
- Allocates a 512x512 RGBA pixel buffer (memset to transparent)
- Iterates all `BLOCK_DEFS`, calls `block_texture_generate()` for each sprite ID
- Each sprite ID dispatches to a per-type procedural draw function
- Seeds a deterministic PRNG per block type for identical output every run
- Uploads the full buffer to GPU as a single `GL_TEXTURE_2D` with `GL_NEAREST` filtering

### Rendering

- `renderer_draw_tile()` enables `GL_TEXTURE_2D`, binds atlas, draws a textured quad with correct UVs
- `renderer_draw_tile_scaled()` same UV logic with custom dimensions
- `renderer_draw_rect()` unchanged — used for UI, player, non-textured elements
- Tile border: 1px darker line on bottom and right edges drawn as a separate quad pass for grid separation

## New File: `src/engine/block_texture.c`

Contains all procedural texture generation logic:

- `block_texture_generate(int sprite_id, unsigned char *buffer, int size)` — fills a 32x32 RGBA buffer
- Dispatch table: sprite ID -> draw function pointer
- Per-type draw functions: `tex_dirt()`, `tex_stone()`, `tex_grass()`, `tex_brick()`, `tex_wood()`, `tex_sand()`, `tex_glass()`, `tex_bedrock()`, `tex_water()`, `tex_lava()`, `tex_ice()`, `tex_snow()`, etc.
- Shared seeded PRNG utility

### Texture Patterns

| Block | Pattern |
|-------|---------|
| Dirt | Brown noise base + darker pebbles + tiny root lines |
| Stone | Gray noise base + crack lines + lighter patches |
| Grass | Green top 25% with blade pixels + brown dirt below |
| Brick | Red-brown base + white mortar grid (offset per row) |
| Wood | Tan base + horizontal grain lines + dark knot circle |
| Sand | Tan noise base + wavy dune lines + grain speckles |
| Glass | Light blue base + white diagonal reflection + edge frame |
| Bedrock | Very dark noise + faint crack lines |
| Water | Blue base + lighter wave bands |
| Lava | Orange-red base + bright yellow-orange veins |
| Ice | Light blue + white diagonal reflection + subtle cracks |
| Snow | Near-white noise + faint blue shadows |
| Seeds | Small seed shape on green/brown background |

### Background Blocks

Background blocks (IDs 128+) render as a desaturated, dimmer version of their foreground counterpart — ~15-20% less saturation and brightness to create visual depth.

### Growth Stages

Seeds planted in-world (growth_stage 1-4):
- Base: dirt/soil texture rendered first
- Overlay: small plant sprite drawn as a second quad on top
- Stage 1: few green pixels (sprout)
- Stage 2: short stem + small leaves
- Stage 3: taller stem + more leaves
- Stage 4: full plant shape

## Renderer Changes (`src/engine/renderer.c`)

- `renderer_generate_atlas()` — implemented: allocates buffer, iterates block defs, generates textures, uploads to GPU
- `renderer_draw_tile()` — switch from flat colored quad to textured quad with atlas UVs
- `renderer_draw_tile_scaled()` — same UV logic with custom width/height
- `renderer_draw_rect()` — unchanged
- `Renderer` struct: `atlas_texture`, `atlas_cols`, `atlas_rows` (already partially present)

## Edge Cases

- Unknown block IDs fall back to flat magenta color (current behavior)
- Atlas regenerated from code each startup — no file dependencies
- `GL_NEAREST` filtering ensures crisp pixel-art rendering at any scale

## Compatibility

- No changes to world format, inventory format, or struct sizes
- No changes to camera, input, world, player, inventory, farming, crafting, or store modules
- Fully compatible with existing save files
- No new external dependencies
