# Seed Splice In-World Design

## Summary

Replace the inventory-based seed splicing system with in-world splicing. Players hold a seed in their hotbar and right-click on an actively growing plant to splice. The recipe table is unchanged.

## Current Behavior

- Player opens inventory, clicks two seeds, and `crafting_splice()` checks the recipe table
- If matched, both seeds are removed and the result seed is added to inventory
- The hint "Click 2 seeds to splice them!" is shown below the inventory panel

## New Behavior

- Player holds a seed in their hotbar and right-clicks a tile with an actively growing plant (growth_stage 1-4)
- The held seed is checked against the growing plant's seed (stored in tile `extra_data`) via `crafting_splice()`
- If matched: both seeds are consumed, the tile is replanted with the result seed at growth stage 1
- If no match: nothing happens (the plant stays as-is)

## Files Changed

### `src/main.c`
- Remove inventory-based splice logic (lines 173-194): the block that checks `drag_from_slot`/`selected_slot` for seed+seed splice. Replace with a simple `inventory_swap_slots()` call.
- Add in-world splice logic in the right-click section (~line 377-396): when the clicked tile has `growth_stage >= GROWTH_STAGE_1 && growth_stage < GROWTH_COMPLETE` and the held item is a seed, check `crafting_splice(held_seed, tile_seed, &result)`. On success, call `farming_plant_seed()` with the result and `inventory_remove()` the held seed.

### `src/engine/ui.c`
- Remove the "Click 2 seeds to splice them!" hint text (line 299-302)

### `src/game/crafting.h`
- Remove `crafting_get_recipes()` declaration (no longer used anywhere)

### `src/game/crafting.c`
- Remove `crafting_get_recipes()` implementation
- Keep recipe table and `crafting_splice()` unchanged

### `docs/superpowers/.../AGENTS.md`
- Update "UI System" section: remove "Seed splicing: when two seeds are clicked in inventory..."
- Add note that splicing is done by placing a seed on a growing plant in the world

## Edge Cases

- Fully grown plants (growth_stage == GROWTH_COMPLETE) cannot be spliced — they must be harvested first
- Air tiles with no plant are handled by the existing seed planting path (no splice, just plant)
- If the recipe has no match, the right-click does nothing — the plant is not disturbed
- The tile's `extra_data` stores the original seed ID, which is used as `seed_a` in the splice check
