#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/stat.h>

#include <SDL2/SDL.h>

#include "engine/renderer.h"
#include "engine/camera.h"
#include "engine/input.h"
#include "engine/ui.h"
#include "engine/world_select.h"
#include "engine/clouds.h"
#include "world/world.h"
#include "world/block.h"
#include "world/items.h"
#include "game/player.h"
#include "game/inventory.h"
#include "game/farming.h"
#include "game/crafting.h"
#include "game/store.h"
#include "game/interact.h"
#include "game/lava.h"
#include "game/explosive.h"
#include "engine/char_select.h"
#include "game/character.h"


#define FPS_CAP 60
#define FRAME_TIME (1000.0 / FPS_CAP)
#define AUTOSAVE_INTERVAL 60.0f
#define PROFILE_PATH "res/worlds/player.dat"

typedef enum {
    GAME_STATE_CHAR_SELECT,
    GAME_STATE_MENU,
    GAME_STATE_PLAYING
} GameState;

static int g_running = 1;
static GameState g_game_state = GAME_STATE_CHAR_SELECT;
static WorldSelect g_world_select;
static CharSelect g_char_select;

typedef struct {
    Renderer renderer;
    Camera camera;
    Clouds clouds;
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
    char current_char_name[CHAR_NAME_MAX + 1];
    char last_world[WORLD_NAME_MAX + 1];
} Game;

static int handle_equip_swap(Inventory *inv, Player *p, int a, int b)
{
    if (a >= 36 && b >= 36)
        return 0;

    int inv_idx, equip_idx;
    if (a >= 36) {
        equip_idx = a;
        inv_idx = b;
    } else {
        equip_idx = b;
        inv_idx = a;
    }

    uint16_t inv_item = inv->items[inv_idx];
    uint16_t equip_item;
    if (equip_idx == 36)
        equip_item = p->equipped_hat;
    else if (equip_idx == 37)
        equip_item = p->equipped_shirt;
    else
        equip_item = p->equipped_pants;

    if (inv_item != 0 && item_clothing_slot(inv_item) != (equip_idx - 36))
        return -1;

    if (equip_idx == 36)
        p->equipped_hat = inv_item;
    else if (equip_idx == 37)
        p->equipped_shirt = inv_item;
    else
        p->equipped_pants = inv_item;

    inv->items[inv_idx] = equip_item;
    inv->counts[inv_idx] = equip_item ? 1 : 0;

    return 0;
}

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


static void ensure_worlds_dir(void) {
    mkdir("res", 0755);
    mkdir("res/worlds", 0755);
    mkdir("res/chars", 0755);
}

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
    explosive_reset();
    clouds_init(&g->clouds, g->world.name, g->world.width * TILE_SIZE, g->world.height * TILE_SIZE);

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
    character_migrate_from_profile(PROFILE_PATH);
    char_select_init(&g_char_select);
}

static void game_handle_events(Game *g) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) {
            g_running = 0;
            return;
        }

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
            if (ui_handle_sign_edit_event(&g->ui, &g->world, &e)) {
                continue;
            }
        }
    }
}

static void game_update(Game *g, float dt) {
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

    clouds_update(&g->clouds, dt);
    if (g->exit_confirm_active) {
        input_update(&g->input);
        return;
    }

    if (g->ui.state != UI_STATE_NONE) {
        ui_update(&g->ui, &g->input, &g->renderer);
        
        if (g->ui.state == UI_STATE_STORE) {
            store_handle_click(&g->store, &g->inventory, &g->input, &g->player.gems, g->ui.store_category);
        }
        
        if (g->ui.state == UI_STATE_INVENTORY && g->ui.drag_from_slot >= 0 && g->ui.selected_slot >= 0 &&
            g->ui.drag_from_slot != g->ui.selected_slot) {
            int a = g->ui.drag_from_slot;
            int b = g->ui.selected_slot;
            if (a >= 36 || b >= 36) {
                if (handle_equip_swap(&g->inventory, &g->player, a, b) < 0) {
                    g->ui.selected_slot = -1;
                }
            } else {
                inventory_swap_slots(&g->inventory, a, b);
            }
            g->ui.drag_from_slot = -1;
        }
        
        if (g->ui.state == UI_STATE_SIGN_EDIT) {
            ui_update_sign_edit(&g->ui, &g->input);
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
    
    player_collide(&g->player, &g->world);
    lava_update(&g->world, &g->player, dt);
    explosive_update(&g->world, &g->player, dt);
    
    int px = (int)(g->player.x / TILE_SIZE);
    int py = (int)(g->player.y / TILE_SIZE);
    farming_tick_nearby(&g->world, px, py, dt);
    farming_update(&g->world, dt);

    {
        int sign_x, sign_y;
        int click_result = interact_handle_click(&g->world, &g->player,
            &g->inventory, g->ui.hotbar_selection, &g->input, &g->camera,
            &sign_x, &sign_y);
        if (click_result == 2) {
            g->sign_overlay_active = 1;
            g->sign_overlay_timer = 3.0f;
            g->sign_overlay_x = sign_x;
            g->sign_overlay_y = sign_y;
        }
        if (!click_result) {
            interact_handle_break(&g->world, &g->player, &g->inventory,
                g->ui.hotbar_selection, &g->input, &g->camera, dt);
        }
    }

    interact_handle_place(&g->world, &g->player, &g->inventory,
        g->ui.hotbar_selection, &g->input, &g->camera, &g->ui,
        &g->portal_link_pending, &g->portal_link_x, &g->portal_link_y);
    
    for (int i = SDL_SCANCODE_1; i <= SDL_SCANCODE_9; i++) {
        if (input_is_key_pressed(&g->input, i)) {
            g->ui.hotbar_selection = i - SDL_SCANCODE_1;
        }
    }
    
    if (g->input.mouse_scroll_y != 0) {
        camera_set_zoom(&g->camera, g->camera.zoom_target + g->input.mouse_scroll_y * 0.1f);
    }
    
    if (input_is_key_pressed(&g->input, SDL_SCANCODE_F5)) {
        game_save_all(g);
    }
    
    camera_set_target(&g->camera, g->player.x, g->player.y);
    camera_update(&g->camera, dt);
    
    if (g->sign_overlay_active) {
        g->sign_overlay_timer -= dt;
        if (g->sign_overlay_timer <= 0) {
            g->sign_overlay_active = 0;
        }
    }
    
    g->autosave_timer -= dt;
    if (g->autosave_timer <= 0) {
        game_save_all(g);
        g->autosave_timer = AUTOSAVE_INTERVAL;
    }
    
    input_update(&g->input);
}

static void game_render(Game *g) {
    if (g_game_state == GAME_STATE_CHAR_SELECT) {
        renderer_clear(&g->renderer, 0.05f, 0.05f, 0.15f);
        char_select_render(&g_char_select, &g->renderer);
        renderer_present(&g->renderer);
        return;
    }

    if (g_game_state == GAME_STATE_MENU) {
        renderer_clear(&g->renderer, 0.05f, 0.05f, 0.15f);
        world_select_render(&g_world_select, &g->renderer);
        renderer_present(&g->renderer);
        return;
    }

    renderer_clear(&g->renderer, 0.4f, 0.7f, 1.0f);
    
    clouds_render(&g->clouds, &g->renderer, &g->camera);
    renderer_begin_tile_batch(&g->renderer, g->camera.zoom);
    
    renderer_draw_world(&g->renderer, &g->world, &g->camera);
    renderer_draw_player(&g->renderer, &g->player, &g->camera);
    renderer_draw_break_progress(&g->renderer, &g->world, &g->player, &g->camera);
    explosive_render(&g->renderer, &g->camera);
    
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
        if (g->sign_overlay_active) {
            const char *txt = interact_get_sign_text(&g->world, g->sign_overlay_x, g->sign_overlay_y);
            if (txt) {
                int sox, soy;
                camera_world_to_screen(&g->camera, g->sign_overlay_x * TILE_SIZE, g->sign_overlay_y * TILE_SIZE, &sox, &soy);
                int px = (int)(sox * g->camera.zoom);
                int py = (int)(soy * g->camera.zoom);
                int tw = renderer_text_width(&g->renderer, txt, 1.5f);
                int th = 16;
                int label_x = px + (int)(TILE_SIZE * g->camera.zoom) / 2 - tw / 2;
                int label_y = py - th - 8;
                renderer_draw_rect(&g->renderer, label_x - 4, label_y - 2, tw + 8, th + 6, 0.0f, 0.0f, 0.0f, 0.8f);
                renderer_draw_text(&g->renderer, txt, label_x, label_y, 1.5f, 1.0f, 1.0f, 1.0f);
            }
        }
    } else if (g->ui.state == UI_STATE_INVENTORY) {
        uint16_t equipped[3] = {g->player.equipped_hat, g->player.equipped_shirt, g->player.equipped_pants};
        ui_render_inventory_screen(&g->ui, &g->renderer, g->inventory.items, g->inventory.counts, INVENTORY_SIZE, equipped);
    } else if (g->ui.state == UI_STATE_STORE) {
        ui_render_store_screen(&g->ui, &g->renderer, g->player.gems);
    } else if (g->ui.state == UI_STATE_SIGN_EDIT) {
        ui_render_sign_edit(&g->ui, &g->renderer);
    }
    
    if (g->exit_confirm_active) {
        ui_render_exit_confirm(&g->renderer);
    }

    renderer_draw_text(&g->renderer, "E: Inv  B: Store  LMB: Break  RMB: Place  Scroll: Zoom  F5: Save  F11: FS  ESC: Menu", 8, g_screen_h - 16, 1.0f, 1.0f, 1.0f, 1.0f);
    
    renderer_end_ui(&g->renderer);
    
    renderer_present(&g->renderer);
}

static void game_shutdown(Game *g) {
    if (g_game_state == GAME_STATE_PLAYING) {
        game_save_all(g);
        world_free(&g->world);
    }
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
