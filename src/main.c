#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <SDL2/SDL.h>

#include "engine/renderer.h"
#include "engine/camera.h"
#include "engine/input.h"
#include "engine/ui.h"
#include "world/world.h"
#include "world/block.h"
#include "world/items.h"
#include "game/player.h"
#include "game/inventory.h"
#include "game/farming.h"
#include "game/crafting.h"
#include "game/store.h"


#define FPS_CAP 60
#define FRAME_TIME (1000.0 / FPS_CAP)
#define AUTOSAVE_INTERVAL 60.0f
#define WORLD_PATH "res/worlds/main.wld"
#define INVENTORY_PATH "res/worlds/main.inv"
#define PLAYER_PATH "res/worlds/main.player"

static int g_running = 1;

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
    uint64_t last_time;
} Game;

static void game_save_all(Game *g) {
    world_save(&g->world, WORLD_PATH);
    inventory_save(&g->inventory, INVENTORY_PATH);
    FILE *f = fopen(PLAYER_PATH, "wb");
    if (f) {
        fwrite(&g->player.x, sizeof(float), 1, f);
        fwrite(&g->player.y, sizeof(float), 1, f);
        fwrite(&g->player.gems, sizeof(int), 1, f);
        fwrite(&g->player.health, sizeof(int), 1, f);
        fclose(f);
    }
    printf("Game saved.\n");
}

static int game_load_all(Game *g) {
    if (world_load(&g->world, WORLD_PATH) != 0) {
        return -1;
    }
    inventory_load(&g->inventory, INVENTORY_PATH);
    FILE *f = fopen(PLAYER_PATH, "rb");
    if (f) {
        fread(&g->player.x, sizeof(float), 1, f);
        fread(&g->player.y, sizeof(float), 1, f);
        fread(&g->player.gems, sizeof(int), 1, f);
        fread(&g->player.health, sizeof(int), 1, f);
        fclose(f);
    }
    printf("Game loaded.\n");
    return 0;
}

static void game_init(Game *g) {
    memset(g, 0, sizeof(Game));
    
    if (renderer_init(&g->renderer) != 0) {
        fprintf(stderr, "Failed to init renderer\n");
        exit(1);
    }
    
    input_init(&g->input);
    ui_init(&g->ui);
    store_init(&g->store);
    
    g->autosave_timer = AUTOSAVE_INTERVAL;
    g->last_time = SDL_GetPerformanceCounter();
    
    if (game_load_all(g) != 0) {
        world_init(&g->world, WORLD_WIDTH, WORLD_HEIGHT);
        world_generate(&g->world);
        player_init(&g->player, WORLD_WIDTH / 2 * TILE_SIZE, 10 * TILE_SIZE);
        inventory_init(&g->inventory);
        inventory_add(&g->inventory, BLOCK_DIRT, 50);
        inventory_add(&g->inventory, BLOCK_STONE, 30);
        inventory_add(&g->inventory, BLOCK_WOOD, 20);
        inventory_add(&g->inventory, SEED_DIRT, 10);
        inventory_add(&g->inventory, SEED_GRASS, 5);
        inventory_add(&g->inventory, SEED_WOOD, 5);
        g->player.gems = 100;
        printf("New world generated.\n");
    }
    
    camera_init(&g->camera, g->world.width * TILE_SIZE, g->world.height * TILE_SIZE);
    camera_set_target(&g->camera, g->player.x, g->player.y);
    g->camera.x = g->camera.target_x;
    g->camera.y = g->camera.target_y;
    renderer_generate_atlas(&g->renderer);
}

static void game_handle_events(Game *g) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) {
            g_running = 0;
        }
        if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) {
            if (g->ui.state != UI_STATE_NONE) {
                ui_close_all(&g->ui);
            } else {
                g_running = 0;
            }
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
    }
}

static void game_update(Game *g, float dt) {
    if (g->ui.state != UI_STATE_NONE) {
        ui_update(&g->ui, &g->input, &g->renderer);
        
        if (g->ui.state == UI_STATE_STORE && input_is_mouse_clicked(&g->input, 1)) {
            Store store;
            store_init(&store);
            StoreEntry *entries;
            int entry_count;
            store_get_items(&store, g->ui.store_category, &entries, &entry_count);
            
            int STORE_COLS_LOCAL = 5;
            int cell_w = 100;
            int cell_h = 80;
            int panel_w = STORE_COLS_LOCAL * 100 + 40;
            int items_x = (g_screen_w - panel_w) / 2 + 20;
            int tabs_y = 60 + 36;
            int items_y = tabs_y + 28 + 16;
            
            for (int i = 0; i < entry_count; i++) {
                int col = i % STORE_COLS_LOCAL;
                int row = i / STORE_COLS_LOCAL;
                int cx = items_x + col * cell_w;
                int cy = items_y + row * cell_h;
                
                if (g->input.mouse_x >= cx && g->input.mouse_x < cx + 48 &&
                    g->input.mouse_y >= cy && g->input.mouse_y < cy + 48) {
                    if (g->player.gems >= entries[i].price) {
                        g->player.gems -= entries[i].price;
                        inventory_add(&g->inventory, entries[i].item_id, 1);
                    }
                    break;
                }
            }
        }
        
        if (g->ui.state == UI_STATE_INVENTORY && g->ui.drag_from_slot >= 0 && g->ui.selected_slot >= 0 &&
            g->ui.drag_from_slot != g->ui.selected_slot) {
            inventory_swap_slots(&g->inventory, g->ui.drag_from_slot, g->ui.selected_slot);
            g->ui.drag_from_slot = -1;
        }
        
        input_update(&g->input);
        return;
    }
    
    int move_left = input_is_key_down(&g->input, SDL_SCANCODE_A) || input_is_key_down(&g->input, SDL_SCANCODE_LEFT);
    int move_right = input_is_key_down(&g->input, SDL_SCANCODE_D) || input_is_key_down(&g->input, SDL_SCANCODE_RIGHT);
    int jump = input_is_key_pressed(&g->input, SDL_SCANCODE_W) || input_is_key_pressed(&g->input, SDL_SCANCODE_UP) || input_is_key_pressed(&g->input, SDL_SCANCODE_SPACE);
    int move_down = input_is_key_down(&g->input, SDL_SCANCODE_S) || input_is_key_down(&g->input, SDL_SCANCODE_DOWN);
    
    if (move_left) {
        g->player.vx = -PLAYER_SPEED;
        g->player.facing_right = 0;
        g->player.walking = 1;
    } else if (move_right) {
        g->player.vx = PLAYER_SPEED;
        g->player.facing_right = 1;
        g->player.walking = 1;
    } else {
        g->player.vx = 0;
        g->player.walking = 0;
    }
    
    if (jump && g->player.on_ground) {
        g->player.vy = PLAYER_JUMP_SPEED;
        g->player.on_ground = 0;
    }
    
    if (move_down) {
        g->player.vy += 200.0f * dt;
    }
    
    player_update(&g->player, dt, g->world.width * TILE_SIZE, g->world.height * TILE_SIZE);
    
    {
        float hw = PLAYER_WIDTH / 2.0f;
        float left = g->player.x - hw;
        float right = g->player.x + hw;
        float top = g->player.y - PLAYER_HEIGHT;
        float bottom = g->player.y;
        
        int tile_left = (int)(left / TILE_SIZE);
        int tile_right = (int)(right / TILE_SIZE);
        int tile_top = (int)(top / TILE_SIZE);
        int tile_bottom = (int)(bottom / TILE_SIZE);
        
        g->player.on_ground = 0;
        
        for (int ty = tile_top; ty <= tile_bottom; ty++) {
            for (int tx = tile_left; tx <= tile_right; tx++) {
                if (world_is_solid(&g->world, tx, ty)) {
                    float block_left = tx * TILE_SIZE;
                    float block_right = block_left + TILE_SIZE;
                    float block_top = ty * TILE_SIZE;
                    float block_bottom = block_top + TILE_SIZE;
                    
                    float overlap_left = right - block_left;
                    float overlap_right = block_right - left;
                    float overlap_top = bottom - block_top;
                    float overlap_bottom = block_bottom - top;
                    
                    float min_overlap = overlap_left;
                    int resolve_axis = 0;
                    
                    if (overlap_right < min_overlap) { min_overlap = overlap_right; resolve_axis = 1; }
                    if (overlap_top < min_overlap) { min_overlap = overlap_top; resolve_axis = 2; }
                    if (overlap_bottom < min_overlap) { min_overlap = overlap_bottom; resolve_axis = 3; }
                    
                    switch (resolve_axis) {
                        case 0:
                            g->player.x = block_left - hw;
                            g->player.vx = 0;
                            break;
                        case 1:
                            g->player.x = block_right + hw;
                            g->player.vx = 0;
                            break;
                        case 2:
                            g->player.y = block_top;
                            g->player.vy = 0;
                            g->player.on_ground = 1;
                            break;
                        case 3:
                            g->player.y = block_bottom + PLAYER_HEIGHT;
                            g->player.vy = 0;
                            break;
                    }
                    
                    left = g->player.x - hw;
                    right = g->player.x + hw;
                    top = g->player.y - PLAYER_HEIGHT;
                    bottom = g->player.y;
                }
            }
        }
        

    }
    
    int px = (int)(g->player.x / TILE_SIZE);
    int py = (int)(g->player.y / TILE_SIZE);
    
    for (int dy = -1; dy <= 2; dy++) {
        for (int dx = -2; dx <= 2; dx++) {
            int cx = px + dx;
            int cy = py + dy;
            if (cx >= 0 && cx < g->world.width && cy >= 0 && cy < g->world.height) {
                Tile *t = world_get_tile(&g->world, cx, cy);
                if (t && t->growth_stage > 0 && t->growth_stage < GROWTH_COMPLETE) {
                    t->growth_timer += (uint32_t)(dt * 1000);
                }
            }
        }
    }
    farming_update(&g->world, dt);
    
    int facing_x, facing_y;
    player_get_facing_tile(&g->player, &facing_x, &facing_y);
    
    if (input_is_mouse_down(&g->input, 1)) {
        int mouse_wx, mouse_wy;
        camera_screen_to_world(&g->camera, g->input.mouse_x, g->input.mouse_y, &mouse_wx, &mouse_wy);
        mouse_wx /= TILE_SIZE;
        mouse_wy /= TILE_SIZE;
        int dist_x = mouse_wx - px;
        int dist_y = mouse_wy - py;
        if (dist_x * dist_x + dist_y * dist_y <= 36) {
            Tile *t = world_get_tile(&g->world, mouse_wx, mouse_wy);
            if (t && t->fg != BLOCK_AIR && t->fg != BLOCK_BEDROCK) {
                if (g->player.breaking && g->player.break_x == mouse_wx && g->player.break_y == mouse_wy) {
                    g->player.break_timer += (int)(dt * 1000);
                    int break_time = block_get_break_time(t->fg);
                    int tool_id = inventory_get_hotbar_item(&g->inventory, g->ui.hotbar_selection);
                    int power = item_get_tool_power(tool_id);
                    if (power > 0) {
                        g->player.break_timer += (int)(power * dt * 1000);
                    }
                    if (g->player.break_timer >= break_time) {
                        uint16_t drop = block_get_drop(t->fg);
                        int count = block_get_drop_count(t->fg);
                        if (drop != 0 && count > 0) {
                            inventory_add(&g->inventory, drop, count);
                        }
                        if (t->growth_stage >= GROWTH_COMPLETE) {
                            uint16_t drops[8];
                            int dcounts[8];
                            int ndrops = 0;
                            farming_harvest(&g->world, mouse_wx, mouse_wy, drops, dcounts, &ndrops);
                            for (int i = 0; i < ndrops; i++) {
                                inventory_add(&g->inventory, drops[i], dcounts[i]);
                            }
                            int gem_drop = 1 + (rand() % 3);
                            g->player.gems += gem_drop;
                        }
                        t->fg = BLOCK_AIR;
                        t->growth_stage = 0;
                        t->growth_timer = 0;
                        g->player.breaking = 0;
                        g->player.break_timer = 0;
                    }
                } else {
                    g->player.breaking = 1;
                    g->player.break_x = mouse_wx;
                    g->player.break_y = mouse_wy;
                    g->player.break_timer = 0;
                }
            }
        }
    } else {
        g->player.breaking = 0;
        g->player.break_timer = 0;
    }
    
    if (input_is_mouse_clicked(&g->input, 3)) {
        int mouse_wx, mouse_wy;
        camera_screen_to_world(&g->camera, g->input.mouse_x, g->input.mouse_y, &mouse_wx, &mouse_wy);
        mouse_wx /= TILE_SIZE;
        mouse_wy /= TILE_SIZE;
        int dist_x = mouse_wx - px;
        int dist_y = mouse_wy - py;
        if (dist_x * dist_x + dist_y * dist_y <= 36) {
            Tile *t = world_get_tile(&g->world, mouse_wx, mouse_wy);
            if (t) {
                int hotbar_slot = g->ui.hotbar_selection;
                uint16_t held = inventory_get_hotbar_item(&g->inventory, hotbar_slot);
                int held_count = inventory_get_hotbar_count(&g->inventory, hotbar_slot);
                if (held != 0 && held_count > 0) {
                    const ItemDef *def = item_get_def(held);
                    if (def && def->is_seed && t->growth_stage >= GROWTH_STAGE_1 && t->growth_stage < GROWTH_COMPLETE) {
                        uint16_t tile_seed = (uint16_t)t->extra_data;
                        uint16_t result;
                        if (crafting_splice(held, tile_seed, &result) == 0) {
                            farming_plant_seed(&g->world, mouse_wx, mouse_wy, result);
                            inventory_remove(&g->inventory, held, 1);
                        }
                    } else if (def && t->fg == BLOCK_AIR) {
                        if (def->is_seed) {
                            if (farming_can_plant(&g->world, mouse_wx, mouse_wy)) {
                                farming_plant_seed(&g->world, mouse_wx, mouse_wy, held);
                                inventory_remove(&g->inventory, held, 1);
                            }
                        } else if (def->category == ITEM_CAT_BLOCK) {
                            t->fg = held;
                            inventory_remove(&g->inventory, held, 1);
                        }
                    }
                }
            }
        }
    }
    
    for (int i = SDL_SCANCODE_1; i <= SDL_SCANCODE_9; i++) {
        if (input_is_key_pressed(&g->input, i)) {
            g->ui.hotbar_selection = i - SDL_SCANCODE_1;
        }
    }
    
    if (input_is_key_pressed(&g->input, SDL_SCANCODE_F5)) {
        game_save_all(g);
    }
    
    camera_set_target(&g->camera, g->player.x, g->player.y);
    camera_update(&g->camera, dt);
    
    g->autosave_timer -= dt;
    if (g->autosave_timer <= 0) {
        game_save_all(g);
        g->autosave_timer = AUTOSAVE_INTERVAL;
    }
    
    input_update(&g->input);
}

static void game_render(Game *g) {
    renderer_clear(&g->renderer, 0.4f, 0.7f, 1.0f);
    
    float cam_left = g->camera.x - g_screen_w / 2.0f;
    float cam_top = g->camera.y - g_screen_h / 2.0f;
    int start_x = (int)(cam_left / TILE_SIZE) - 1;
    int start_y = (int)(cam_top / TILE_SIZE) - 1;
    int end_x = start_x + (g_screen_w / TILE_SIZE) + 3;
    int end_y = start_y + (g_screen_h / TILE_SIZE) + 3;
    
    if (start_x < 0) start_x = 0;
    if (start_y < 0) start_y = 0;
    if (end_x > g->world.width) end_x = g->world.width;
    if (end_y > g->world.height) end_y = g->world.height;
    
    renderer_begin_tile_batch(&g->renderer);
    
    for (int y = start_y; y < end_y; y++) {
        for (int x = start_x; x < end_x; x++) {
            Tile *t = world_get_tile(&g->world, x, y);
            if (!t) continue;
            
            int sx, sy;
            camera_world_to_screen(&g->camera, x * TILE_SIZE, y * TILE_SIZE, &sx, &sy);
            
            if (t->bg != BLOCK_AIR) {
                int sprite = block_get_sprite(t->bg);
                renderer_draw_tile(&g->renderer, sx, sy, sprite, 0);
            }
            
            if (t->fg != BLOCK_AIR) {
                int sprite = block_get_sprite(t->fg);
                if (t->growth_stage > 0 && t->growth_stage < GROWTH_COMPLETE) {
                    int dirt_sprite = block_get_sprite(BLOCK_DIRT);
                    renderer_draw_tile(&g->renderer, sx, sy, dirt_sprite, 0);
                    float height_factor = 0.3f + 0.7f * (t->growth_stage / (float)GROWTH_COMPLETE);
                    int draw_h = (int)(TILE_SIZE * height_factor);
                    renderer_draw_tile_scaled(&g->renderer, sx, sy + TILE_SIZE - draw_h,
                        TILE_SIZE, draw_h, sprite, 0);
                } else if (t->growth_stage >= GROWTH_COMPLETE) {
                    renderer_draw_tile(&g->renderer, sx, sy, sprite, 0);
                    int leaf_sprite = block_get_sprite(BLOCK_LEAVES);
                    renderer_draw_tile_scaled(&g->renderer, sx - 4, sy - 12,
                        TILE_SIZE + 8, TILE_SIZE / 2 + 12, leaf_sprite, 0);
                    renderer_draw_tile_border(&g->renderer, sx, sy);
                } else {
                    renderer_draw_tile(&g->renderer, sx, sy, sprite, 0);
                    renderer_draw_tile_border(&g->renderer, sx, sy);
                }
            }
        }
    }
    
    {
        int psx, psy;
        camera_world_to_screen(&g->camera, g->player.x - PLAYER_WIDTH / 2, g->player.y - PLAYER_HEIGHT, &psx, &psy);
        renderer_draw_rect(&g->renderer, psx, psy, PLAYER_WIDTH, PLAYER_HEIGHT,
            1.0f, 0.8f, 0.6f, 1.0f);
        renderer_draw_rect(&g->renderer, psx + 4, psy + 4, 6, 6, 0.0f, 0.0f, 0.0f, 1.0f);
        renderer_draw_rect(&g->renderer, psx + 14, psy + 4, 6, 6, 0.0f, 0.0f, 0.0f, 1.0f);
    }
    
    if (g->player.breaking) {
        int bsx, bsy;
        camera_world_to_screen(&g->camera, g->player.break_x * TILE_SIZE, g->player.break_y * TILE_SIZE, &bsx, &bsy);
        Tile *bt = world_get_tile(&g->world, g->player.break_x, g->player.break_y);
        if (bt) {
            int break_time = block_get_break_time(bt->fg);
            float progress = (break_time > 0) ? (float)g->player.break_timer / break_time : 0.0f;
            renderer_draw_rect(&g->renderer, bsx, bsy, TILE_SIZE, TILE_SIZE,
                1.0f, 1.0f, 1.0f, progress * 0.5f);
        }
    }
    
    renderer_end_tile_batch(&g->renderer);
    
    renderer_begin_ui(&g->renderer);
    
    if (g->ui.state == UI_STATE_NONE) {
        ui_render_hud(&g->ui, &g->renderer, g->player.gems, g->player.health);
        ui_render_hotbar(&g->ui, &g->renderer, g->inventory.items, g->inventory.counts, g->ui.hotbar_selection);
        
        int mouse_wx, mouse_wy;
        camera_screen_to_world(&g->camera, g->input.mouse_x, g->input.mouse_y, &mouse_wx, &mouse_wy);
        int tx = mouse_wx / TILE_SIZE;
        int ty = mouse_wy / TILE_SIZE;
        if (tx >= 0 && tx < g->world.width && ty >= 0 && ty < g->world.height) {
            Tile *t = world_get_tile(&g->world, tx, ty);
            if (t && t->fg != BLOCK_AIR) {
                const char *name = block_get_name(t->fg);
                if (name) {
                    int tw = renderer_text_width(&g->renderer, name, 2.0f);
                    int th = 16;
                    renderer_draw_rect(&g->renderer, g->input.mouse_x - tw/2 - 4, g->input.mouse_y - th - 8, tw + 8, th + 4, 0.0f, 0.0f, 0.0f, 0.8f);
                    renderer_draw_text(&g->renderer, name, g->input.mouse_x - tw/2, g->input.mouse_y - th - 4, 2.0f, 1.0f, 1.0f, 1.0f);
                }
            }
        }
    } else if (g->ui.state == UI_STATE_INVENTORY) {
        ui_render_inventory_screen(&g->ui, &g->renderer, g->inventory.items, g->inventory.counts, INVENTORY_SIZE);
    } else if (g->ui.state == UI_STATE_STORE) {
        ui_render_store_screen(&g->ui, &g->renderer, g->player.gems);
    }
    
    renderer_draw_text(&g->renderer, "E: Inv  B: Store  LMB: Break  RMB: Place  F5: Save  F11: Fullscreen  ESC: Quit", 8, g_screen_h - 16, 1.0f, 1.0f, 1.0f, 1.0f);
    
    renderer_end_ui(&g->renderer);
    
    renderer_present(&g->renderer);
}

static void game_shutdown(Game *g) {
    game_save_all(g);
    world_free(&g->world);
    renderer_shutdown(&g->renderer);
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    
    Game game;
    game_init(&game);
    
    while (g_running) {
        uint64_t now = SDL_GetPerformanceCounter();
        float dt = (float)(now - game.last_time) / (float)SDL_GetPerformanceFrequency();
        game.last_time = now;
        
        if (dt > 0.1f) dt = 0.1f;
        
        game_handle_events(&game);
        game_update(&game, dt);
        game_render(&game);
        
        uint64_t frame_end = SDL_GetPerformanceCounter();
        float elapsed_ms = (float)(frame_end - now) / (float)SDL_GetPerformanceFrequency() * 1000.0f;
        if (elapsed_ms < FRAME_TIME) {
            SDL_Delay((Uint32)(FRAME_TIME - elapsed_ms));
        }
    }
    
    game_shutdown(&game);
    return 0;
}
