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

## Architecture

### Camera (`camera.h/c`)

Add to Camera struct:
- `float zoom` - current zoom level (default 1.0)
- `float zoom_target` - target zoom for smooth interpolation

New/changed functions:
- `camera_update`: lerp `zoom` toward `zoom_target` (same speed as position lerp), clamp camera position using `half_w = g_screen_w / (2.0f * zoom)` and `half_h = g_screen_h / (2.0f * zoom)`
- `camera_world_to_screen`: multiply screen coords by zoom: `*sx = (int)((wx - cam->x) * cam->zoom + g_screen_w / 2.0f)`
- `camera_screen_to_world`: divide by zoom: `*wx = (int)((float)sx / cam->zoom + cam->x - g_screen_w / (2.0f * cam->zoom))`
- `camera_set_zoom(Camera *cam, float zoom)`: sets `zoom_target`, clamped to [0.5, 2.0]

### Input (`input.h/c`)

Add to Input struct:
- `int mouse_scroll_y` - scroll wheel delta (+1 = scroll up, -1 = scroll down)

Changes:
- `input_handle_event`: handle `SDL_MOUSEWHEEL` event, set `mouse_scroll_y += e->wheel.y`
- `input_update`: reset `mouse_scroll_y = 0` at frame start

### Renderer (`renderer.c`)

Changes:
- `renderer_begin_tile_batch`: accept `float zoom` parameter, set ortho to `glOrtho(0, g_screen_w / zoom, g_screen_h / zoom, 0, -1, 1)`, viewport remains full screen
- `renderer_begin_ui`: unchanged (1:1 screen coords)

### Main (`main.c`)

Changes:
- In `game_update`: check `g->input.mouse_scroll_y`, call `camera_set_zoom(&g->camera, g->camera.zoom_target + g->input.mouse_scroll_y * 0.1f)`
- In `game_render`: pass `g->camera.zoom` to `renderer_begin_tile_batch`
- Update help text to mention scroll zoom

## Data Flow

1. SDL_MOUSEWHEEL event sets `input.mouse_scroll_y`
2. `game_update` reads scroll delta, adjusts `camera.zoom_target` by +/- 0.1, clamped [0.5, 2.0]
3. `camera_update` lerps `zoom` toward `zoom_target`, re-clamps camera position
4. `renderer_begin_tile_batch` sets ortho projection scaled by 1/zoom
5. Tile rendering uses camera's `world_to_screen` (which applies zoom) for vertex positions
6. UI rendering uses separate 1:1 ortho projection, unaffected by zoom

## Edge Cases

- Zoom clamping prevents values outside [0.5, 2.0]
- Camera position clamping uses zoom-adjusted visible area to prevent showing beyond world edges
- Scroll wheel events only processed during `GAME_STATE_PLAYING` with no UI open
