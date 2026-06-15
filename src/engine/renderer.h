#ifndef RENDERER_H
#define RENDERER_H

#include <SDL2/SDL.h>
#include "camera.h"
#include "block_texture.h"
#include "../world/world.h"
#include "../game/player.h"

#define TILE_SIZE 32

extern int g_screen_w;
extern int g_screen_h;

typedef struct {
    SDL_Window *window;
    SDL_GLContext gl_context;
    unsigned int atlas_textures[MAX_ATLAS_FRAMES];
    int atlas_frame_count;
    int atlas_cols;
    int atlas_rows;
    int fullscreen;
    int windowed_x, windowed_y, windowed_w, windowed_h;
} Renderer;

int renderer_init(Renderer *r);
void renderer_shutdown(Renderer *r);
void renderer_clear(Renderer *r, float r_col, float g_col, float b_col);
void renderer_present(Renderer *r);

void renderer_draw_tile(Renderer *r, int screen_x, int screen_y, int tile_id, int frame);
void renderer_draw_tile_scaled(Renderer *r, int screen_x, int screen_y, int w, int h, int tile_id, int frame);
void renderer_draw_rect(Renderer *r, int x, int y, int w, int h, float r_col, float g, float b_col, float a);
void renderer_draw_text(Renderer *r, const char *text, int x, int y, float scale, float r_col, float g, float b_col);
void renderer_draw_number(Renderer *r, int num, int x, int y, float scale, float r_col, float g, float b_col);
int renderer_text_width(Renderer *r, const char *text, float scale);

void renderer_begin_tile_batch(Renderer *r, float zoom);
void renderer_end_tile_batch(Renderer *r);
void renderer_begin_ui(Renderer *r);
void renderer_end_ui(Renderer *r);

unsigned int renderer_load_texture(const unsigned char *data, int width, int height, int channels);
void renderer_generate_atlas(Renderer *r);

void renderer_toggle_fullscreen(Renderer *r);
int renderer_is_fullscreen(Renderer *r);
void renderer_get_size(Renderer *r, int *w, int *h);

void renderer_atlas_uv(int sprite_id, float *u0, float *v0, float *u1, float *v1);

void renderer_draw_tile_border(Renderer *r, int screen_x, int screen_y);

void renderer_draw_world(Renderer *r, World *w, Camera *cam);
void renderer_draw_player(Renderer *r, Player *p, Camera *cam);
void renderer_draw_break_progress(Renderer *r, World *w, Player *p, Camera *cam);

#endif
