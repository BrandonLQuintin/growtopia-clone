# Clouds (Parallax Sky)

## Summary

Add drifting, parallax-scrolling clouds to the gameplay sky. Clouds are blocky rectangles matching the game's pixelated aesthetic, drawn behind all world tiles. Two depth layers (near + far) scroll slower than the camera to create a depth illusion, and each cloud drifts horizontally on its own. Cloud formations are generated deterministically from the world's seed, so a given world always shows the same clouds without requiring any save-format change.

## Requirements

- Clouds render in the gameplay sky (`GAME_STATE_PLAYING`) only, not on the menu/world-select screen
- Clouds are drawn behind all world tiles and the player (true background)
- Two parallax depth layers: a near layer and a far layer
- Clouds scroll slower than the camera to fake depth (parallax factor < 1.0)
- Each cloud drifts horizontally at its own slow speed
- Clouds wrap horizontally: when a cloud drifts off one screen edge it re-enters from the other
- Cloud shapes are blocky (clusters of rectangles) matching the procedural rectangle aesthetic of the rest of the game
- Cloud formations are deterministic per-world (same seed -> same clouds)
- No disk save required: clouds are regenerated from the world seed on world entry
- No save format change (world save stays at v3)
- No new texture assets, no texture atlas changes
- No interaction or collision with clouds (purely visual)
- Compiles clean with `-Wall -Wextra`

## Architecture

### New module: `src/engine/clouds.h` / `src/engine/clouds.c`

A self-contained engine module owning cloud state, generation, drift update, and parallax rendering. State is held in the `Game` struct and (re)initialized on world entry.

#### Public interface (`clouds.h`)

```c
#ifndef CLOUDS_H
#define CLOUDS_H

#include <stdint.h>
#include "renderer.h"
#include "camera.h"

#define CLOUD_LAYER_COUNT 2
#define CLOUDS_PER_LAYER  12
#define CLOUD_MAX_PARTS   9

typedef struct {
    int ox[CLOUD_MAX_PARTS];
    int oy[CLOUD_MAX_PARTS];
    int w[CLOUD_MAX_PARTS];
    int h[CLOUD_MAX_PARTS];
    int part_count;
    float world_x;
    float world_y;
    float drift_speed;
    int size_scale;
} Cloud;

typedef struct Clouds {
    Cloud layer[CLOUD_LAYER_COUNT][CLOUDS_PER_LAYER];
    int   count[CLOUD_LAYER_COUNT];
    int   world_w;
    int   world_h;
    int   wrap_w;
} Clouds;

void clouds_init(Clouds *c, uint32_t seed, int world_w, int world_h);
void clouds_update(Clouds *c, float dt);
void clouds_render(Clouds *c, Renderer *r, const Camera *cam);

#endif
```

#### Per-layer configuration

| Layer | Index | Parallax | Drift speed | Size scale | Opacity | Color (RGB)     |
|-------|-------|----------|-------------|------------|---------|-----------------|
| Near  | 0     | 0.50     | 10-16 px/s  | 1.0x       | 0.90    | white (1,1,1)   |
| Far   | 1     | 0.25     | 4-8 px/s    | 0.6x       | 0.60    | light-gray (0.85,0.85,0.9) |

Parallax and drift sign are positive (clouds appear to move left as the camera pans right; clouds drift slowly to the right over time).

#### `clouds_init(Clouds *c, uint32_t seed, int world_w, int world_h)`

- Stores `c->world_w = world_w; c->world_h = world_h; c->wrap_w = world_w;` (cloud-space wrap width = world pixel width).
- Seeds a local `Prng` with `seed` (derived from `world.rng.state`).
- For each layer index `L` in `[0, CLOUD_LAYER_COUNT)`:
  - Sets `c->count[L] = CLOUDS_PER_LAYER`.
  - For each cloud slot `i`:
    - `world_x` = `prng_range(&rng, 0, world_w)` (spread across world width in cloud-space pixels; world_w is world pixel width = `WORLD_WIDTH * TILE_SIZE`)
    - `world_y` = `prng_range(&rng, 0, world_h * 0.4)` (keep clouds in the upper 40% of the sky band so they sit above terrain)
    - `drift_speed` = `prng_float`-based value in the layer's range (near: 10-16; far: 4-8)
    - `size_scale` = layer scale (near 1.0, far 0.6)
    - `part_count` = `prng_range(&rng, 5, CLOUD_MAX_PARTS)` (5-9 rectangles)
    - For each part: generate `ox`, `oy` offsets clustered near origin (e.g. `ox` in `[-32, 32]`, `oy` in `[-12, 12]`) and `w`,`h` in `[12, 28]`, all scaled by `size_scale`.
- Pure setup; no allocations (fixed-size arrays), so no teardown function needed.

#### `clouds_update(Clouds *c, float dt)`

- For each cloud in each layer: `cloud.world_x += cloud.drift_speed * dt`.
- Horizontal wrap handled in render (modulo over the cloud-space width), so update only advances position. Wrap width is recomputed from `world_w` captured at init; store it in the `Clouds` struct as `int wrap_w`.

Add `int wrap_w;` field to `struct Clouds`.

#### `clouds_render(Clouds *c, Renderer *r, const Camera *cam)`

Drawn via `renderer_draw_rect` (alpha-blended quads). For each layer `L` with parallax factor `P[L]` and color/alpha `col[L]`:

For each cloud:
1. Apply drift wrap: `eff_x = fmodf(cloud.world_x, c->wrap_w); if (eff_x < 0) eff_x += c->wrap_w;` then subtract the camera's horizontal parallax offset: `screen_x = eff_x - (cam->x - c->world_w / 2.0f) * P[L]`. (Captures drift + parallax in one x.)
2. Vertical: `screen_y = cloud.world_y - (cam->y - c->world_h / 2.0f) * P[L]`.
3. Cull clouds whose `screen_x` is fully outside `[-CLOUD_MAX_WIDTH, g_screen_w + CLOUD_MAX_WIDTH]` (skip off-screen).
4. For each part: `renderer_draw_rect(r, screen_x + ox, screen_y + oy, w, h, col.r, col.g, col.b, col.a)`.

Store `world_w`, `world_h` in the `Clouds` struct during init for use here. Add fields `int world_w; int world_h;` to `struct Clouds`.

Note: clouds render in screen-space (after `renderer_clear`, before `renderer_begin_tile_batch`), so coordinates are raw screen pixels, NOT ortho/tile-space. This deliberately decouples cloud motion from the camera zoom used by the tile batch.

### Camera (`camera.h` / `camera.c`) - NO CHANGES

Clouds read `cam->x`, `cam->y`, and the world dimensions only. No camera API changes.

### Renderer (`renderer.h` / `renderer.c`) - NO CHANGES

Clouds reuse the existing `renderer_draw_rect` (alpha-blended quad) primitive. No new renderer functions, no atlas changes, no new shaders.

### Main (`main.c`)

Changes:
- Include `engine/clouds.h`.
- Add `Clouds clouds;` field to the `Game` struct (after `Camera camera;` at `main.c:43`, near line 43-44).
- In `game_enter_world` (`main.c:118`), after `player_init` / `explosive_reset` (around `main.c:141-142`): call `clouds_init(&g->clouds, g->world.rng.state, g->world.width * TILE_SIZE, g->world.height * TILE_SIZE);`.
- In `game_update`: call `clouds_update(&g->clouds, dt);` alongside the other per-frame updates (only when `GAME_STATE_PLAYING`).
- In `game_render` (`main.c:402`), after `renderer_clear(&g->renderer, 0.4f, 0.7f, 1.0f)` at `main.c:410` and BEFORE `renderer_begin_tile_batch` at `main.c:412`: call `clouds_render(&g->clouds, &g->renderer, &g->camera);`.

### Makefile

Add `src/engine/clouds.c` to the source list (alongside `camera.c`, `renderer.c`, etc.).

## Data Flow

1. `game_enter_world` loads/generates the world (which sets `world.rng`), then calls `clouds_init` with `world.rng.state` as seed and the world pixel dimensions.
2. `clouds_init` seeds a local PRNG and fills the two layer arrays with deterministic cloud shapes/positions.
3. Each frame, `game_update` calls `clouds_update(dt)`, advancing each cloud's `world_x` by `drift_speed * dt`.
4. `game_render` clears the sky blue, then calls `clouds_render`:
   - Per cloud: combine drift offset + camera parallax into a screen-space x/y.
   - Wrap horizontally over `wrap_w`.
   - Cull off-screen clouds.
   - Draw each part as an alpha-blanded rectangle via `renderer_draw_rect`.
5. Then `renderer_begin_tile_batch` + world/player/explosive drawing happens on top, so clouds always appear behind the world.

## Determinism

- Seed source: `world.rng.state` (already set deterministically during `world_generate`; existing worlds loaded from disk also carry their `rng` state from save).
- All shape/position values come from the seeded local `Prng`, so the same world always yields the same cloud layout.
- Drift is the only non-deterministic-in-principle element (time-based), but it is deterministic given a session start time; this is acceptable and matches how other animated elements (portal pulse, lava animation) behave.

## Edge Cases

- Clouds only update/render during `GAME_STATE_PLAYING`. The menu screen (`main.c:403-407`) is untouched.
- Drift wrap uses `fmodf` over `wrap_w` so clouds never permanently exit the sky; they recycle from the left edge.
- Off-screen culling prevents wasted draw calls for clouds far outside the viewport.
- Clouds are drawn before the tile batch, so camera zoom (which only affects the tile batch's ortho projection) does not scale clouds - they remain at constant screen size regardless of zoom. This is intentional: parallax clouds are a sky backdrop, not world objects.
- `clouds_init` uses fixed-size arrays, so there is no allocation to fail and no `clouds_free` needed. Re-entering a world simply overwrites the struct.
- No save format change: clouds are regenerated from seed on every world entry, so v3 saves remain compatible.
- If `world.rng.state` happens to be 0 on a freshly init'd-but-not-generated world, `prng_seed` with 0 must still produce usable output (PRNG should handle 0 gracefully; verify during implementation and reseed with a fallback like a name hash if needed).

## Out of Scope

- Day/night cycle or cloud tinting by time of day (no day/night system exists).
- Clouds on the menu/world-select screen.
- Cloud interaction, collision, or gameplay effect.
- Textured cloud sprites (using blocky rectangles instead).
- Rain, weather, or shadow casting from clouds.
- More than two layers.
- Clouds reacting to player actions.

## Files

| File | Change |
|------|--------|
| `src/engine/clouds.h` | **NEW** - public interface, `Cloud`/`Clouds` structs |
| `src/engine/clouds.c` | **NEW** - generation, update, parallax render |
| `src/main.c` | Add `Clouds` to `Game` struct; init/update/render calls |
| `Makefile` | Add `src/engine/clouds.c` to sources |
