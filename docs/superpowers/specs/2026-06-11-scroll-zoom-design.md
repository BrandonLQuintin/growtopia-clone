# Scroll Wheel Zoom

## Summary

Add mouse scroll wheel zoom (0.5x - 2.0x) to the gameplay camera. Scrolling up zooms in, scrolling down zooms out. Zoom uses ortho projection scaling so the world layer zooms while UI stays at 1:1.

## Requirements

- Mouse scroll wheel zooms the game world view in/out
- Zoom range: 0.5x (zoomed out, see more) to 2.0x (zoomed in, see less detail)
- Smooth zoom interpolation (lerp toward target)
- Zoom step: 0.1 per scroll tick
- UI (hotbar, inventory, store, HUD) is NOT affected by zoom
- Camera clamping respects zoom level (visible area = screen / zoom)
- Mouse-to-world coordinate conversion works correctly at all zoom levels
- Block interaction (break/place) works correctly at all zoom levels
- Zoom resets to 1.0 when entering a new world
- Tile visibility range adjusts with zoom (no black gaps when zoomed out)
- World-space overlays (sign labels) position correctly in UI batch

## Architecture

### Camera (`camera.h/c`)

Add to Camera struct:
- `float zoom` - current zoom level (default 1.0)
- `float zoom_target` - target zoom for smooth interpolation

New/changed functions:
- `camera_init`: set `zoom = 1.0` and `zoom_target = 1.0`
- `camera_update`: lerp `zoom` toward `zoom_target` (same speed as position lerp), clamp camera position using `half_w = g_screen_w / (2.0f * zoom)` and `half_h = g_screen_h / (2.0f * zoom)`
- `camera_world_to_screen`: `*sx = (int)(wx - cam->x + g_screen_w / (2.0f * cam->zoom))`, `*sy = (int)(wy - cam->y + g_screen_h / (2.0f * cam->zoom))` (no zoom multiply — ortho projection handles scaling)
- `camera_screen_to_world`: `*wx = (int)((float)sx * cam->zoom + cam->x - g_screen_w / 2.0f)`, `*wy = (int)((float)sy * cam->zoom + cam->y - g_screen_h / 2.0f)` (multiply screen by zoom to get world offset)
- `camera_set_zoom(Camera *cam, float zoom)`: sets `zoom_target`, clamped to [0.5, 2.0]

### Input (`input.h/c`)

Add to Input struct:
- `int mouse_scroll_y` - scroll wheel delta (+1 = scroll up, -1 = scroll down)

Changes:
- `input_handle_event`: handle `SDL_MOUSEWHEEL` event, set `mouse_scroll_y += e->wheel.y`
- `input_update`: reset `mouse_scroll_y = 0` at frame start

### Renderer (`renderer.h/c`)

Changes:
- `renderer_begin_tile_batch`: change signature to `void renderer_begin_tile_batch(Renderer *r, float zoom)`, set ortho to `glOrtho(0, g_screen_w / zoom, g_screen_h / zoom, 0, -1, 1)`, viewport remains full screen
- `renderer_begin_ui`: unchanged (1:1 screen coords)

### Main (`main.c`)

Changes:
- In `game_update` (after UI early returns, in the `UI_STATE_NONE` gameplay section): check `g->input.mouse_scroll_y`, call `camera_set_zoom(&g->camera, g->camera.zoom_target + g->input.mouse_scroll_y * 0.1f)`
- In `game_render`: update tile visibility range to use zoom-adjusted visible area: `visible_w = g_screen_w / g->camera.zoom`, `visible_h = g_screen_h / g->camera.zoom`, use these for `cam_left`, `cam_top`, `end_x`, `end_y`
- In `game_render`: pass `g->camera.zoom` to `renderer_begin_tile_batch`
- In `game_render`: sign overlay label position — convert world-to-screen coords to pixel coords for UI batch by multiplying by zoom: `pixel_x = sox * g->camera.zoom`, `pixel_y = soy * g->camera.zoom`
- Update help text to mention scroll zoom

## Data Flow

1. SDL_MOUSEWHEEL event sets `input.mouse_scroll_y`
2. `game_update` reads scroll delta, adjusts `camera.zoom_target` by +/- 0.1, clamped [0.5, 2.0]
3. `camera_update` lerps `zoom` toward `zoom_target`, re-clamps camera position
4. `renderer_begin_tile_batch` sets ortho projection scaled by 1/zoom (visible area = screen/zoom in world units)
5. `camera_world_to_screen` outputs coordinates in ortho space (no zoom multiply)
6. Tile rendering draws in ortho space, GL maps to screen pixels (zoomed)
7. UI rendering uses separate 1:1 ortho projection, unaffected by zoom
8. World-to-screen coords used in UI batch are converted to pixels by multiplying by zoom

## Edge Cases

- Zoom clamping prevents values outside [0.5, 2.0]
- Camera position clamping uses zoom-adjusted visible area to prevent showing beyond world edges
- Scroll wheel events only processed during `GAME_STATE_PLAYING` with `UI_STATE_NONE`
- Tile visibility range uses `screen / zoom` so zoomed-out view renders enough tiles
- Sign overlay and block tooltip positions multiply ortho coords by zoom when used in UI batch
- Zoom resets to 1.0 on world entry via `camera_init`
