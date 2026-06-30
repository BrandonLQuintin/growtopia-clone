# Multiple Characters (Character Saves)

**Date:** 2026-06-30
**Status:** Approved
**Scope:** Add support for multiple, independently-progressed character saves. Each character owns its own inventory, gems, health, and equipped clothing. Players pick or create a character at startup via a new character-select screen before entering world search. Worlds remain global/shared.

## Problem

The game has a single global player profile (`res/worlds/player.dat`) holding inventory, gems, health, and equipped clothing. This profile is shared across every world and there is exactly one of it (`main.c:31`, `main.c:110`, `main.c:149`). There is no way to maintain multiple characters with separate progression — a second player on the same machine, or a single player wanting a fresh "save slot," cannot coexist with an established character without overwriting it.

## Goals

- Multiple characters, each with its own inventory, gems, health, and equipped clothing.
- Unlimited number of characters, presented in a scrollable list at startup.
- Create a new character by typing a name (alphanumeric lowercase, mirroring the world-name input UX).
- Select a character to play it; its profile loads into the active game.
- Delete a character (with a confirm prompt).
- Rename a character.
- The character list shows identifying stats per entry (gem count + last played world).
- Existing single-character progress (`res/worlds/player.dat`) is not silently discarded.

## Non-Goals

- No per-character worlds: worlds (`.wld` files) and `recent.txt` stay global/shared. Any character can enter any world.
- No per-character world state (broken blocks etc. are part of the shared world file).
- No password/login, avatars, or visual character customization.
- No per-character keybinds or settings.
- No network/multi-character sync.
- No changes to the world save format (`.wld`).
- No test harness (verification is `make clean && make` zero-warning + manual playtest).

## Approach

**Approach A (chosen): New `character` data module + new `GAME_STATE_CHAR_SELECT` state + new `char_select` UI module.**

- New `src/game/character.c/.h` owns the `Character` data type, the binary file format, and directory operations (list / load / save / create / delete / rename).
- New `src/engine/char_select.c/.h` renders the scrollable selection list and handles create/rename/delete interactions.
- A new game state is inserted *before* the existing world-select state: `CHAR_SELECT → MENU (world select) → PLAYING`.
- Per-character files live in a new `res/chars/<name>.dat`; `res/worlds/` is unchanged.

Rejected alternatives:
- *Approach B (extend `inventory.c`, no new state):* muddies `inventory.c`'s single responsibility and complicates the menu state machine.
- *Approach C (name-based text entry, no list):* cannot satisfy the chosen requirements (scrollable list with delete/rename/stats).

## Design

### Data Model

**New module `src/game/character.h`:**

```c
#define CHAR_NAME_MAX 20
#define CHAR_MAGIC "CHAR"
#define CHAR_VERSION 1

typedef struct {
    char      name[CHAR_NAME_MAX + 1];
    char      last_world[WORLD_NAME_MAX + 1];
    int       gems;
    int       health;
    uint16_t  equipped[3];
    Inventory inventory;
} Character;
```

`CHAR_NAME_MAX` matches `WORLD_NAME_MAX` (20). Name validation reuses world-select rules: alphanumeric, lowercased, max 20 chars. `last_world` is empty for a brand-new character and is stamped with the current world's name every time that character is saved.

### File Format

Per-character file: `res/chars/<name>.dat`. One file per character; the directory is scanned to build the list. Binary format mirrors the world file's magic+version approach and the existing profile field order:

```
"CHAR" magic (4 bytes)
version      int32       (= 1)
name         char[64]    (null-padded)
last_world   char[64]    (null-padded)
gems         int32
health       int32
equipped     uint16[3]
inventory    36 x (uint16 item_id + int32 count)
```

The name is stored inside the file *and* derived from the filename (matching how `world.c` treats world names), allowing a consistency check on load. The inventory layout is identical to what `inventory_save_profile`/`inventory_load_profile` write/read today, so the slot-serialization logic can be reused.

### Storage Layout

- `res/chars/` — **new** directory, created on first run. One `.dat` per character.
- `res/worlds/` — **unchanged**. Worlds remain global; `recent.txt` remains global (consistent with "profile only" scope).
- `res/worlds/player.dat` — **migrated** to `res/chars/default.dat` on first character-select init *only if* `res/chars/` is empty and `res/worlds/player.dat` exists. Because `player.dat` stores only inventory/gems/health/equipped (no name or last-world), migration constructs a `Character` with `name = "default"` and `last_world = ""` plus the loaded fields, then writes it via `character_save`. After migration the old `player.dat` is left in place but no longer read or written by the game.

### Character Module API (`src/game/character.h`)

```c
void character_path(const char *name, char *out, size_t out_size);
int  character_load(Character *c, const char *path);      /* returns 0 on success */
int  character_save(const Character *c, const char *path);
int  character_list(char names[][CHAR_NAME_MAX + 1],
                    int *count, int max);                 /* scans res/chars/, alphabetical */
int  character_exists(const char *name);
int  character_delete(const char *name);
int  character_rename(const char *old_name, const char *new_name);
int  character_name_valid(const char *name);              /* alnum lowercase, len 1..20 */
```

`character_list` uses POSIX `opendir`/`readdir` on `res/chars/`, filters the `.dat` suffix, strips it to recover the name, then sorts alphabetically with `qsort` for stable display. `character_rename` renames the file and rewrites the stored `name` field. `character_delete` uses `remove()`.

### Game-State Flow (`src/main.c`)

New state added before world select:

```c
typedef enum {
    GAME_STATE_CHAR_SELECT,   /* new */
    GAME_STATE_MENU,          /* world select — unchanged */
    GAME_STATE_PLAYING
} GameState;
```

Startup → `GAME_STATE_CHAR_SELECT`. Transitions and ESC behavior:

| State          | Action                | Result                                  |
|----------------|-----------------------|-----------------------------------------|
| CHAR_SELECT    | click character       | load it → `GAME_STATE_MENU`             |
| CHAR_SELECT    | ESC                   | quit game                               |
| MENU           | Enter world           | → `GAME_STATE_PLAYING`                  |
| MENU           | ESC                   | → `GAME_STATE_CHAR_SELECT` (was: quit)  |
| PLAYING        | ESC → confirm Y       | save → `GAME_STATE_MENU` (unchanged)    |

**`Game` struct additions:**
```c
char current_char_name[CHAR_NAME_MAX + 1];
char last_world[WORLD_NAME_MAX + 1];
```

**Profile load/save re-pointed to the active character:**
- `game_save_all` (`main.c:103`): after saving the world, write `res/chars/<current_char_name>.dat` via `character_save`, stamping `last_world` = `g->current_world_name`. This replaces the hardcoded `PROFILE_PATH` / `inventory_save_profile` call at `main.c:110`.
- `game_enter_world` (`main.c:120`): load `res/chars/<current_char_name>.dat` instead of `player.dat` (`main.c:149`). On success populate `g->inventory`, `g->player.gems/health/equipped`, and `g->last_world`. On load failure (defensive — should not happen for an existing character), fall back to default starting inventory.
- **New-character defaults:** a freshly created `Character` gets the same starting inventory as today (`main.c:158-165`): 50 dirt, 30 stone, 20 wood, 10 dirt seed, 5 grass seed, 5 wood seed, `gems = 999999`, `health = MAX_HEALTH`, empty `last_world`. This preserves current new-player behavior.

**Entry points in `main.c`:**
- `game_init`: set `g_game_state = GAME_STATE_CHAR_SELECT`; init the new `CharSelect` UI (mirrors `world_select_init`).
- The event/update/render dispatch each gain a `GAME_STATE_CHAR_SELECT` case alongside the existing `GAME_STATE_MENU` case.
- When a character is selected, copy its name into `g->current_char_name` before transitioning to `GAME_STATE_MENU`.
- `ensure_worlds_dir` (`main.c:115`) is extended to also `mkdir("res/chars", 0755)`.

The existing per-world-entry profile load is retained in spirit: the active character's file always holds the latest inventory/gems, and saving happens on exit/autosave as today, now writing to the character file.

### `char_select` UI Module (`src/engine/char_select.h`)

A new screen modeled on `world_select.c`, reusing the same renderer primitives (rects, bitmap text, cursor blink). Example layout on a dark background:

```
            SELECT CHARACTER
        +------------------------+
        | gemrock      1,250 g   | last: start       [R] [X]
        | builder     48,213 g   | last: megaworld   [R] [X]
        | newchar          0 g   | (no world yet)    [R] [X]
        +------------------------+
        [ New Character ]
        ... scroll if more rows than fit ...
        ESC: Back
```

**`CharSelect` struct:**

`CHAR_LIST_MAX` is a practical in-memory snapshot cap (64), consistent with the codebase's fixed-array style (e.g. `RECENT_WORLDS_MAX`). It is not a slot limit on the number of characters that can exist on disk; `character_list` caps the *displayed* list at this many entries, which is far beyond realistic single-player use and trivially raised if ever needed.

```c
#define CHAR_LIST_MAX 64

typedef struct {
    char names[CHAR_LIST_MAX][CHAR_NAME_MAX + 1];
    int  gems_snap[CHAR_LIST_MAX];
    char last_world_snap[CHAR_LIST_MAX][WORLD_NAME_MAX + 1];
    int  count;
    int  scroll;
    int  hovered;
    int  selected;          /* index clicked to play, or -1 */

    int  creating;          /* 1 = new-character text-input mode */
    int  renaming;          /* index being renamed inline, or -1 */
    int  deleting;          /* index pending delete-confirm, or -1 */
    char input_text[CHAR_NAME_MAX + 1];
    int  input_cursor;
    float cursor_timer;
    int  mouse_clicked;
} CharSelect;
```

**API** (mirrors `world_select` signatures):
```c
void char_select_init(CharSelect *cs);
void char_select_refresh(CharSelect *cs);   /* re-scan res/chars/ */
void char_select_handle_event(CharSelect *cs, SDL_Event *e);
void char_select_update(CharSelect *cs, float dt);
void char_select_render(CharSelect *cs, Renderer *r);
```

**Interactions:**
- **Select to play:** click a row body → set `selected` → `main.c` loads that character and goes to `GAME_STATE_MENU`.
- **New Character** button → `creating = 1`, activates a text field reusing the world-select alnum-lowercase input + blinking cursor + Backspace/Enter. On Enter: validate non-empty and name-not-taken (`character_exists`), create the `.dat` with default inventory via `character_save`, refresh the list, and immediately select the new character.
- **Rename:** a small `[R]` button per row → `renaming = <idx>` enables inline text edit of that row's name. Enter validates uniqueness (excluding the character being renamed itself, so an unchanged name is accepted) then calls `character_rename` and refreshes.
- **Delete:** a small `[X]` button per row → `deleting = <idx>` shows a confirm prompt "Delete <name>? Y/N" (mirrors the existing `exit_confirm_active` pattern). Y → `character_delete` + refresh; N/ESC → cancel. Deleting all characters is permitted (list becomes empty; only "New Character" remains).
- **Scroll:** mouse-wheel up/down and/or Up/Down arrow keys adjust `scroll`, clamped so the list cannot scroll past the end. Only rows that fit the visible area are rendered and click-tested.

**Stat snapshot:** at refresh time, load each `.dat` just enough to read `gems` + `last_world` (file payload is ~270 bytes/character, so a full per-file read is trivial and avoids a separate metadata format). Full `Character` inventory data is **not** retained in the UI struct; `main.c` loads the complete `Character` only when one is selected.

**Readability:** list names use font scale >= 1.2; stats and hints >= 1.0 — consistent with `world_select`'s existing scales. Screen sizing uses `g_screen_w`/`g_screen_h` (no hardcoded screen dimensions).

### Conventions

- C11, `-Wall -Wextra`, no comments in code, `#ifndef` include guards, `snake_case`, module-prefixed functions (`character_*`, `char_select_*`).
- `main.c` stays the orchestrator; data logic in `src/game/character.c`, UI in `src/engine/char_select.c`.
- UI click-detection rects exactly match render rects (the common bug source called out in AGENTS.md).

## Verification

- `make clean && make` compiles with zero warnings.
- Manual playtest:
  1. Fresh start with no `res/chars/` but an existing `res/worlds/player.dat` → character list shows one entry `default` carrying the old inventory/gems (migration works).
  2. Create a second character `builder` → appears in list with 0-gem defaults; select it, enter a world, pick up items/gems, exit → re-selecting `builder` shows updated gems; switching back to `default` shows its untouched progress.
  3. Rename `builder` → `architect`; the file is renamed and the list updates.
  4. Delete `architect` with confirm → removed; cancel (N) keeps it.
  5. World select ESC returns to character select; character select ESC quits.
  6. Worlds remain shared: both characters can enter the same world name and see the same world state.
