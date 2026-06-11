# Scroll Wheel Zoom Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add mouse scroll wheel zoom (0.5x - 2.0x) to the gameplay camera using ortho projection scaling.

**Architecture:** The camera gets a `zoom` field that scales the ortho projection in the tile batch. The ortho maps `[0, screen/zoom]` to the full viewport, so world coords render zoomed. UI batch stays at 1:1. Camera coordinate transforms account for the zoom-adjusted visible area.

**Tech Stack:** C11, SDL2, OpenGL 2.1

---

### Task 1: Add scroll wheel input tracking

**Files:**
- Modify: `src/engine/input.h:9-15` (Input struct)
- Modify: `src/engine/input.c:9-14` (input_update)
- Modify: `src/engine/input.c:17-50` (input_handle_event)

- [ ] **Step 1: Add `mouse_scroll_y` field to Input struct**

In `src/engine/input.h`, add `int mouse_scroll_y;` to the Input struct after `mouse_rel_y`:

```c
    int mouse_x, mouse_y;
    int mouse_rel_x, mouse_rel_y;
    int mouse_scroll_y;
} Input;
```

- [ ] **Step 2: Reset `mouse_scroll_y` in `input_update`**

In `src/engine/input.c`, add at the end of `input_update` (after line 14, before the closing brace):

```c
    input->mouse_scroll_y = 0;
```

- [ ] **Step 3: Handle `SDL_MOUSEWHEEL` in `input_handle_event`**

In `src/engine/input.c`, add a new case in the switch statement after the `SDL_MOUSEMOTION` case (after line 48):

```c
    case SDL_MOUSEWHEEL:
        input->mouse_scroll_y += e->wheel.y;
        break;
```

- [ ] **Step 4: Build and verify**

Run: `make clean && make`
Expected: Compiles with zero warnings.

- [ ] **Step 5: Commit**

```bash
git add src/engine/input.h src/engine/input.c
git commit -m "Add mouse scroll wheel tracking to input system"
```

---

### Task 2: Add zoom to camera

**Files:**
- Modify: `src/engine/camera.h:4-9` (Camera struct)
- Modify: `src/engine/camera.h:11-16` (function declarations)
- Modify: `src/engine/camera.c` (all functions)

- [ ] **Step 1: Add zoom fields to Camera struct**

In `src/engine/camera.h`, add `zoom` and `zoom_target` to the struct, and add `camera_set_zoom` declaration:

```c
typedef struct {
    float x, y;
    float target_x, target_y;
    int world_w, world_h;
    float lerp_speed;
    float zoom;
    float zoom_target;
} Camera;

void camera_init(Camera *cam, int world_w, int world_h);
void camera_update(Camera *cam, float dt);
void camera_set_target(Camera *cam, float x, float y);
void camera_world_to_screen(Camera *cam, float wx, float wy, int *sx, int *sy);
void camera_screen_to_world(Camera *cam, int sx, int sy, int *wx, int *wy);
void camera_set_zoom(Camera *cam, float zoom);
```

- [ ] **Step 2: Initialize zoom in `camera_init`**

In `src/engine/camera.c`, add at the end of `camera_init` (after `cam->lerp_speed = 5.0f;`):

```c
    cam->zoom = 1.0f;
    cam->zoom_target = 1.0f;
```

- [ ] **Step 3: Update `camera_update` for zoom lerp and zoom-adjusted clamping**

Replace the entire `camera_update` function body:

```c
void camera_update(Camera *cam, float dt) {
    cam->x += (cam->target_x - cam->x) * cam->lerp_speed * dt;
    cam->y += (cam->target_y - cam->y) * cam->lerp_speed * dt;
    cam->zoom += (cam->zoom_target - cam->zoom) * cam->lerp_speed * dt;

    if (cam->zoom < 0.5f) cam->zoom = 0.5f;
    if (cam->zoom > 2.0f) cam->zoom = 2.0f;

    float half_w = g_screen_w / (2.0f * cam->zoom);
    float half_h = g_screen_h / (2.0f * cam->zoom);

    if (cam->x - half_w < 0.0f)
        cam->x = half_w;
    if (cam->x + half_w > (float)cam->world_w)
        cam->x = (float)cam->world_w - half_w;
    if (cam->y - half_h < 0.0f)
        cam->y = half_h;
    if (cam->y + half_h > (float)cam->world_h)
        cam->y = (float)cam->world_h - half_h;
}
```

- [ ] **Step 4: Update `camera_world_to_screen` for ortho-scaled coords**

Replace the entire `camera_world_to_screen` function body:

```c
void camera_world_to_screen(Camera *cam, float wx, float wy, int *sx, int *sy) {
    *sx = (int)(wx - cam->x + g_screen_w / (2.0f * cam->zoom));
    *sy = (int)(wy - cam->y + g_screen_h / (2.0f * cam->zoom));
}
```

- [ ] **Step 5: Update `camera_screen_to_world` for ortho-scaled coords**

Replace the entire `camera_screen_to_world` function body:

```c
void camera_screen_to_world(Camera *cam, int sx, int sy, int *wx, int *wy) {
    *wx = (int)((float)sx * cam->zoom + cam->x - g_screen_w / 2.0f);
    *wy = (int)((float)sy * cam->zoom + cam->y - g_screen_h / 2.0f);
}
```

- [ ] **Step 6: Add `camera_set_zoom` function**

Add at the end of `src/engine/camera.c`:

```c
void camera_set_zoom(Camera *cam, float zoom) {
    if (zoom < 0.5f) zoom = 0.5f;
    if (zoom > 2.0f) zoom = 2.0f;
    cam->zoom_target = zoom;
}
```

- [ ] **Step 7: Build and verify**

Run: `make clean && make`
Expected: Compiles with zero warnings.

- [ ] **Step 8: Commit**

```bash
git add src/engine/camera.h src/engine/camera.c
git commit -m "Add zoom support to camera with lerp and clamping"
```

---

### Task 3: Update renderer for zoom-scaled tile batch

**Files:**
- Modify: `src/engine/renderer.h:33` (function signature)
- Modify: `src/engine/renderer.c:922-930` (renderer_begin_tile_batch)

- [ ] **Step 1: Update `renderer_begin_tile_batch` signature in header**

In `src/engine/renderer.h`, change line 33 from:

```c
void renderer_begin_tile_batch(Renderer *r);
```

to:

```c
void renderer_begin_tile_batch(Renderer *r, float zoom);
```

- [ ] **Step 2: Update `renderer_begin_tile_batch` implementation**

In `src/engine/renderer.c`, replace the `renderer_begin_tile_batch` function (lines 922-930) with:

```c
void renderer_begin_tile_batch(Renderer *r, float zoom) {
    renderer_get_size(r, &g_screen_w, &g_screen_h);
    float view_w = (float)g_screen_w / zoom;
    float view_h = (float)g_screen_h / zoom;
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, view_w, view_h, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glViewport(0, 0, g_screen_w, g_screen_h);
}
```

- [ ] **Step 3: Build and verify**

Run: `make clean && make`
Expected: Compile error in `main.c` — `renderer_begin_tile_batch` now requires a `zoom` argument. This is expected; we fix it in Task 4.

- [ ] **Step 4: Commit**

```bash
git add src/engine/renderer.h src/engine/renderer.c
git commit -m "Update renderer_begin_tile_batch to accept zoom parameter"
```

---

### Task 4: Wire zoom into main game loop

**Files:**
- Modify: `src/main.c:558-562` (scroll handling in game_update)
- Modify: `src/main.c:593-605` (tile visibility range in game_render)
- Modify: `src/main.c:693-703` (sign overlay positioning in UI batch)
- Modify: `src/main.c:730` (help text)

- [ ] **Step 1: Add scroll zoom handling in `game_update`**

In `src/main.c`, add zoom handling after the hotbar key loop (after line 558, before the F5 save check). Insert after the closing brace of the `for` loop:

```c
    if (g->input.mouse_scroll_y != 0) {
        camera_set_zoom(&g->camera, g->camera.zoom_target + g->input.mouse_scroll_y * 0.1f);
    }
```

This goes between line 558 and line 560 (before the `if (input_is_key_pressed(...SDL_SCANCODE_F5))` line).

- [ ] **Step 2: Update tile visibility range in `game_render`**

In `src/main.c`, replace lines 593-598:

```c
    float cam_left = g->camera.x - g_screen_w / 2.0f;
    float cam_top = g->camera.y - g_screen_h / 2.0f;
    int start_x = (int)(cam_left / TILE_SIZE) - 1;
    int start_y = (int)(cam_top / TILE_SIZE) - 1;
    int end_x = start_x + (g_screen_w / TILE_SIZE) + 3;
    int end_y = start_y + (g_screen_h / TILE_SIZE) + 3;
```

with:

```c
    float visible_w = g_screen_w / g->camera.zoom;
    float visible_h = g_screen_h / g->camera.zoom;
    float cam_left = g->camera.x - visible_w / 2.0f;
    float cam_top = g->camera.y - visible_h / 2.0f;
    int start_x = (int)(cam_left / TILE_SIZE) - 1;
    int start_y = (int)(cam_top / TILE_SIZE) - 1;
    int end_x = start_x + (int)(visible_w / TILE_SIZE) + 3;
    int end_y = start_y + (int)(visible_h / TILE_SIZE) + 3;
```

- [ ] **Step 3: Pass zoom to `renderer_begin_tile_batch`**

In `src/main.c`, change line 605 from:

```c
    renderer_begin_tile_batch(&g->renderer);
```

to:

```c
    renderer_begin_tile_batch(&g->renderer, g->camera.zoom);
```

- [ ] **Step 4: Fix sign overlay positioning in UI batch**

In `src/main.c`, the sign overlay block (lines 696-703) uses `camera_world_to_screen` in the UI batch. The returned coords are in ortho space, but UI batch uses pixel space. Replace lines 697-701:

```c
                int sox, soy;
                camera_world_to_screen(&g->camera, g->sign_overlay_x * TILE_SIZE, g->sign_overlay_y * TILE_SIZE, &sox, &soy);
                int tw = renderer_text_width(&g->renderer, txt, 1.5f);
                int th = 16;
                int label_x = sox + TILE_SIZE / 2 - tw / 2;
                int label_y = soy - th - 8;
```

with:

```c
                int sox, soy;
                camera_world_to_screen(&g->camera, g->sign_overlay_x * TILE_SIZE, g->sign_overlay_y * TILE_SIZE, &sox, &soy);
                int px = (int)(sox * g->camera.zoom);
                int py = (int)(soy * g->camera.zoom);
                int tw = renderer_text_width(&g->renderer, txt, 1.5f);
                int th = 16;
                int label_x = px + (int)(TILE_SIZE * g->camera.zoom) / 2 - tw / 2;
                int label_y = py - th - 8;
```

- [ ] **Step 5: Update help text**

In `src/main.c`, replace line 730:

```c
    renderer_draw_text(&g->renderer, "E: Inv  B: Store  LMB: Break  RMB: Place  F5: Save  F11: Fullscreen  ESC: Menu", 8, g_screen_h - 16, 1.0f, 1.0f, 1.0f, 1.0f);
```

with:

```c
    renderer_draw_text(&g->renderer, "E: Inv  B: Store  LMB: Break  RMB: Place  Scroll: Zoom  F5: Save  F11: FS  ESC: Menu", 8, g_screen_h - 16, 1.0f, 1.0f, 1.0f, 1.0f);
```

- [ ] **Step 6: Build and verify**

Run: `make clean && make`
Expected: Compiles with zero warnings.

- [ ] **Step 7: Test manually**

Run: `make run`
Expected:
- Game starts, enter a world
- Scroll up zooms in (tiles get bigger, see less area)
- Scroll down zooms out (tiles get smaller, see more area)
- Zoom transitions are smooth (lerp)
- UI (hotbar, HUD) stays same size regardless of zoom
- Block interaction (break/place) works at all zoom levels
- Camera stays within world bounds at all zoom levels
- Sign overlay labels position correctly over signs

- [ ] **Step 8: Commit**

```bash
git add src/main.c
git commit -m "Wire scroll wheel zoom into game loop with visibility and UI fixes"
```
