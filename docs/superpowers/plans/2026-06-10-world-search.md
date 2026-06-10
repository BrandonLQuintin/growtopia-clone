# World Search Screen Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a Growtopia-style world search screen that appears at startup, letting players type a world name to load or create, with ESC-to-return during gameplay.

**Architecture:** A `GameState` machine (`GAME_STATE_MENU` / `GAME_STATE_PLAYING`) wraps the main loop. A new `world_select` module handles the menu screen rendering and input. World saves use dynamic paths (`res/worlds/<name>.wld`). A shared `player.dat` stores inventory/gems/health across worlds.

**Tech Stack:** C11, SDL2, OpenGL, existing renderer/UI/camera modules.

---

### Task 1: Add `spawn_x`/`spawn_y` to World struct and update save format

**Files:**
- Modify: `src/world/world.h:86-93`
- Modify: `src/world/world.c:142-210`

- [ ] **Step 1: Add spawn fields to World struct in `src/world/world.h`**

In the `World` struct, add `spawn_x` and `spawn_y` after the `name` field:

```c
typedef struct {
    int width;
    int height;
    Tile *tiles;
    char name[64];
    float spawn_x;
    float spawn_y;
    char sign_texts[SIGN_TABLE_SIZE][SIGN_TEXT_MAX_LEN + 1];
    int sign_count;
} World;
```

- [ ] **Step 2: Update `world_init` to initialize spawn position in `src/world/world.c`**

In `world_init`, after `memset(w->name, 0, sizeof(w->name));`, add:

```c
w->spawn_x = (float)(width / 2 * TILE_SIZE);
w->spawn_y = 10.0f * TILE_SIZE;
```

- [ ] **Step 3: Update `world_save` to write version 3 format in `src/world/world.c`**

Replace the version constant and add name + spawn writes after the height write. Change `world_save` from:

```c
uint32_t version = 2;
```

to:

```c
uint32_t version = 3;
```

After the `fwrite(&w->height, ...)` line and before the tile array write, add:

```c
if (fwrite(w->name, 1, 64, f) != 64) { fclose(f); return -1; }
if (fwrite(&w->spawn_x, sizeof(float), 1, f) != 1) { fclose(f); return -1; }
if (fwrite(&w->spawn_y, sizeof(float), 1, f) != 1) { fclose(f); return -1; }
```

- [ ] **Step 4: Update `world_load` to handle version 3 in `src/world/world.c`**

Change the version guard from:

```c
if (version < 1 || version > 2) { fclose(f); return -1; }
```

to:

```c
if (version < 1 || version > 3) { fclose(f); return -1; }
```

After the `world_init` call and before the tile array read, add v3 handling:

```c
if (version >= 3) {
    if (fread(w->name, 1, 64, f) != 64) { world_free(w); fclose(f); return -1; }
    if (fread(&w->spawn_x, sizeof(float), 1, f) != 1) { world_free(w); fclose(f); return -1; }
    if (fread(&w->spawn_y, sizeof(float), 1, f) != 1) { world_free(w); fclose(f); return -1; }
} else {
    memset(w->name, 0, sizeof(w->name));
    w->spawn_x = (float)(w->width / 2 * TILE_SIZE);
    w->spawn_y = 10.0f * TILE_SIZE;
}
```

The existing `memset(w->name, 0, sizeof(w->name));` in `world_init` should remain unchanged — it's still needed for fresh world creation. The v2 else-branch above also zeroes name, which is redundant but harmless.

- [ ] **Step 5: Add `world_set_name`, `world_exists`, and `world_build_path` functions in `src/world/world.c`**

Add these after the `world_is_solid` function:

```c
void world_set_name(World *w, const char *name) {
    memset(w->name, 0, sizeof(w->name));
    snprintf(w->name, sizeof(w->name), "%s", name);
}

int world_exists(const char *name) {
    char path[256];
    snprintf(path, sizeof(path), "res/worlds/%s.wld", name);
    FILE *f = fopen(path, "rb");
    if (f) { fclose(f); return 1; }
    return 0;
}

void world_build_path(char *buf, int buf_size, const char *name, const char *ext) {
    snprintf(buf, buf_size, "res/worlds/%s.%s", name, ext);
}
```

- [ ] **Step 6: Add declarations to `src/world/world.h`**

Before the `#endif`, add:

```c
void world_set_name(World *w, const char *name);
int world_exists(const char *name);
void world_build_path(char *buf, int buf_size, const char *name, const char *ext);
```

- [ ] **Step 7: Build and verify no warnings**

Run: `make clean && make`
Expected: Compiles with zero warnings.

- [ ] **Step 8: Commit**

```bash
git add src/world/world.h src/world/world.c
git commit -m "Add spawn_x/spawn_y to World, update save format to v3, add world_exists/world_build_path helpers"
```

---

### Task 2: Add shared player profile save/load to inventory module

**Files:**
- Modify: `src/game/inventory.h`
- Modify: `src/game/inventory.c`

- [ ] **Step 1: Add profile function declarations to `src/game/inventory.h`**

Before the `#endif`, add:

```c
int inventory_save_profile(Inventory *inv, int gems, int health, const char *path);
int inventory_load_profile(Inventory *inv, int *gems, int *health, const char *path);
```

- [ ] **Step 2: Implement `inventory_save_profile` in `src/game/inventory.c`**

Add after `inventory_load`:

```c
int inventory_save_profile(Inventory *inv, int gems, int health, const char *path)
{
    FILE *f = fopen(path, "wb");
    if (!f)
        return -1;
    for (int i = 0; i < INVENTORY_SIZE; i++) {
        fwrite(&inv->items[i], sizeof(uint16_t), 1, f);
        fwrite(&inv->counts[i], sizeof(int), 1, f);
    }
    fwrite(&gems, sizeof(int), 1, f);
    fwrite(&health, sizeof(int), 1, f);
    fclose(f);
    return 0;
}
```

- [ ] **Step 3: Implement `inventory_load_profile` in `src/game/inventory.c`**

Add after `inventory_save_profile`:

```c
int inventory_load_profile(Inventory *inv, int *gems, int *health, const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f)
        return -1;
    for (int i = 0; i < INVENTORY_SIZE; i++) {
        if (fread(&inv->items[i], sizeof(uint16_t), 1, f) != 1 ||
            fread(&inv->counts[i], sizeof(int), 1, f) != 1) {
            fclose(f);
            return -1;
        }
    }
    if (fread(gems, sizeof(int), 1, f) != 1 ||
        fread(health, sizeof(int), 1, f) != 1) {
        fclose(f);
        return -1;
    }
    fclose(f);
    return 0;
}
```

- [ ] **Step 4: Build and verify no warnings**

Run: `make clean && make`
Expected: Compiles with zero warnings.

- [ ] **Step 5: Commit**

```bash
git add src/game/inventory.h src/game/inventory.c
git commit -m "Add shared player profile save/load (inventory + gems + health)"
```

---

### Task 3: Create `world_select` module

**Files:**
- Create: `src/engine/world_select.h`
- Create: `src/engine/world_select.c`

- [ ] **Step 1: Create `src/engine/world_select.h`**

```c
#ifndef WORLD_SELECT_H
#define WORLD_SELECT_H

#include "renderer.h"

#define WORLD_NAME_MAX 20
#define RECENT_WORLDS_MAX 8

typedef struct {
    char input_text[WORLD_NAME_MAX + 1];
    int input_cursor;
    float cursor_timer;
    char recent_names[RECENT_WORLDS_MAX][WORLD_NAME_MAX + 1];
    int recent_count;
    int hovered_recent;
    int hovered_enter;
    int mouse_clicked;
    int submitted;
} WorldSelect;

void world_select_init(WorldSelect *ws);
void world_select_handle_event(WorldSelect *ws, SDL_Event *e);
void world_select_update(WorldSelect *ws, float dt);
void world_select_render(WorldSelect *ws, Renderer *r);
void world_select_load_recent(WorldSelect *ws);
void world_select_add_recent(WorldSelect *ws, const char *name);

#endif
```

- [ ] **Step 2: Create `src/engine/world_select.c`**

```c
#include "world_select.h"
#include "renderer.h"
#include <SDL2/SDL.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

static int point_in_rect(int px, int py, int rx, int ry, int rw, int rh)
{
    return px >= rx && px < rx + rw && py >= ry && py < ry + rh;
}

void world_select_init(WorldSelect *ws)
{
    memset(ws, 0, sizeof(WorldSelect));
    ws->input_cursor = 0;
    ws->cursor_timer = 0;
    ws->submitted = 0;
    world_select_load_recent(ws);
}

void world_select_handle_event(WorldSelect *ws, SDL_Event *e)
{
    if (e->type == SDL_MOUSEBUTTONDOWN && e->button.button == SDL_BUTTON_LEFT) {
        ws->mouse_clicked = 1;
    }
    if (e->type == SDL_KEYDOWN) {
        if (e->key.keysym.sym == SDLK_BACKSPACE && ws->input_cursor > 0) {
            ws->input_cursor--;
            ws->input_text[ws->input_cursor] = '\0';
        }
        if (e->key.keysym.sym == SDLK_RETURN) {
            if (ws->input_cursor > 0) {
                ws->submitted = 1;
            }
        }
    }
    if (e->type == SDL_TEXTINPUT) {
        if (ws->input_cursor < WORLD_NAME_MAX) {
            char c = e->text.text[0];
            if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) {
                if (c >= 'A' && c <= 'Z') c = c - 'A' + 'a';
                ws->input_text[ws->input_cursor] = c;
                ws->input_cursor++;
            }
        }
    }
}

void world_select_update(WorldSelect *ws, float dt)
{
    ws->cursor_timer += dt;
    if (ws->cursor_timer >= 1.0f) ws->cursor_timer -= 1.0f;

    ws->hovered_recent = -1;
    ws->hovered_enter = 0;

    int mx, my;
    SDL_GetMouseState(&mx, &my);

    int input_w = 300;
    int btn_w = 120;
    int btn_h = 24;
    int btn_spacing = 4;
    int panel_w = input_w;
    int panel_x = (g_screen_w - panel_w) / 2;
    int panel_y = g_screen_h / 2 - 80;
    int btn_y = panel_y + 60;

    int enter_x = panel_x + (panel_w - btn_w) / 2;
    int enter_y = btn_y;
    ws->hovered_enter = point_in_rect(mx, my, enter_x, enter_y, btn_w, btn_h);

    if (ws->hovered_enter && ws->mouse_clicked) {
        if (ws->input_cursor > 0) {
            ws->submitted = 1;
        }
    }

    int recent_start_y = btn_y + btn_h + 20;
    int recent_btn_w = 120;
    int recent_btn_h = 28;
    for (int i = 0; i < ws->recent_count; i++) {
        int col = i % 4;
        int row = i / 4;
        int rx = panel_x + col * (recent_btn_w + 8);
        int ry = recent_start_y + row * (recent_btn_h + 4);
        if (point_in_rect(mx, my, rx, ry, recent_btn_w, recent_btn_h)) {
            ws->hovered_recent = i;
            if (ws->mouse_clicked) {
                strncpy(ws->input_text, ws->recent_names[i], WORLD_NAME_MAX);
                ws->input_cursor = (int)strlen(ws->recent_names[i]);
            }
        }
    }

    ws->mouse_clicked = 0;
}

void world_select_render(WorldSelect *ws, Renderer *r)
{
    renderer_begin_ui(r);

    renderer_draw_rect(r, 0, 0, g_screen_w, g_screen_h, 0.05f, 0.05f, 0.15f, 1.0f);

    int input_w = 300;
    int panel_w = input_w;
    int panel_x = (g_screen_w - panel_w) / 2;
    int panel_y = g_screen_h / 2 - 80;

    int title_w = renderer_text_width(r, "SEARCH WORLD", 3.0f);
    renderer_draw_text(r, "SEARCH WORLD",
        panel_x + (panel_w - title_w) / 2, panel_y - 60, 3.0f,
        1.0f, 1.0f, 1.0f);

    renderer_draw_rect(r, panel_x, panel_y, input_w, 40, 0.2f, 0.2f, 0.3f, 1.0f);
    renderer_draw_rect(r, panel_x, panel_y, input_w, 1, 0.5f, 0.5f, 0.6f, 1.0f);
    renderer_draw_rect(r, panel_x, panel_y + 39, input_w, 1, 0.0f, 0.0f, 0.0f, 1.0f);
    renderer_draw_rect(r, panel_x, panel_y, 1, 40, 0.5f, 0.5f, 0.6f, 1.0f);
    renderer_draw_rect(r, panel_x + input_w - 1, panel_y, 1, 40, 0.0f, 0.0f, 0.0f, 1.0f);

    renderer_draw_text(r, ws->input_text, panel_x + 8, panel_y + 10, 2.0f,
        1.0f, 1.0f, 1.0f);

    if (ws->cursor_timer < 0.5f) {
        int text_w = renderer_text_width(r, ws->input_text, 2.0f);
        renderer_draw_rect(r, panel_x + 8 + text_w, panel_y + 8, 2, 24,
            1.0f, 1.0f, 1.0f, 1.0f);
    }

    int btn_w = 120;
    int btn_h = 24;
    int btn_y = panel_y + 60;
    int enter_x = panel_x + (panel_w - btn_w) / 2;
    int enter_y = btn_y;

    float btn_r = 0.2f, btn_g = 0.5f, btn_b = 0.2f;
    if (ws->hovered_enter) { btn_r = 0.3f; btn_g = 0.7f; btn_b = 0.3f; }
    renderer_draw_rect(r, enter_x, enter_y, btn_w, btn_h, btn_r, btn_g, btn_b, 1.0f);
    renderer_draw_rect(r, enter_x, enter_y, btn_w, 1, btn_r + 0.2f, btn_g + 0.2f, btn_b + 0.2f, 1.0f);
    renderer_draw_rect(r, enter_x, enter_y + btn_h - 1, btn_w, 1, 0.0f, 0.0f, 0.0f, 1.0f);
    renderer_draw_rect(r, enter_x, enter_y, 1, btn_h, btn_r + 0.2f, btn_g + 0.2f, btn_b + 0.2f, 1.0f);
    renderer_draw_rect(r, enter_x + btn_w - 1, enter_y, 1, btn_h, 0.0f, 0.0f, 0.0f, 1.0f);

    int enter_text_w = renderer_text_width(r, "ENTER", 1.5f);
    renderer_draw_text(r, "ENTER",
        enter_x + (btn_w - enter_text_w) / 2, enter_y + 4, 1.5f,
        1.0f, 1.0f, 1.0f);

    if (ws->recent_count > 0) {
        int recent_label_w = renderer_text_width(r, "Recent:", 1.5f);
        int recent_start_y = btn_y + btn_h + 20;
        renderer_draw_text(r, "Recent:",
            panel_x, recent_start_y - 20, 1.5f, 0.7f, 0.7f, 0.7f);

        int recent_btn_w = 120;
        int recent_btn_h = 28;
        for (int i = 0; i < ws->recent_count; i++) {
            int col = i % 4;
            int row = i / 4;
            int rx = panel_x + col * (recent_btn_w + 8);
            int ry = recent_start_y + row * (recent_btn_h + 4);

            float rb_r = 0.2f, rb_g = 0.2f, rb_b = 0.3f;
            if (ws->hovered_recent == i) { rb_r = 0.3f; rb_g = 0.3f; rb_b = 0.5f; }
            renderer_draw_rect(r, rx, ry, recent_btn_w, recent_btn_h, rb_r, rb_g, rb_b, 1.0f);
            renderer_draw_rect(r, rx, ry, recent_btn_w, 1, rb_r + 0.2f, rb_g + 0.2f, rb_b + 0.2f, 1.0f);
            renderer_draw_rect(r, rx, ry + recent_btn_h - 1, recent_btn_w, 1, 0.0f, 0.0f, 0.0f, 1.0f);
            renderer_draw_rect(r, rx, ry, 1, recent_btn_h, rb_r + 0.2f, rb_g + 0.2f, rb_b + 0.2f, 1.0f);
            renderer_draw_rect(r, rx + recent_btn_w - 1, ry, 1, recent_btn_h, 0.0f, 0.0f, 0.0f, 1.0f);

            int tw = renderer_text_width(r, ws->recent_names[i], 1.2f);
            renderer_draw_text(r, ws->recent_names[i],
                rx + (recent_btn_w - tw) / 2, ry + 8, 1.2f,
                1.0f, 1.0f, 1.0f);
        }
    }

    int hint_w = renderer_text_width(r, "ESC: Quit", 1.0f);
    renderer_draw_text(r, "ESC: Quit",
        (g_screen_w - hint_w) / 2, g_screen_h - 24, 1.0f,
        0.5f, 0.5f, 0.5f);

    renderer_end_ui(r);
}

void world_select_load_recent(WorldSelect *ws)
{
    memset(ws->recent_names, 0, sizeof(ws->recent_names));
    ws->recent_count = 0;

    FILE *f = fopen("res/worlds/recent.txt", "r");
    if (!f) return;

    char line[WORLD_NAME_MAX + 2];
    while (ws->recent_count < RECENT_WORLDS_MAX && fgets(line, sizeof(line), f)) {
        int len = (int)strlen(line);
        while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r'))
            len--;
        line[len] = '\0';
        if (len > 0) {
            strncpy(ws->recent_names[ws->recent_count], line, WORLD_NAME_MAX);
            ws->recent_names[ws->recent_count][WORLD_NAME_MAX] = '\0';
            ws->recent_count++;
        }
    }
    fclose(f);
}

void world_select_add_recent(WorldSelect *ws, const char *name)
{
    int existing = -1;
    for (int i = 0; i < ws->recent_count; i++) {
        if (strcmp(ws->recent_names[i], name) == 0) {
            existing = i;
            break;
        }
    }

    if (existing >= 0) {
        char saved[WORLD_NAME_MAX + 1];
        strncpy(saved, ws->recent_names[existing], WORLD_NAME_MAX);
        saved[WORLD_NAME_MAX] = '\0';
        for (int i = existing; i > 0; i--) {
            strncpy(ws->recent_names[i], ws->recent_names[i - 1], WORLD_NAME_MAX);
        }
        strncpy(ws->recent_names[0], saved, WORLD_NAME_MAX);
    } else {
        if (ws->recent_count >= RECENT_WORLDS_MAX) {
            ws->recent_count = RECENT_WORLDS_MAX - 1;
        }
        for (int i = ws->recent_count; i > 0; i--) {
            strncpy(ws->recent_names[i], ws->recent_names[i - 1], WORLD_NAME_MAX);
        }
        strncpy(ws->recent_names[0], name, WORLD_NAME_MAX);
        ws->recent_count++;
    }

    FILE *f = fopen("res/worlds/recent.txt", "w");
    if (!f) return;
    for (int i = 0; i < ws->recent_count; i++) {
        fprintf(f, "%s\n", ws->recent_names[i]);
    }
    fclose(f);
}
```

- [ ] **Step 3: Build and verify no warnings**

Run: `make clean && make`
Expected: Compiles with zero warnings.

- [ ] **Step 4: Commit**

```bash
git add src/engine/world_select.h src/engine/world_select.c
git commit -m "Add world_select module: search screen, text input, recent worlds"
```

---

### Task 4: Restructure `main.c` with GameState machine and dynamic world loading

**Files:**
- Modify: `src/main.c`

This is the largest task. It restructures the main loop to support menu vs gameplay states, removes hardcoded paths, and wires everything together.

- [ ] **Step 1: Add includes and GameState enum to `src/main.c`**

Add after the existing includes (after `#include "game/interact.h"`):

```c
#include "engine/world_select.h"
```

Replace the `#define` block for paths:

```c
#define FPS_CAP 60
#define FRAME_TIME (1000.0 / FPS_CAP)
#define AUTOSAVE_INTERVAL 60.0f
#define ITEM_WRENCH 9000
#define ITEM_PICKAXE 9001
#define PROFILE_PATH "res/worlds/player.dat"
```

Add after the `#define` block:

```c
typedef enum {
    GAME_STATE_MENU,
    GAME_STATE_PLAYING
} GameState;

static int g_running = 1;
static GameState g_game_state = GAME_STATE_MENU;
static WorldSelect g_world_select;
```

- [ ] **Step 2: Update Game struct**

Replace the existing `Game` struct with:

```c
typedef struct {
    Renderer renderer;
    Camera camera;
    Input input;
    UI ui;
    World world;
    Player player;
    Inventory inventory;
    Store store;
    float autosave_timer;
    int break_progress;
    float sign_overlay_timer;
    int sign_overlay_x, sign_overlay_y;
    int sign_overlay_active;
    int portal_link_pending;
    int portal_link_x, portal_link_y;
    uint64_t last_time;
    char current_world_name[64];
    int exit_confirm_active;
} Game;
```

- [ ] **Step 3: Replace `game_save_all` and `game_load_all`**

Replace the existing `game_save_all` function with:

```c
static void game_save_all(Game *g) {
    g->world.spawn_x = g->player.x;
    g->world.spawn_y = g->player.y;
    char path[256];
    world_build_path(path, sizeof(path), g->current_world_name, "wld");
    world_save(&g->world, path);
    inventory_save_profile(&g->inventory, g->player.gems, g->player.health, PROFILE_PATH);
    printf("Game saved.\n");
}
```

Remove the existing `game_load_all` function entirely (it will be replaced by `game_enter_world`).

- [ ] **Step 4: Add `ensure_worlds_dir` helper**

Add `#include <sys/stat.h>` at the top of `src/main.c` with the other includes. Then add before `game_init`:

```c
static void ensure_worlds_dir(void) {
    mkdir("res", 0755);
    mkdir("res/worlds", 0755);
}
```

- [ ] **Step 5: Add `game_enter_world` function**

Add after `ensure_worlds_dir`:

```c
static void game_enter_world(Game *g, const char *name) {
    if (g->world.tiles) {
        game_save_all(g);
        world_free(&g->world);
    }

    strncpy(g->current_world_name, name, sizeof(g->current_world_name) - 1);
    g->current_world_name[sizeof(g->current_world_name) - 1] = '\0';

    char wld_path[256];
    world_build_path(wld_path, sizeof(wld_path), name, "wld");

    if (world_load(&g->world, wld_path) != 0) {
        if (world_init(&g->world, WORLD_WIDTH, WORLD_HEIGHT) != 0) {
            fprintf(stderr, "Failed to create world\n");
            return;
        }
        world_generate(&g->world);
        world_set_name(&g->world, name);
        world_save(&g->world, wld_path);
        printf("New world '%s' generated.\n", name);
    }

    player_init(&g->player, g->world.spawn_x, g->world.spawn_y);

    int profile_loaded = 0;
    if (inventory_load_profile(&g->inventory, &g->player.gems, &g->player.health, PROFILE_PATH) == 0) {
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

    camera_init(&g->camera, g->world.width * TILE_SIZE, g->world.height * TILE_SIZE);
    camera_set_target(&g->camera, g->player.x, g->player.y);
    g->camera.x = g->camera.target_x;
    g->camera.y = g->camera.target_y;
    renderer_generate_atlas(&g->renderer);

    ui_close_all(&g->ui);
    g->exit_confirm_active = 0;
    g->sign_overlay_active = 0;
    g->sign_overlay_timer = 0;
    g->portal_link_pending = 0;
    g->autosave_timer = AUTOSAVE_INTERVAL;

    world_select_add_recent(&g_world_select, name);

    g_game_state = GAME_STATE_PLAYING;
}
```

- [ ] **Step 6: Update `game_init`**

Replace the existing `game_init` function with:

```c
static void game_init(Game *g) {
    memset(g, 0, sizeof(Game));

    if (renderer_init(&g->renderer) != 0) {
        fprintf(stderr, "Failed to init renderer\n");
        exit(1);
    }

    SDL_StartTextInput();

    input_init(&g->input);
    ui_init(&g->ui);
    store_init(&g->store);

    g->autosave_timer = AUTOSAVE_INTERVAL;
    g->last_time = SDL_GetPerformanceCounter();

    ensure_worlds_dir();
    world_select_init(&g_world_select);
}
```

- [ ] **Step 7: Update `game_handle_events`**

Replace the existing `game_handle_events` function with:

```c
static void game_handle_events(Game *g) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) {
            g_running = 0;
            return;
        }

        if (g_game_state == GAME_STATE_MENU) {
            if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) {
                g_running = 0;
                return;
            }
            world_select_handle_event(&g_world_select, &e);
            continue;
        }

        if (g_game_state == GAME_STATE_PLAYING) {
            if (g->exit_confirm_active) {
                if (e.type == SDL_KEYDOWN) {
                    if (e.key.keysym.sym == SDLK_y) {
                        g->exit_confirm_active = 0;
                        game_save_all(g);
                        g_game_state = GAME_STATE_MENU;
                        world_select_init(&g_world_select);
                        continue;
                    }
                    if (e.key.keysym.sym == SDLK_n || e.key.keysym.sym == SDLK_ESCAPE) {
                        g->exit_confirm_active = 0;
                    }
                }
                input_handle_event(&g->input, &e);
                continue;
            }

            if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) {
                if (g->ui.state != UI_STATE_NONE) {
                    ui_close_all(&g->ui);
                } else {
                    g->exit_confirm_active = 1;
                }
                input_handle_event(&g->input, &e);
                continue;
            }
            if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_e) {
                ui_toggle_inventory(&g->ui);
            }
            if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_b) {
                ui_toggle_store(&g->ui);
            }
            input_handle_event(&g->input, &e);
            if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_F11) {
                renderer_toggle_fullscreen(&g->renderer);
            }
            if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_RETURN) {
                if (g->ui.state == UI_STATE_SIGN_EDIT) {
                    ui_finish_sign_edit(&g->ui, &g->world);
                }
            }
            if (e.type == SDL_TEXTINPUT && g->ui.state == UI_STATE_SIGN_EDIT) {
                if (g->ui.sign_edit_cursor < SIGN_TEXT_MAX_LEN) {
                    int len = (int)strlen(e.text.text);
                    if (len > 0 && g->ui.sign_edit_cursor + len <= SIGN_TEXT_MAX_LEN) {
                        g->ui.sign_edit_text[g->ui.sign_edit_cursor] = e.text.text[0];
                        g->ui.sign_edit_cursor++;
                    }
                }
            }
            if (e.type == SDL_KEYDOWN && g->ui.state == UI_STATE_SIGN_EDIT) {
                if (e.key.keysym.sym == SDLK_BACKSPACE && g->ui.sign_edit_cursor > 0) {
                    g->ui.sign_edit_cursor--;
                    g->ui.sign_edit_text[g->ui.sign_edit_cursor] = '\0';
                }
            }
        }
    }
}
```

- [ ] **Step 8: Update `game_update`**

Add a guard at the top of `game_update` to skip gameplay when in menu:

```c
static void game_update(Game *g, float dt) {
    if (g_game_state == GAME_STATE_MENU) {
        world_select_update(&g_world_select, dt);
        if (g_world_select.submitted) {
            g_world_select.submitted = 0;
            game_enter_world(g, g_world_select.input_text);
        }
        input_update(&g->input);
        return;
    }

    if (g->exit_confirm_active) {
        input_update(&g->input);
        return;
    }

    if (g->ui.state != UI_STATE_NONE) {
```

The rest of `game_update` stays exactly the same from `ui_update(&g->ui, ...)` onward.

- [ ] **Step 9: Update `game_render`**

Add menu rendering at the top of `game_render`:

```c
static void game_render(Game *g) {
    if (g_game_state == GAME_STATE_MENU) {
        renderer_clear(&g->renderer, 0.05f, 0.05f, 0.15f);
        world_select_render(&g_world_select, &g->renderer);
        renderer_present(&g->renderer);
        return;
    }

    renderer_clear(&g->renderer, 0.4f, 0.7f, 1.0f);
```

The rest of the tile and UI rendering stays the same. Just add the exit confirmation dialog rendering before the help text at the bottom of the UI section. Find this line:

```c
    renderer_draw_text(&g->renderer, "E: Inv  B: Store  LMB: Break  RMB: Place  F5: Save  F11: Fullscreen  ESC: Quit", 8, g_screen_h - 16, 1.0f, 1.0f, 1.0f, 1.0f);
```

And change it to:

```c
    if (g->exit_confirm_active) {
        int dw = 260;
        int dh = 60;
        int dx = (g_screen_w - dw) / 2;
        int dy = (g_screen_h - dh) / 2;
        renderer_draw_rect(&g->renderer, dx, dy, dw, dh, 0.0f, 0.0f, 0.0f, 0.9f);
        renderer_draw_rect(&g->renderer, dx, dy, dw, 1, 0.4f, 0.4f, 0.4f, 1.0f);
        renderer_draw_rect(&g->renderer, dx, dy + dh - 1, dw, 1, 0.0f, 0.0f, 0.0f, 1.0f);
        renderer_draw_rect(&g->renderer, dx, dy, 1, dh, 0.4f, 0.4f, 0.4f, 1.0f);
        renderer_draw_rect(&g->renderer, dx + dw - 1, dy, 1, dh, 0.0f, 0.0f, 0.0f, 1.0f);
        int qtw = renderer_text_width(&g->renderer, "EXIT WORLD?", 2.0f);
        renderer_draw_text(&g->renderer, "EXIT WORLD?", dx + (dw - qtw) / 2, dy + 8, 2.0f, 1.0f, 1.0f, 1.0f);
        int htw = renderer_text_width(&g->renderer, "Y/N", 1.5f);
        renderer_draw_text(&g->renderer, "Y/N", dx + (dw - htw) / 2, dy + 34, 1.5f, 0.7f, 0.7f, 0.7f);
    }

    renderer_draw_text(&g->renderer, "E: Inv  B: Store  LMB: Break  RMB: Place  F5: Save  F11: Fullscreen  ESC: Menu", 8, g_screen_h - 16, 1.0f, 1.0f, 1.0f, 1.0f);
```

- [ ] **Step 10: Update `game_shutdown`**

Replace the existing `game_shutdown`:

```c
static void game_shutdown(Game *g) {
    if (g_game_state == GAME_STATE_PLAYING) {
        game_save_all(g);
        world_free(&g->world);
    }
    renderer_shutdown(&g->renderer);
}
```

- [ ] **Step 11: Build and verify no warnings**

Run: `make clean && make`
Expected: Compiles with zero warnings. Fix any warnings before proceeding.

- [ ] **Step 12: Test manually**

Run: `make run`

Expected behavior:
1. Game starts with "SEARCH WORLD" screen on dark blue background
2. Type a world name (e.g. "test") - appears as lowercase
3. Press ENTER or click ENTER button - new world generates and loads
4. Walk around, place/break blocks
5. Press F5 to save
6. Press ESC - "EXIT WORLD? Y/N" dialog appears
7. Press Y - returns to search screen
8. Type same name "test" - loads existing world at saved position
9. Press ESC from search screen - quits

- [ ] **Step 13: Commit**

```bash
git add src/main.c
git commit -m "Restructure main loop with GameState machine, dynamic world loading, shared player profile"
```

---

### Task 5: Final verification and cleanup

**Files:**
- All modified files

- [ ] **Step 1: Clean build**

Run: `make clean && make`
Expected: Zero warnings, zero errors.

- [ ] **Step 2: Full manual test**

Run: `make run`

Test all scenarios:
1. Start game -> search screen appears
2. Create a new world "myworld" -> generates and loads
3. Walk around, modify blocks
4. F5 save -> "Game saved." in console
5. ESC -> confirmation dialog
6. Y -> returns to search screen
7. Type "myworld" again -> loads existing world with player at saved position
8. ESC from search screen -> quits
9. Restart game -> search screen, "myworld" appears in recent list
10. Click "myworld" in recent -> fills input, press ENTER -> loads
11. Create second world "test2" -> generates
12. ESC -> Y -> search screen
13. Enter "myworld" -> inventory/gems should be same as before (shared profile)
14. ESC from search -> quits

- [ ] **Step 3: Commit final state if any fixes were needed**

```bash
git add -A
git commit -m "World search feature: final fixes"
```
