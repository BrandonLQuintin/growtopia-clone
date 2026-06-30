# Multiple Characters Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add multiple independently-progressed character saves with a scrollable selection screen at startup.

**Architecture:** New `character` data module (`src/game/character.c/.h`) owns the `Character` type and `res/chars/<name>.dat` file format. New `char_select` UI module (`src/engine/char_select.c/.h`) renders a scrollable character list with create/rename/delete. A new `GAME_STATE_CHAR_SELECT` state is inserted before the existing world-select state. Game profile load/save re-pointed from `res/worlds/player.dat` (single global) to the active character's file. Existing `player.dat` auto-migrated to `res/chars/default.dat`.

**Tech Stack:** C11, SDL2, OpenGL, POSIX `opendir`/`readdir`.

## Global Constraints

- C11 standard, compiled with `-Wall -Wextra`, zero warnings.
- No comments in code.
- All headers use `#ifndef` include guards.
- Functions prefixed by module name (`character_*`, `char_select_*`).
- Use `snake_case` for everything.
- Screen size via `g_screen_w`/`g_screen_h` (from `renderer.h`), never hardcoded.
- Font scales >= 1.0 for any displayed text.
- Build: `make clean && make` in project root.
- Verification: clean build + manual playtest (no test harness).

---

## File Structure

| File | Action | Responsibility |
|------|--------|----------------|
| `src/game/character.h` | Create | `Character` type, constants, function declarations |
| `src/game/character.c` | Create | File format I/O, dir ops, name validation, defaults, migration |
| `src/engine/char_select.h` | Create | `CharSelect` type, UI function declarations |
| `src/engine/char_select.c` | Create | Scrollable list render, update, event handling, create/rename/delete |
| `src/main.c` | Modify | New enum state, `Game` fields, dispatch wiring, save/load re-point, migration call |

---

### Task 1: Character Module Core (struct, path, save, load)

**Files:**
- Create: `src/game/character.h`
- Create: `src/game/character.c`

**Interfaces:**
- Consumes: `Inventory` (from `inventory.h`), `WORLD_NAME_MAX` (from `world_select.h`)
- Produces: `Character` type, `CHAR_NAME_MAX`, `CHAR_MAGIC`, `CHAR_VERSION`, `character_path()`, `character_load()`, `character_save()`

- [ ] **Step 1: Write `src/game/character.h`**

```c
#ifndef CHARACTER_H
#define CHARACTER_H

#include <stdint.h>
#include "inventory.h"
#include "../engine/world_select.h"

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

void character_path(const char *name, char *out, size_t out_size);
int  character_load(Character *c, const char *path);
int  character_save(const Character *c, const char *path);

#endif
```

- [ ] **Step 2: Write `src/game/character.c` (core only)**

```c
#include "character.h"
#include <stdio.h>
#include <string.h>

void character_path(const char *name, char *out, size_t out_size)
{
    snprintf(out, out_size, "res/chars/%s.dat", name);
}

int character_save(const Character *c, const char *path)
{
    FILE *f = fopen(path, "wb");
    if (!f) return -1;

    fwrite(CHAR_MAGIC, 4, 1, f);
    int version = CHAR_VERSION;
    fwrite(&version, sizeof(int), 1, f);

    char name_buf[64];
    memset(name_buf, 0, sizeof(name_buf));
    snprintf(name_buf, sizeof(name_buf), "%s", c->name);
    fwrite(name_buf, sizeof(name_buf), 1, f);

    char last_buf[64];
    memset(last_buf, 0, sizeof(last_buf));
    snprintf(last_buf, sizeof(last_buf), "%s", c->last_world);
    fwrite(last_buf, sizeof(last_buf), 1, f);

    fwrite(&c->gems, sizeof(int), 1, f);
    fwrite(&c->health, sizeof(int), 1, f);
    fwrite(c->equipped, sizeof(uint16_t), 3, f);

    for (int i = 0; i < INVENTORY_SIZE; i++) {
        fwrite(&c->inventory.items[i], sizeof(uint16_t), 1, f);
        fwrite(&c->inventory.counts[i], sizeof(int), 1, f);
    }

    fclose(f);
    return 0;
}

int character_load(Character *c, const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f) return -1;

    char magic[4];
    if (fread(magic, 1, 4, f) != 4 || memcmp(magic, CHAR_MAGIC, 4) != 0) {
        fclose(f);
        return -1;
    }

    int version = 0;
    if (fread(&version, sizeof(int), 1, f) != 1) {
        fclose(f);
        return -1;
    }
    if (version != CHAR_VERSION) {
        fclose(f);
        return -1;
    }

    char name_buf[64];
    char last_buf[64];
    if (fread(name_buf, sizeof(name_buf), 1, f) != 1 ||
        fread(last_buf, sizeof(last_buf), 1, f) != 1) {
        fclose(f);
        return -1;
    }
    name_buf[sizeof(name_buf) - 1] = '\0';
    last_buf[sizeof(last_buf) - 1] = '\0';
    snprintf(c->name, sizeof(c->name), "%s", name_buf);
    snprintf(c->last_world, sizeof(c->last_world), "%s", last_buf);

    if (fread(&c->gems, sizeof(int), 1, f) != 1 ||
        fread(&c->health, sizeof(int), 1, f) != 1 ||
        fread(c->equipped, sizeof(uint16_t), 3, f) != 3) {
        fclose(f);
        return -1;
    }

    for (int i = 0; i < INVENTORY_SIZE; i++) {
        if (fread(&c->inventory.items[i], sizeof(uint16_t), 1, f) != 1 ||
            fread(&c->inventory.counts[i], sizeof(int), 1, f) != 1) {
            fclose(f);
            return -1;
        }
    }

    fclose(f);
    return 0;
}
```

- [ ] **Step 3: Build and verify**

```bash
make clean && make
```

Expected: compiles with zero warnings. The new `.c` files are picked up by `$(wildcard src/**/*.c)`. `character.o` and `char_select.o` will be built under `build/game/` and `build/engine/`. Since `character.c` has no `main`, it links as an object. (Note: if no other file references the character symbols yet, the linker will not complain — they're in the object file.)

- [ ] **Step 4: Commit**

```bash
git add src/game/character.h src/game/character.c
git commit -m "Add character module: data type, binary file format, load/save"
```

---

### Task 2: Character Module Dir Ops (list, exists, delete, rename, defaults, migration)

**Files:**
- Modify: `src/game/character.h` (add declarations)
- Modify: `src/game/character.c` (add functions)

**Interfaces:**
- Consumes: `character_path/load/save` (Task 1), `inventory_add` (existing `inventory.h`), `MAX_HEALTH` (existing `player.h`), `BLOCK_*`/`SEED_*` (existing `world.h`)
- Produces: `character_list()`, `character_exists()`, `character_delete()`, `character_rename()`, `character_name_valid()`, `character_apply_defaults()`, `character_migrate_from_profile()`

- [ ] **Step 1: Add declarations to `src/game/character.h`**

Append before the existing `#endif`:

```c
int  character_list(char names[][CHAR_NAME_MAX + 1], int *count, int max);
int  character_exists(const char *name);
int  character_delete(const char *name);
int  character_rename(const char *old_name, const char *new_name);
int  character_name_valid(const char *name);
void character_apply_defaults(Character *c);
int  character_migrate_from_profile(const char *profile_path);
```

- [ ] **Step 2: Add includes and implementations to `src/game/character.c`**

Add includes at the top (after existing includes — `stdio.h`, `string.h`):

```c
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include "player.h"
#include "../world/world.h"
```

Add the `name_cmp` comparator and directory functions after `character_load()`:

```c
static int name_cmp(const void *a, const void *b)
{
    const char *sa = (const char *)a;
    const char *sb = (const char *)b;
    return strcmp(sa, sb);
}

int character_exists(const char *name)
{
    char path[256];
    character_path(name, path, sizeof(path));
    FILE *f = fopen(path, "rb");
    if (f) { fclose(f); return 1; }
    return 0;
}

int character_name_valid(const char *name)
{
    int len = (int)strlen(name);
    if (len < 1 || len > CHAR_NAME_MAX) return 0;
    for (int i = 0; i < len; i++) {
        char c = name[i];
        if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9'))) return 0;
    }
    return 1;
}

int character_delete(const char *name)
{
    char path[256];
    character_path(name, path, sizeof(path));
    return remove(path);
}

int character_rename(const char *old_name, const char *new_name)
{
    char old_path[256];
    char new_path[256];
    character_path(old_name, old_path, sizeof(old_path));
    character_path(new_name, new_path, sizeof(new_path));
    if (rename(old_path, new_path) != 0) return -1;

    Character c;
    memset(&c, 0, sizeof(c));
    if (character_load(&c, new_path) != 0) return -1;
    snprintf(c.name, sizeof(c.name), "%s", new_name);
    return character_save(&c, new_path);
}

int character_list(char names[][CHAR_NAME_MAX + 1], int *count, int max)
{
    *count = 0;
    DIR *d = opendir("res/chars");
    if (!d) return -1;

    struct dirent *de;
    while ((de = readdir(d)) != NULL) {
        const char *nm = de->d_name;
        int len = (int)strlen(nm);
        if (len < 5) continue;
        if (strcmp(nm + len - 4, ".dat") != 0) continue;
        if (*count >= max) break;
        int blen = len - 4;
        if (blen > CHAR_NAME_MAX) blen = CHAR_NAME_MAX;
        char base[CHAR_NAME_MAX + 1];
        memcpy(base, nm, blen);
        base[blen] = '\0';
        snprintf(names[*count], CHAR_NAME_MAX + 1, "%s", base);
        (*count)++;
    }
    closedir(d);
    qsort(names, *count, sizeof(names[0]), name_cmp);
    return 0;
}

void character_apply_defaults(Character *c)
{
    memset(c, 0, sizeof(*c));
    inventory_add(&c->inventory, BLOCK_DIRT, 50);
    inventory_add(&c->inventory, BLOCK_STONE, 30);
    inventory_add(&c->inventory, BLOCK_WOOD, 20);
    inventory_add(&c->inventory, SEED_DIRT, 10);
    inventory_add(&c->inventory, SEED_GRASS, 5);
    inventory_add(&c->inventory, SEED_WOOD, 5);
    c->gems = 999999;
    c->health = MAX_HEALTH;
}

int character_migrate_from_profile(const char *profile_path)
{
    DIR *d = opendir("res/chars");
    int empty = 1;
    if (d) {
        struct dirent *de;
        while ((de = readdir(d)) != NULL) {
            int len = (int)strlen(de->d_name);
            if (len >= 5 && strcmp(de->d_name + len - 4, ".dat") == 0) {
                empty = 0;
                break;
            }
        }
        closedir(d);
    }
    if (!empty) return 0;

    FILE *f = fopen(profile_path, "rb");
    if (!f) return 0;
    fclose(f);

    Character c;
    memset(&c, 0, sizeof(c));
    uint16_t equipped[3] = {0, 0, 0};
    if (inventory_load_profile(&c.inventory, &c.gems, &c.health, equipped, profile_path) != 0) {
        return -1;
    }
    c.equipped[0] = equipped[0];
    c.equipped[1] = equipped[1];
    c.equipped[2] = equipped[2];
    snprintf(c.name, sizeof(c.name), "default");
    c.last_world[0] = '\0';

    char out[256];
    character_path("default", out, sizeof(out));
    return character_save(&c, out);
}
```

- [ ] **Step 3: Build and verify**

```bash
make clean && make
```

Expected: zero warnings.

- [ ] **Step 4: Commit**

```bash
git add src/game/character.h src/game/character.c
git commit -m "Add character dir ops: list, exists, delete, rename, name_valid, defaults, migration"
```

---

### Task 3: char_select Module (Complete: list, scroll, select, create, rename, delete)

**Files:**
- Create: `src/engine/char_select.h`
- Create: `src/engine/char_select.c`

**Interfaces:**
- Consumes: `Character`/`character_*` (Tasks 1–2), `renderer_*` (existing), `SDL_Event` (existing), `g_screen_w`/`g_screen_h` (existing `renderer.h`)
- Produces: `CharSelect` type, `char_select_init/refresh/handle_event/update/render()`, `CHAR_LIST_MAX`

- [ ] **Step 1: Write `src/engine/char_select.h`**

```c
#ifndef CHAR_SELECT_H
#define CHAR_SELECT_H

#include "renderer.h"
#include "../game/character.h"

#define CHAR_LIST_MAX 64

typedef struct {
    char names[CHAR_LIST_MAX][CHAR_NAME_MAX + 1];
    int  gems_snap[CHAR_LIST_MAX];
    char last_world_snap[CHAR_LIST_MAX][WORLD_NAME_MAX + 1];
    int  count;
    int  scroll;
    int  hovered;
    int  selected;

    int  creating;
    int  renaming;
    int  deleting;
    char input_text[CHAR_NAME_MAX + 1];
    int  input_cursor;
    float cursor_timer;
    int  mouse_clicked;
} CharSelect;

void char_select_init(CharSelect *cs);
void char_select_refresh(CharSelect *cs);
void char_select_handle_event(CharSelect *cs, SDL_Event *e);
void char_select_update(CharSelect *cs, float dt);
void char_select_render(CharSelect *cs, Renderer *r);

#endif
```

- [ ] **Step 2: Write `src/engine/char_select.c`**

```c
#include "char_select.h"
#include <string.h>
#include <stdio.h>

#define CS_ROW_H 30
#define CS_ROW_W 440
#define CS_VISIBLE 6
#define CS_BTN_W 26

static int point_in_rect(int px, int py, int rx, int ry, int rw, int rh)
{
    return px >= rx && px < rx + rw && py >= ry && py < ry + rh;
}

static void clamp_scroll(CharSelect *cs)
{
    int max_scroll = cs->count - CS_VISIBLE;
    if (max_scroll < 0) max_scroll = 0;
    if (cs->scroll < 0) cs->scroll = 0;
    if (cs->scroll > max_scroll) cs->scroll = max_scroll;
}

static void commit_input(CharSelect *cs);

void char_select_init(CharSelect *cs)
{
    memset(cs, 0, sizeof(*cs));
    cs->selected = -1;
    cs->hovered = -1;
    cs->renaming = -1;
    cs->deleting = -1;
    cs->creating = 0;
    char_select_refresh(cs);
}

void char_select_refresh(CharSelect *cs)
{
    cs->count = 0;
    character_list(cs->names, &cs->count, CHAR_LIST_MAX);
    for (int i = 0; i < cs->count; i++) {
        Character c;
        memset(&c, 0, sizeof(c));
        char path[256];
        character_path(cs->names[i], path, sizeof(path));
        if (character_load(&c, path) == 0) {
            cs->gems_snap[i] = c.gems;
            snprintf(cs->last_world_snap[i], sizeof(cs->last_world_snap[i]), "%s", c.last_world);
        } else {
            cs->gems_snap[i] = 0;
            cs->last_world_snap[i][0] = '\0';
        }
    }
    clamp_scroll(cs);
}

void char_select_handle_event(CharSelect *cs, SDL_Event *e)
{
    if (e->type == SDL_MOUSEBUTTONDOWN && e->button.button == SDL_BUTTON_LEFT)
        cs->mouse_clicked = 1;

    if (e->type == SDL_MOUSEWHEEL) {
        cs->scroll -= e->wheel.y;
        clamp_scroll(cs);
    }

    if (cs->deleting >= 0) {
        if (e->type == SDL_KEYDOWN) {
            if (e->key.keysym.sym == SDLK_y) {
                character_delete(cs->names[cs->deleting]);
                cs->deleting = -1;
                char_select_refresh(cs);
            } else if (e->key.keysym.sym == SDLK_n || e->key.keysym.sym == SDLK_ESCAPE) {
                cs->deleting = -1;
            }
        }
        return;
    }

    int text_mode = cs->creating || cs->renaming >= 0;
    if (text_mode) {
        if (e->type == SDL_KEYDOWN) {
            if (e->key.keysym.sym == SDLK_BACKSPACE && cs->input_cursor > 0) {
                cs->input_cursor--;
                cs->input_text[cs->input_cursor] = '\0';
            } else if (e->key.keysym.sym == SDLK_RETURN) {
                commit_input(cs);
            } else if (e->key.keysym.sym == SDLK_ESCAPE) {
                cs->creating = 0;
                cs->renaming = -1;
                cs->input_text[0] = '\0';
                cs->input_cursor = 0;
            }
        } else if (e->type == SDL_TEXTINPUT) {
            if (cs->input_cursor < CHAR_NAME_MAX) {
                char c = e->text.text[0];
                if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) {
                    if (c >= 'A' && c <= 'Z') c = c - 'A' + 'a';
                    cs->input_text[cs->input_cursor] = c;
                    cs->input_cursor++;
                }
            }
        }
    }
}

static void commit_input(CharSelect *cs)
{
    if (cs->input_cursor == 0) {
        cs->creating = 0;
        cs->renaming = -1;
        return;
    }
    if (!character_name_valid(cs->input_text)) {
        cs->creating = 0;
        cs->renaming = -1;
        cs->input_text[0] = '\0';
        cs->input_cursor = 0;
        return;
    }

    if (cs->creating) {
        if (character_exists(cs->input_text)) {
            cs->creating = 0;
            cs->input_text[0] = '\0';
            cs->input_cursor = 0;
            return;
        }
        Character c;
        character_apply_defaults(&c);
        snprintf(c.name, sizeof(c.name), "%s", cs->input_text);
        char path[256];
        character_path(cs->input_text, path, sizeof(path));
        character_save(&c, path);
        char_select_refresh(cs);
        for (int i = 0; i < cs->count; i++) {
            if (strcmp(cs->names[i], cs->input_text) == 0) {
                cs->selected = i;
                break;
            }
        }
        cs->creating = 0;
        cs->input_text[0] = '\0';
        cs->input_cursor = 0;
        return;
    }

    if (cs->renaming >= 0) {
        const char *old = cs->names[cs->renaming];
        if (strcmp(old, cs->input_text) != 0) {
            if (character_exists(cs->input_text)) {
                cs->renaming = -1;
                cs->input_text[0] = '\0';
                cs->input_cursor = 0;
                return;
            }
            character_rename(old, cs->input_text);
        }
        char_select_refresh(cs);
        cs->renaming = -1;
        cs->input_text[0] = '\0';
        cs->input_cursor = 0;
    }
}

void char_select_update(CharSelect *cs, float dt)
{
    cs->cursor_timer += dt;
    if (cs->cursor_timer >= 1.0f) cs->cursor_timer -= 1.0f;

    if (cs->deleting >= 0) {
        cs->mouse_clicked = 0;
        return;
    }

    int mx, my;
    SDL_GetMouseState(&mx, &my);

    int panel_x = (g_screen_w - CS_ROW_W) / 2;
    int list_y = 100;
    int body_w = CS_ROW_W - 2 * (CS_BTN_W + 4);
    int text_mode = cs->creating || cs->renaming >= 0;

    if (!text_mode) {
        int new_btn_w = 160, new_btn_h = 28;
        int new_x = panel_x + (CS_ROW_W - new_btn_w) / 2;
        int new_y = list_y + CS_VISIBLE * CS_ROW_H + 16;
        if (point_in_rect(mx, my, new_x, new_y, new_btn_w, new_btn_h) && cs->mouse_clicked) {
            cs->creating = 1;
            cs->input_text[0] = '\0';
            cs->input_cursor = 0;
        }
    }

    cs->hovered = -1;
    for (int i = 0; i < CS_VISIBLE; i++) {
        int idx = cs->scroll + i;
        if (idx >= cs->count) break;
        int ry = list_y + i * CS_ROW_H;

        if (!text_mode) {
            if (point_in_rect(mx, my, panel_x, ry, body_w, CS_ROW_H)) {
                cs->hovered = idx;
                if (cs->mouse_clicked) cs->selected = idx;
            }
            int rx_btn = panel_x + body_w + 4;
            if (point_in_rect(mx, my, rx_btn, ry, CS_BTN_W, CS_ROW_H) && cs->mouse_clicked) {
                snprintf(cs->input_text, sizeof(cs->input_text), "%s", cs->names[idx]);
                cs->input_cursor = (int)strlen(cs->input_text);
                cs->renaming = idx;
            }
            int dx_btn = rx_btn + CS_BTN_W + 4;
            if (point_in_rect(mx, my, dx_btn, ry, CS_BTN_W, CS_ROW_H) && cs->mouse_clicked) {
                cs->deleting = idx;
            }
        }
    }

    cs->mouse_clicked = 0;
}

void char_select_render(CharSelect *cs, Renderer *r)
{
    renderer_begin_ui(r);
    renderer_draw_rect(r, 0, 0, g_screen_w, g_screen_h, 0.05f, 0.05f, 0.15f, 1.0f);

    int panel_x = (g_screen_w - CS_ROW_W) / 2;
    int list_y = 100;
    int body_w = CS_ROW_W - 2 * (CS_BTN_W + 4);

    const char *title = "SELECT CHARACTER";
    int tw = renderer_text_width(r, title, 3.0f);
    renderer_draw_text(r, title, panel_x + (CS_ROW_W - tw) / 2, list_y - 50, 3.0f, 1.0f, 1.0f, 1.0f);

    for (int i = 0; i < CS_VISIBLE; i++) {
        int idx = cs->scroll + i;
        if (idx >= cs->count) break;
        int ry = list_y + i * CS_ROW_H;

        float br = 0.15f, bg = 0.15f, bb = 0.22f;
        if (cs->hovered == idx) { br = 0.25f; bg = 0.25f; bb = 0.38f; }
        renderer_draw_rect(r, panel_x, ry, body_w, CS_ROW_H - 2, br, bg, bb, 1.0f);

        if (cs->renaming != idx) {
            const char *nm = cs->names[idx];
            renderer_draw_text(r, nm, panel_x + 8, ry + 8, 1.3f, 1.0f, 1.0f, 1.0f);

            char gbuf[32];
            snprintf(gbuf, sizeof(gbuf), "%d g", cs->gems_snap[idx]);
            int gw = renderer_text_width(r, gbuf, 1.1f);
            renderer_draw_text(r, gbuf, panel_x + body_w - gw - 8, ry + 9, 1.1f, 1.0f, 0.85f, 0.3f);

            if (cs->last_world_snap[idx][0]) {
                char lbuf[48];
                snprintf(lbuf, sizeof(lbuf), "last: %s", cs->last_world_snap[idx]);
                int nw = renderer_text_width(r, cs->names[idx], 1.3f);
                renderer_draw_text(r, lbuf, panel_x + 8 + nw + 16, ry + 10, 1.0f, 0.6f, 0.6f, 0.6f);
            }
        }

        int rx_btn = panel_x + body_w + 4;
        renderer_draw_rect(r, rx_btn, ry, CS_BTN_W, CS_ROW_H - 2, 0.2f, 0.2f, 0.3f, 1.0f);
        int rw = renderer_text_width(r, "R", 1.2f);
        renderer_draw_text(r, "R", rx_btn + (CS_BTN_W - rw) / 2, ry + 8, 1.2f, 1.0f, 1.0f, 1.0f);

        int dx_btn = rx_btn + CS_BTN_W + 4;
        renderer_draw_rect(r, dx_btn, ry, CS_BTN_W, CS_ROW_H - 2, 0.35f, 0.15f, 0.15f, 1.0f);
        int xw = renderer_text_width(r, "X", 1.2f);
        renderer_draw_text(r, "X", dx_btn + (CS_BTN_W - xw) / 2, ry + 8, 1.2f, 1.0f, 1.0f, 1.0f);
    }

    int new_btn_w = 160, new_btn_h = 28;
    int new_x = panel_x + (CS_ROW_W - new_btn_w) / 2;
    int new_y = list_y + CS_VISIBLE * CS_ROW_H + 16;

    if (cs->creating) {
        renderer_draw_rect(r, new_x, new_y, new_btn_w, new_btn_h, 0.2f, 0.2f, 0.3f, 1.0f);
        renderer_draw_text(r, cs->input_text, new_x + 8, new_y + 8, 1.3f, 1.0f, 1.0f, 1.0f);
        if (cs->cursor_timer < 0.5f) {
            int cw = renderer_text_width(r, cs->input_text, 1.3f);
            renderer_draw_rect(r, new_x + 8 + cw, new_y + 6, 2, 18, 1.0f, 1.0f, 1.0f, 1.0f);
        }
    } else {
        renderer_draw_rect(r, new_x, new_y, new_btn_w, new_btn_h, 0.2f, 0.5f, 0.2f, 1.0f);
        int lw = renderer_text_width(r, "New Character", 1.3f);
        renderer_draw_text(r, "New Character", new_x + (new_btn_w - lw) / 2, new_y + 8, 1.3f, 1.0f, 1.0f, 1.0f);
    }

    if (cs->renaming >= 0) {
        for (int i = 0; i < CS_VISIBLE; i++) {
            int idx = cs->scroll + i;
            if (idx == cs->renaming) {
                int ry = list_y + i * CS_ROW_H;
                renderer_draw_rect(r, panel_x, ry, body_w, CS_ROW_H - 2, 0.2f, 0.2f, 0.3f, 1.0f);
                renderer_draw_text(r, cs->input_text, panel_x + 8, ry + 8, 1.3f, 1.0f, 1.0f, 1.0f);
                if (cs->cursor_timer < 0.5f) {
                    int cw = renderer_text_width(r, cs->input_text, 1.3f);
                    renderer_draw_rect(r, panel_x + 8 + cw, ry + 6, 2, 18, 1.0f, 1.0f, 1.0f, 1.0f);
                }
                break;
            }
        }
    }

    if (cs->deleting >= 0) {
        renderer_draw_rect(r, 0, 0, g_screen_w, g_screen_h, 0.0f, 0.0f, 0.0f, 0.6f);
        char msg[64];
        snprintf(msg, sizeof(msg), "Delete %s? Y/N", cs->names[cs->deleting]);
        int mw = renderer_text_width(r, msg, 2.0f);
        int bx = (g_screen_w - mw - 24) / 2;
        int by = g_screen_h / 2 - 30;
        renderer_draw_rect(r, bx, by, mw + 24, 48, 0.15f, 0.1f, 0.1f, 1.0f);
        renderer_draw_text(r, msg, bx + 12, by + 14, 2.0f, 1.0f, 1.0f, 1.0f);
    }

    const char *hint = "Click: Play   R: Rename   X: Delete   Wheel: Scroll   ESC: Back/Cancel";
    int hw = renderer_text_width(r, hint, 1.0f);
    renderer_draw_text(r, hint, (g_screen_w - hw) / 2, g_screen_h - 20, 1.0f, 0.5f, 0.5f, 0.5f);

    renderer_end_ui(r);
}
```

- [ ] **Step 3: Build and verify**

```bash
make clean && make
```

Expected: zero warnings. The `char_select.o` compiles and all symbols resolve (through `character.o` and `renderer.o`).

- [ ] **Step 4: Commit**

```bash
git add src/engine/char_select.h src/engine/char_select.c
git commit -m "Add char_select UI module: scrollable list, select, create, rename, delete"
```

---

### Task 4: Wire GAME_STATE_CHAR_SELECT into main.c

**Files:**
- Modify: `src/main.c`

**Interfaces:**
- Consumes: `CharSelect`/`char_select_*` (Task 3), `character_migrate_from_profile` (Task 2, declared but NOT called yet)
- Produces: reachable character-select screen, `g->current_char_name` set on selection, correct ESC transitions

The key change: the game now boots to `GAME_STATE_CHAR_SELECT`. Selecting a character sets `current_char_name` and transitions to `GAME_STATE_MENU` (world select). ESC at world select goes back to `GAME_STATE_CHAR_SELECT`. ESC at char select quits. `game_save_all` and `game_enter_world` still use the old `PROFILE_PATH`/`player.dat` — they will be re-pointed in Task 5.

- [ ] **Step 1: Add includes**

After existing `#include` block (after `"game/explosive.h"`), add:

```c
#include "engine/char_select.h"
#include "game/character.h"
```

- [ ] **Step 2: Add new game state and global CharSelect**

Replace the existing `GameState` enum at line 33:

Old:
```c
typedef enum {
    GAME_STATE_MENU,
    GAME_STATE_PLAYING
} GameState;
```

New:
```c
typedef enum {
    GAME_STATE_CHAR_SELECT,
    GAME_STATE_MENU,
    GAME_STATE_PLAYING
} GameState;
```

Replace the existing static state and world_select globals (around line 38–40):

Old:
```c
static int g_running = 1;
static GameState g_game_state = GAME_STATE_MENU;
static WorldSelect g_world_select;
```

New:
```c
static int g_running = 1;
static GameState g_game_state = GAME_STATE_CHAR_SELECT;
static WorldSelect g_world_select;
static CharSelect g_char_select;
```

- [ ] **Step 3: Add Game struct fields**

In the `Game` struct (around line 60), after `int exit_confirm_active;`, add:

```c
char current_char_name[CHAR_NAME_MAX + 1];
char last_world[WORLD_NAME_MAX + 1];
```

Full last lines of struct should end:
```c
int exit_confirm_active;
char current_char_name[CHAR_NAME_MAX + 1];
char last_world[WORLD_NAME_MAX + 1];
} Game;
```

- [ ] **Step 4: Create res/chars directory**

In `ensure_worlds_dir` (around line 115), add the new directory:

Old:
```c
static void ensure_worlds_dir(void) {
    mkdir("res", 0755);
    mkdir("res/worlds", 0755);
}
```

New:
```c
static void ensure_worlds_dir(void) {
    mkdir("res", 0755);
    mkdir("res/worlds", 0755);
    mkdir("res/chars", 0755);
}
```

- [ ] **Step 5: Init char_select in game_init**

In `game_init` (around line 186), after `world_select_init(&g_world_select);` (around line 203), add:

```c
    character_migrate_from_profile(PROFILE_PATH);
    char_select_init(&g_char_select);
```

- [ ] **Step 6: Wire event dispatch**

Replace the `GAME_STATE_MENU` event branch (lines 215–221):

Old:
```c
        if (g_game_state == GAME_STATE_MENU) {
            if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) {
                g_running = 0;
                return;
            }
            world_select_handle_event(&g_world_select, &e);
            continue;
        }
```

New:
```c
        if (g_game_state == GAME_STATE_CHAR_SELECT) {
            if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) {
                g_running = 0;
                return;
            }
            char_select_handle_event(&g_char_select, &e);
            continue;
        }

        if (g_game_state == GAME_STATE_MENU) {
            if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) {
                g_game_state = GAME_STATE_CHAR_SELECT;
                char_select_init(&g_char_select);
                continue;
            }
            world_select_handle_event(&g_world_select, &e);
            continue;
        }
```

- [ ] **Step 7: Wire update dispatch**

Replace the `GAME_STATE_MENU` update branch (lines 269–277):

Old:
```c
    if (g_game_state == GAME_STATE_MENU) {
        world_select_update(&g_world_select, dt);
        if (g_world_select.submitted) {
            g_world_select.submitted = 0;
            game_enter_world(g, g_world_select.input_text);
        }
        input_update(&g->input);
        return;
    }
```

New:
```c
    if (g_game_state == GAME_STATE_CHAR_SELECT) {
        char_select_update(&g_char_select, dt);
        if (g_char_select.selected >= 0) {
            snprintf(g->current_char_name, sizeof(g->current_char_name), "%s", g_char_select.names[g_char_select.selected]);
            g_char_select.selected = -1;
            world_select_init(&g_world_select);
            g_game_state = GAME_STATE_MENU;
        }
        input_update(&g->input);
        return;
    }

    if (g_game_state == GAME_STATE_MENU) {
        world_select_update(&g_world_select, dt);
        if (g_world_select.submitted) {
            g_world_select.submitted = 0;
            game_enter_world(g, g_world_select.input_text);
        }
        input_update(&g->input);
        return;
    }
```

- [ ] **Step 8: Wire render dispatch**

Before the existing `GAME_STATE_MENU` render branch (line 407), prepend:

```c
    if (g_game_state == GAME_STATE_CHAR_SELECT) {
        renderer_clear(&g->renderer, 0.05f, 0.05f, 0.15f);
        char_select_render(&g_char_select, &g->renderer);
        renderer_present(&g->renderer);
        return;
    }
```

- [ ] **Step 9: Build and verify**

```bash
make clean && make
```

Expected: zero warnings.

- [ ] **Step 10: Manual playtest — character selection flow**

1. Run the game (`./growtopia` or `make run`). Observe the dark char-select screen with "SELECT CHARACTER" title.
2. If you have an existing `res/worlds/player.dat`, the list should show "default" (migrated). Otherwise the list is empty with only "New Character" button.
3. Click "New Character", type "test", press Enter. "test" appears in the list.
4. Click the "test" row. It transitions to world select (same as before).
5. Type a world name, enter. The game enters the world with the old `player.dat` profile (gems/inventory from old profile or defaults). This is expected — Task 5 will re-point to character files.
6. In-game, press ESC, confirm Y. You return to world select. Press ESC again. You return to char select (shows "test" in the list).
7. Press ESC at char select. Game quits.

- [ ] **Step 11: Commit**

```bash
git add src/main.c
git commit -m "Wire GAME_STATE_CHAR_SELECT: new state, dispatch, Game fields, ESC transitions"
```

---

### Task 5: Re-point Save/Load to Character Files + Migration Integration

**Files:**
- Modify: `src/main.c` (re-point `game_save_all` and `game_enter_world`)

**Interfaces:**
- Consumes: `character_load/save`, `character_path` (Tasks 1–2), `CHAR_NAME_MAX`, `CHAR_LIST_MAX` (existing), `game_enter_world` context (current `g->current_char_name` set by Task 4)
- Produces: per-character persistence (gems/inventory/health/equipped survive world exit and character re-select), `last_world` tracking, `player.dat` migrated to `default` on first run

- [ ] **Step 1: Re-point `game_save_all`**

Replace the entire `game_save_all` function (lines 103–112):

Old:
```c
static void game_save_all(Game *g) {
    g->world.spawn_x = g->player.x;
    g->world.spawn_y = g->player.y;
    char path[256];
    world_build_path(path, sizeof(path), g->current_world_name, "wld");
    world_save(&g->world, path);
    uint16_t equipped[3] = {g->player.equipped_hat, g->player.equipped_shirt, g->player.equipped_pants};
    inventory_save_profile(&g->inventory, g->player.gems, g->player.health, equipped, PROFILE_PATH);
    printf("Game saved.\n");
}
```

New:
```c
static void game_save_all(Game *g) {
    g->world.spawn_x = g->player.x;
    g->world.spawn_y = g->player.y;
    char path[256];
    world_build_path(path, sizeof(path), g->current_world_name, "wld");
    world_save(&g->world, path);

    Character c;
    memset(&c, 0, sizeof(c));
    snprintf(c.name, sizeof(c.name), "%s", g->current_char_name);
    snprintf(c.last_world, sizeof(c.last_world), "%s", g->current_world_name);
    c.gems = g->player.gems;
    c.health = g->player.health;
    c.equipped[0] = g->player.equipped_hat;
    c.equipped[1] = g->player.equipped_shirt;
    c.equipped[2] = g->player.equipped_pants;
    c.inventory = g->inventory;
    char cpath[256];
    character_path(g->current_char_name, cpath, sizeof(cpath));
    character_save(&c, cpath);
    printf("Game saved.\n");
}
```

- [ ] **Step 2: Re-point `game_enter_world` profile loading**

In `game_enter_world`, replace the profile-load block (lines 147–166):

Old:
```c
    int profile_loaded = 0;
    uint16_t equipped[3] = {0, 0, 0};
    if (inventory_load_profile(&g->inventory, &g->player.gems, &g->player.health, equipped, PROFILE_PATH) == 0) {
        g->player.equipped_hat = equipped[0];
        g->player.equipped_shirt = equipped[1];
        g->player.equipped_pants = equipped[2];
        profile_loaded = 1;
    }

    if (!profile_loaded) {
        inventory_init(&g->inventory);
        inventory_add(&g->inventory, BLOCK_DIRT, 50);
        inventory_add(&g->inventory, BLOCK_STONE, 30);
        inventory_add(&g->inventory, BLOCK_WOOD, 20);
        inventory_add(&g->inventory, SEED_DIRT, 10);
        inventory_add(&g->inventory, SEED_GRASS, 5);
        inventory_add(&g->inventory, SEED_WOOD, 5);
        g->player.gems = 999999;
        g->player.health = MAX_HEALTH;
    }
```

New:
```c
    int profile_loaded = 0;
    Character c;
    memset(&c, 0, sizeof(c));
    char cpath[256];
    character_path(g->current_char_name, cpath, sizeof(cpath));
    if (character_load(&c, cpath) == 0) {
        g->inventory = c.inventory;
        g->player.gems = c.gems;
        g->player.health = c.health;
        g->player.equipped_hat = c.equipped[0];
        g->player.equipped_shirt = c.equipped[1];
        g->player.equipped_pants = c.equipped[2];
        snprintf(g->last_world, sizeof(g->last_world), "%s", c.last_world);
        profile_loaded = 1;
    }

    if (!profile_loaded) {
        inventory_init(&g->inventory);
        inventory_add(&g->inventory, BLOCK_DIRT, 50);
        inventory_add(&g->inventory, BLOCK_STONE, 30);
        inventory_add(&g->inventory, BLOCK_WOOD, 20);
        inventory_add(&g->inventory, SEED_DIRT, 10);
        inventory_add(&g->inventory, SEED_GRASS, 5);
        inventory_add(&g->inventory, SEED_WOOD, 5);
        g->player.gems = 999999;
        g->player.health = MAX_HEALTH;
    }
```

- [ ] **Step 3: Build**

```bash
make clean && make
```

Expected: zero warnings.

- [ ] **Step 4: Manual playtest — full per-character persistence**

**Test A: Per-character gems survive**
1. Run the game.
2. Select a character (or create one). Enter a world.
3. Note the gem count on the HUD. Open the inventory (E), note the items.
4. Press ESC, confirm Y, return to world select, then ESC to char select.
5. Select the same character again, enter a world.
6. Verify gem count and inventory items match exactly from step 3.

**Test B: Characters are independent**
1. Create character `char1`, enter a world, note gems = 999999 (or whatever default).
2. Enter world, exit to char select.
3. Create character `char2`, enter the same world, note gems = 999999 (independent default).
4. Exit to char select, select `char1` again. Verify `char1`'s state is unchanged by playing as `char2`.

**Test C: last_world displayed**
1. Play `char1` in world "start". Exit to char select.
2. The `char1` row should display "last: start".
3. Play `char1` in a different world "test". Exit to char select.
4. The `char1` row should display "last: test".

**Test D: player.dat migration**
1. Stop the game.
2. Delete `res/chars/` directory entirely.
3. Ensure `res/worlds/player.dat` still exists (from before multi-character was active).
4. Start the game. The char-select list should show "default" with the gems/inventory from the old `player.dat`.
5. Entering a world with "default" should restore the old game's profile.

**Test E: Rename and Delete**
1. Create character `old`. Exit to char select.
2. Click R, type `new`, press Enter. "new" appears, "old" gone.
3. Select "new", enter world. Verify character data is preserved (gems intact).
4. Create `temp`. Exit to char select. Click X on `temp`. Press Y. `temp` removed.
5. Click X on `temp` again (doesn't exist). Press N. Cancel works (no deletion of other chars).

- [ ] **Step 5: Commit**

```bash
git add src/main.c
git commit -m "Re-point save/load to per-character files; last_world tracking; player.dat migration"
```

---

## Verification Summary

- `make clean && make` compiles with zero warnings.
- Five manual playtest scenarios (persistence, independence, last_world, migration, rename/delete) confirm correctness end-to-end.
