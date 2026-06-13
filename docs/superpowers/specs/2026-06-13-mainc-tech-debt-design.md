# main.c Tech Debt Refactor

**Date:** 2026-06-13
**Status:** Approved
**Scope:** Slim main.c to a thin orchestrator by extracting game logic into the modules that own each domain.

## Problem

`main.c` is 780 lines. Two functions hold the bulk of it:

- `game_update` (~350 lines): collision resolution, a punch/break/place/wrench/splice loop with a `goto`, store purchase logic, growth ticks, and movement.
- `game_render` (~155 lines): world tile rendering with growth stages, player/break rendering, exit-confirm dialog.

AGENTS.md states "Keep main.c as the orchestrator; game logic in `src/game/`, engine in `src/engine/`, data in `src/world/`." That contract is violated: game logic and rendering logic live inline in main.c instead of in their owning modules.

Additionally, two cross-cutting warts exist:

- **RNG inconsistency:** `prng.h`/`prng.c` define a clean LCG but are unused. `world_generate` uses `srand(time)/rand()` (9 calls), and the gem-drop path uses `rand() % 3` (1 call). Additionally, `prng_float` has a pre-existing bug (returns `[0, ~2.0)` instead of `[0, 1.0)`) that must be fixed before relying on it.
- **Duplicated constants:** `ITEM_WRENCH`/`ITEM_PICKAXE` are `#define`d locally in both `main.c:28-29` and `store.c:6-7`, with no shared definition.
- **Dead code:** `inventory_save` and `inventory_load` (non-profile versions) have zero call sites. Dead locals `facing_x`/`facing_y` (main.c:413-414) are computed but never read.

## Goals

- main.c shrinks from ~780 to ~300 lines, becoming pure orchestration (init, event dispatch, thin update/render delegation, shutdown).
- Every chunk of game logic moves to the module that owns its domain. Zero new files.
- Behavior is preserved, with noted exceptions: (1) camera updates on wrench frame (invisible improvement); (2) no per-click Store alloc; (3) RNG unification preserves probabilities and ranges exactly but produces different output sequences because the LCG algorithm differs from libc `rand()` — generated worlds will differ from before but with statistically identical distributions.
- Opportunistic warts in moved code are fixed inline.
- RNG is unified onto the existing `prng` module.
- Dead inventory save/load code is deleted.

## Non-Goals

- No new features.
- No save format changes (existing `.wld` files must load unchanged).
- No test harness (verification is build + manual playtest).
- No rendering/visual changes.
- Storing world generation seed for reproducibility (infrastructure is put in place, but the feature is not exposed).

## Approach

**Approach A: Expand existing modules.** Each extraction target lands in the module that already owns its domain. No new files are created.

### Extraction Map

| main.c code (current lines) | Destination | New function |
|---|---|---|
| Collision resolution (331-394) | `src/game/player.c` | `player_collide(Player*, World*)` |
| Growth tick loop (399-410) | `src/game/farming.c` | `farming_tick_nearby(World*, px, py, dt)` |
| Punch on click (416-444) | `src/game/interact.c` | `interact_handle_click(...)` |
| Break on hold (446-501) | `src/game/interact.c` | `interact_handle_break(...)` |
| Place/wrench/splice on RMB (504-552) | `src/game/interact.c` | `interact_handle_place(...)` |
| Store purchase click (256-286) | `src/game/store.c` | `store_handle_click(...)` |
| Sign-edit text input events (218-232) | `src/engine/ui.c` | `ui_handle_sign_edit_event(...)` |
| World tile + growth rendering (613-652) | `src/engine/renderer.c` | `renderer_draw_world(...)` |
| Player + break-progress rendering (654-673) | `src/engine/renderer.c` | `renderer_draw_player(...)` |
| Exit-confirm dialog rendering (722-736) | `src/engine/ui.c` | `ui_render_exit_confirm(...)` |

### What stays in main.c

Legitimate orchestration that belongs in the orchestrator:

- `Game` struct definition, `main()` loop, `game_init`, `game_shutdown`.
- `game_handle_events` — event dispatch. Sign-edit text input delegates to `ui_handle_sign_edit_event`.
- `game_update` — thin delegation: movement input setup, `player_update`, `player_collide`, `farming_tick_nearby` + `farming_update`, `interact_handle_*`, hotbar/zoom/F5 keys, camera update, sign-overlay timer, autosave, `input_update`. (~80 lines)
- `game_render` — thin delegation: `renderer_clear`, `renderer_draw_world`, `renderer_draw_player`, `ui_render_*`, `renderer_present`. (~50 lines)
- `game_save_all`, `game_enter_world`, `ensure_worlds_dir` — orchestration-level setup/teardown.

## New Function Signatures

### player.h / player.c

```c
void player_collide(Player *p, World *w);
```

Resolves the player AABB against the tile grid using minimum-overlap resolution. Mutates `p->x`, `p->y`, `p->vx`, `p->vy`, `p->on_ground`. Called after `player_update` (which applies gravity + velocity). `player.c` adds `#include "../world/world.h"` (for `World*`, `world_is_solid`) and `#include "../engine/renderer.h"` (for `TILE_SIZE`, which is defined in `renderer.h:6`, not `world.h`).

### farming.h / farming.c

```c
void farming_tick_nearby(World *w, int px, int py, float dt);
```

Increments `growth_timer` on tiles within the 5x4 region around the player tile `(px, py)` for tiles with `growth_stage` in `[1, GROWTH_COMPLETE)`. This is the loop currently at main.c:399-410. `farming_update` (main.c:411, stage advancement) is called separately by the orchestrator immediately after, unchanged.

### interact.h / interact.c

Three new high-level handlers that encapsulate mouse-to-tile conversion, reach checking, and dispatch to the existing tile-level primitives (`interact_punch`, `interact_wrench`, etc.):

```c
#define REACH_RADIUS 6

int  interact_handle_click(World *w, Player *p, const Inventory *inv, int hotbar_sel,
                           const Input *in, const Camera *cam,
                           int *sign_x, int *sign_y);
void interact_handle_break(World *w, Player *p, Inventory *inv, int hotbar_sel,
                           const Input *in, const Camera *cam, float dt);
void interact_handle_place(World *w, Player *p, Inventory *inv, int hotbar_sel,
                           const Input *in, const Camera *cam, UI *ui,
                           int *portal_pending, int *portal_x, int *portal_y);
```

**`interact_handle_click`** (left mouse click):
- Converts mouse screen coords to world tile via `camera_screen_to_world`.
- Checks reach: `dx*dx + dy*dy <= REACH_RADIUS * REACH_RADIUS`.
- If target is an interactive block (`block_is_interactive`) and held tool is not pickaxe: dispatches `interact_punch`.
- Returns: `0` = no action, `1` = handled (orchestrator skips break), `2` = sign was read (sets `*sign_x`/`*sign_y`, orchestrator skips break and activates sign overlay).

**`interact_handle_break`** (left mouse held):
- Same mouse-to-tile + reach conversion.
- Preserves the existing guard: interactive blocks are only breakable with a pickaxe (the empty-body `if (block_is_interactive(t->fg) && tool_id != ITEM_PICKAXE) {}` at main.c:457 — silently skips breaking).
- Manages the break state machine: `breaking`/`break_x`/`break_y`/`break_timer`.
- On completion: adds drops via `inventory_add`, harvests grown plants via `farming_harvest`, adds gem drops via `prng_range(&w->rng, 1, 3)`, calls `interact_cleanup_break`, clears the tile.
- Resets break state when mouse not held.

**`interact_handle_place`** (right mouse click):
- Same mouse-to-tile + reach conversion.
- Wrench path: dispatches `interact_wrench`, manages portal link state via out-params, opens sign edit via `ui_init_sign_edit`.
- Place path: if held item is a seed, plants via `farming_plant_seed`; if a block, sets `t->fg`. Removes item from inventory.
- Splice path: if held seed and target is growing plant, calls `crafting_splice` + `farming_plant_seed`.

**Includes added to interact.c:** `#include "../engine/input.h"`, `#include "../engine/camera.h"`, `#include "inventory.h"`, `#include "../engine/ui.h"`, `#include "../world/items.h"` (for `item_get_def`/`ItemDef`/`ITEM_CAT_BLOCK`), `#include "crafting.h"` (for `crafting_splice`), `#include "farming.h"` (for `farming_harvest`/`farming_plant_seed`/`farming_can_plant`). No cycle: none of these include `interact.h`.

### store.h / store.c

Centralized layout constants (resolves the click/render divergence):

```c
#define STORE_COLS    5
#define STORE_CELL_W  100
#define STORE_CELL_H  80

int store_handle_click(Store *s, const Input *in, int *gems,
                       Inventory *inv, int screen_w, int category);
```

Uses the persistent `Store*` from the `Game` struct (no per-click `store_init` alloc). Computes item grid positions from the shared constants. On click within an item cell: checks gems, deducts, calls `inventory_add`. Returns 1 if a purchase was made, 0 otherwise.

`ui_render_store_screen` in `ui.c` switches from its local `#define STORE_COLS 5` and hardcoded `cell_w`/`cell_h` to the shared `store.h` constants, so click detection and rendering positions can never diverge.

**Includes added to store.h:** `#include "../engine/input.h"` (for `Input`), `#include "../game/inventory.h"` (for `Inventory`). Both are leaf modules — no cycle.

### renderer.h / renderer.c

```c
void renderer_draw_world(Renderer *r, const Camera *cam, const World *w);
void renderer_draw_player(Renderer *r, const Camera *cam, const Player *p, const World *w);
```

**`renderer_draw_world`:** Computes visible tile bounds from camera, iterates tiles, draws background and foreground with growth-stage rendering (partial-height plants, leaf canopies for mature plants) and portal pulse overlay. Called between `renderer_begin_tile_batch` and `renderer_end_tile_batch`.

**`renderer_draw_player`:** Draws the player body rectangle, eyes, and break-progress overlay. Looks up the break tile's `break_time` via the `World*` to compute progress fraction.

**Includes added to renderer.c:** `#include "camera.h"`, `#include "../world/world.h"`, `#include "../game/player.h"`, `#include "../world/block.h"` (for `block_get_sprite`/`block_get_break_time`/`block_get_name`).

### ui.h / ui.c

```c
void ui_handle_sign_edit_event(UI *ui, const SDL_Event *e);
void ui_render_exit_confirm(UI *ui, Renderer *r);
```

**`ui_handle_sign_edit_event`:** Handles `SDL_TEXTINPUT` (append character if under `SIGN_TEXT_MAX_LEN`) and `SDL_KEYDOWN` backspace (remove last character) while in `UI_STATE_SIGN_EDIT`. Called from `game_handle_events`.

**`ui_render_exit_confirm`:** Draws the centered "EXIT WORLD? Y/N" dialog box. Called from `game_render`.

## Orchestrator Data Flow

`game_update` becomes:

```
dispatch menu / exit-confirm / UI-overlay states (existing, unchanged)
  └─ store click → store_handle_click()
  └─ inventory swap → inventory_swap_slots() (existing)
  └─ sign edit → ui_update_sign_edit() (existing)

read movement keys → set player.vx / walking / jump
player_update(player, dt, world bounds)
player_collide(player, world)                          ← extracted
farming_tick_nearby(world, px, py, dt)                 ← extracted
farming_update(world, dt)                              ← existing

click_result = interact_handle_click(...)              ← replaces goto block
  if click_result == 2 → activate sign overlay (set active, timer, x, y)
  if !click_result → interact_handle_break(...)
interact_handle_place(...)

hotbar selection (1-9 keys)
zoom (scroll wheel)
F5 manual save
camera_set_target + camera_update
sign overlay timer countdown
autosave timer
input_update
```

`game_render` becomes:

```
renderer_clear()
renderer_begin_tile_batch()
  renderer_draw_world()                                ← extracted
  renderer_draw_player()                               ← extracted
renderer_end_tile_batch()
renderer_begin_ui()
  ui_render_hud / ui_render_hotbar / tooltip           ← existing (stays)
  block-name tooltip at mouse                          ← stays in orchestrator (glue)
  sign overlay text                                    ← stays in orchestrator (glue)
  ui_render_inventory_screen / store_screen / sign_edit← existing
  ui_render_exit_confirm()                             ← extracted
  help text bar                                        ← stays
renderer_end_ui()
renderer_present()
```

## Opportunistic Fixes

Warts fixed during the move, scoped to moved code:

1. **`goto skip_break` eliminated.** Replaced by `interact_handle_click` returning a result code (0/1/2). Break runs only when result is 0. The `skip_break:` label and `goto` are deleted.

2. **`REACH_RADIUS 6` constant** in `interact.h`. Replaces 3 instances of bare `<= 36` (punch, break, place reach checks).

3. **Throwaway `Store` alloc removed.** `store_handle_click` uses the persistent `g->store` instead of `store_init` on every click.

4. **Store layout centralized.** `STORE_COLS`, `STORE_CELL_W`, `STORE_CELL_H` in `store.h`, shared by `store_handle_click` and `ui_render_store_screen`. Fixes the latent divergence: main.c used hardcoded `tabs_y = 60 + 36; items_y = tabs_y + 28 + 16`, ui.c used computed `tabs_y + tab_h + 16`. Both now derive from the same constants.

5. **Wrench early-return dropped.** The original `input_update(&g->input); return;` after wrenching a sign only skipped one frame of camera/autosave/sign-timer. The orchestrator now always runs camera/autosave/input_update at the end. Effect: camera updates on the wrench frame (invisible improvement). Behavior is otherwise identical because `input_update` runs exactly once per frame either way.

6. **`ITEM_WRENCH`/`ITEM_PICKAXE` centralized.** Currently `#define`d locally in both `main.c:28-29` and `store.c:6-7`. Move to `items.h` (which already defines `ITEM_FIST` and `ITEM_GEMS`). Remove the local `#define`s from `main.c` and `store.c`.

## RNG Unification

The world owns its RNG via a `Prng` field:

- **Prerequisite — fix `prng_float` bug.** `prng.c:11` divides a full-range `uint32_t` state by `0x80000000u` (2^31), yielding `[0, ~2.0)` instead of `[0, 1.0)`. This would double all probabilities and overflow ranges. Fix: `return (float)(p->state >> 1) / (float)0x80000000u;` — right-shift gives `[0, 2^31-1]`, divided by 2^31 yields `[0, 1.0)`. This must be done before relying on `prng_float`/`prng_range`.
- `world.h` adds `#include "../engine/prng.h"` and a `Prng rng;` field to the `World` struct. (`prng.h` is a leaf header depending only on `stdint.h` — no cycle.)
- `world_init` seeds `w->rng` from `(uint32_t)time(NULL)`. Preserves current time-based seeding behavior.
- `world_generate` replaces `srand(time(NULL))` and all 9 `rand()` calls with `prng_float(&w->rng)` (for percentage checks) and `prng_range(&w->rng, min, max)` (for ranges).
- `interact_handle_break` (the moved harvest code) replaces `1 + (rand() % 3)` with `prng_range(&w->rng, 1, 3)`, using the `World*` it already receives — no new function params.

Translation reference:

| Original | Replacement |
|---|---|
| `rand() % 100 < 15` | `prng_float(&w->rng) < 0.15f` |
| `rand() % 100 < 10` | `prng_float(&w->rng) < 0.10f` |
| `rand() % 100 < 8` | `prng_float(&w->rng) < 0.08f` |
| `30 + rand() % 25` | `prng_range(&w->rng, 30, 54)` |
| `rand() % w->width` | `prng_range(&w->rng, 0, w->width - 1)` |
| `28 + rand() % 29` | `prng_range(&w->rng, 28, 56)` |
| `2 + rand() % 4` | `prng_range(&w->rng, 2, 5)` |
| `4 + rand() % 3` | `prng_range(&w->rng, 4, 6)` |
| `5 + rand() % 4` | `prng_range(&w->rng, 5, 8)` |
| `1 + (rand() % 3)` | `prng_range(&w->rng, 1, 3)` |

`world.c` removes `#include <time.h>` if it becomes unused after this change (it is still needed for `time(NULL)` in `world_init`, so it stays).

## Dead Code Removal

- `inventory_save` and `inventory_load` (the non-profile versions) have zero call sites. Delete them from `inventory.c` and their declarations from `inventory.h`. The profile versions (`inventory_save_profile` / `inventory_load_profile`) remain — they are not duplicates of each other and are the only live persistence code.
- Dead locals `facing_x`/`facing_y` (main.c:413-414) are computed via `player_get_facing_tile` but never referenced anywhere in `game_update`. Delete the declaration and the call.

## Include Graph

All additions are leaf-module includes. The graph stays acyclic:

- `world.h` adds `../engine/prng.h` (leaf: `stdint.h` only).
- `player.c` adds `../world/world.h` (leaf: `stdint.h`, `prng.h`) and `../engine/renderer.h` (for `TILE_SIZE`).
- `interact.c` adds `../engine/input.h`, `../engine/camera.h`, `inventory.h`, `../engine/ui.h`, `../world/items.h`, `crafting.h`, `farming.h` (all leaves — none include `interact.h`).
- `store.h` adds `../engine/input.h`, `../game/inventory.h` (both leaves).
- `renderer.c` adds `camera.h`, `../world/world.h`, `../game/player.h`, `../world/block.h` (all leaves).

No module gains a dependency on main.c's `Game` struct. Extracted functions receive only the specific pointers they need.

## Verification Strategy

No test harness exists. Verification is build + manual playtest, executed one module at a time so regressions are immediately localizable.

1. **Build:** `make clean && make` must compile with zero warnings (`-Wall -Wextra`) after each extraction step.
2. **Save compatibility:** Save/load code is untouched. Existing `.wld` and `player.dat` files must load without error.
3. **Manual playtest checklist** (after full refactor):
   - Walk, jump, collide with walls/floor (`player_collide`).
   - Break dirt/stone with and without pickaxe; verify drops + break progress bar (`interact_handle_break`).
   - Punch door (toggles open/closed), sign (shows text overlay), portal (teleports) (`interact_handle_click`).
   - Place blocks, plant seeds, splice a growing plant (`interact_handle_place`).
   - Wrench: link two portals, edit a sign.
   - Buy from store in each category (`store_handle_click`).
   - Sign edit: type text, backspace, press Enter to save (`ui_handle_sign_edit_event`).
   - Wait for a seed to advance a growth stage (`farming_tick_nearby`).
   - Visually confirm: tile rendering, growth-stage plant heights, portal pulse, player sprite, break overlay, exit-confirm dialog.
4. **Behavior diff:** Intentional changes: (1) camera updates on the wrench frame (invisible); (2) RNG unification produces different world generation output but with identical probability distributions (15% stays 15%, etc.); (3) gem drops use the same range `[1,3]` via a different RNG. All game mechanics, physics, save format, and UI behavior are otherwise identical.

## Execution Order

Extract one module at a time, build + verify after each:

1. **Prerequisite:** Fix `prng_float` bug in `prng.c` (right-shift state before dividing).
2. `player_collide` → player.c (collision resolution out of game_update).
3. `farming_tick_nearby` → farming.c (growth tick out of game_update). Delete dead `facing_x`/`facing_y`.
4. `interact_handle_click` + `interact_handle_break` + `interact_handle_place` → interact.c (the big interaction loop + goto removal + REACH_RADIUS).
5. `store_handle_click` + layout constants → store.c + ui.c alignment.
6. `ui_handle_sign_edit_event` + `ui_render_exit_confirm` → ui.c.
7. `renderer_draw_world` + `renderer_draw_player` → renderer.c (rendering out of game_render).
8. Move `ITEM_WRENCH`/`ITEM_PICKAXE` to `items.h`; remove local `#define`s from main.c and store.c.
9. RNG unification (world.h `Prng` field + `world_generate` + gem drop).
10. Dead code removal (inventory_save/inventory_load deletion).

Each step is independently compilable and testable. The refactor is complete when main.c contains only orchestration and the playtest checklist passes.
