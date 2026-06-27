#include "renderer.h"
#include "block_texture.h"
#include "../world/block.h"
#include "../world/items.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <GL/gl.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

int g_screen_w = 1280;
int g_screen_h = 720;

static void (*gl_mip_gen)(GLenum) = NULL;

#define STB_IMAGE_IMPLEMENTATION
#include "../stb_image.h"

static const unsigned char FONT_5X7[][7] = {
    ['A'] = {
        0b01110,
        0b10001,
        0b10001,
        0b11111,
        0b10001,
        0b10001,
        0b10001
    },
    ['B'] = {
        0b11110,
        0b10001,
        0b10001,
        0b11110,
        0b10001,
        0b10001,
        0b11110
    },
    ['C'] = {
        0b01110,
        0b10001,
        0b10000,
        0b10000,
        0b10000,
        0b10001,
        0b01110
    },
    ['D'] = {
        0b11110,
        0b10001,
        0b10001,
        0b10001,
        0b10001,
        0b10001,
        0b11110
    },
    ['E'] = {
        0b11111,
        0b10000,
        0b10000,
        0b11110,
        0b10000,
        0b10000,
        0b11111
    },
    ['F'] = {
        0b11111,
        0b10000,
        0b10000,
        0b11110,
        0b10000,
        0b10000,
        0b10000
    },
    ['G'] = {
        0b01110,
        0b10001,
        0b10000,
        0b10111,
        0b10001,
        0b10001,
        0b01110
    },
    ['H'] = {
        0b10001,
        0b10001,
        0b10001,
        0b11111,
        0b10001,
        0b10001,
        0b10001
    },
    ['I'] = {
        0b01110,
        0b00100,
        0b00100,
        0b00100,
        0b00100,
        0b00100,
        0b01110
    },
    ['J'] = {
        0b00111,
        0b00010,
        0b00010,
        0b00010,
        0b00010,
        0b10010,
        0b01100
    },
    ['K'] = {
        0b10001,
        0b10010,
        0b10100,
        0b11000,
        0b10100,
        0b10010,
        0b10001
    },
    ['L'] = {
        0b10000,
        0b10000,
        0b10000,
        0b10000,
        0b10000,
        0b10000,
        0b11111
    },
    ['M'] = {
        0b10001,
        0b11011,
        0b10101,
        0b10101,
        0b10001,
        0b10001,
        0b10001
    },
    ['N'] = {
        0b10001,
        0b11001,
        0b10101,
        0b10011,
        0b10001,
        0b10001,
        0b10001
    },
    ['O'] = {
        0b01110,
        0b10001,
        0b10001,
        0b10001,
        0b10001,
        0b10001,
        0b01110
    },
    ['P'] = {
        0b11110,
        0b10001,
        0b10001,
        0b11110,
        0b10000,
        0b10000,
        0b10000
    },
    ['Q'] = {
        0b01110,
        0b10001,
        0b10001,
        0b10001,
        0b10101,
        0b10010,
        0b01101
    },
    ['R'] = {
        0b11110,
        0b10001,
        0b10001,
        0b11110,
        0b10100,
        0b10010,
        0b10001
    },
    ['S'] = {
        0b01110,
        0b10001,
        0b10000,
        0b01110,
        0b00001,
        0b10001,
        0b01110
    },
    ['T'] = {
        0b11111,
        0b00100,
        0b00100,
        0b00100,
        0b00100,
        0b00100,
        0b00100
    },
    ['U'] = {
        0b10001,
        0b10001,
        0b10001,
        0b10001,
        0b10001,
        0b10001,
        0b01110
    },
    ['V'] = {
        0b10001,
        0b10001,
        0b10001,
        0b10001,
        0b01010,
        0b01010,
        0b00100
    },
    ['W'] = {
        0b10001,
        0b10001,
        0b10001,
        0b10101,
        0b10101,
        0b10101,
        0b01010
    },
    ['X'] = {
        0b10001,
        0b10001,
        0b01010,
        0b00100,
        0b01010,
        0b10001,
        0b10001
    },
    ['Y'] = {
        0b10001,
        0b10001,
        0b01010,
        0b00100,
        0b00100,
        0b00100,
        0b00100
    },
    ['Z'] = {
        0b11111,
        0b00001,
        0b00010,
        0b00100,
        0b01000,
        0b10000,
        0b11111
    },
    ['0'] = {
        0b01110,
        0b10011,
        0b10101,
        0b10101,
        0b10101,
        0b11001,
        0b01110
    },
    ['1'] = {
        0b00100,
        0b01100,
        0b00100,
        0b00100,
        0b00100,
        0b00100,
        0b01110
    },
    ['2'] = {
        0b01110,
        0b10001,
        0b00001,
        0b00110,
        0b01000,
        0b10000,
        0b11111
    },
    ['3'] = {
        0b01110,
        0b10001,
        0b00001,
        0b00110,
        0b00001,
        0b10001,
        0b01110
    },
    ['4'] = {
        0b00010,
        0b00110,
        0b01010,
        0b10010,
        0b11111,
        0b00010,
        0b00010
    },
    ['5'] = {
        0b11111,
        0b10000,
        0b11110,
        0b00001,
        0b00001,
        0b10001,
        0b01110
    },
    ['6'] = {
        0b00110,
        0b01000,
        0b10000,
        0b11110,
        0b10001,
        0b10001,
        0b01110
    },
    ['7'] = {
        0b11111,
        0b00001,
        0b00010,
        0b00100,
        0b01000,
        0b01000,
        0b01000
    },
    ['8'] = {
        0b01110,
        0b10001,
        0b10001,
        0b01110,
        0b10001,
        0b10001,
        0b01110
    },
    ['9'] = {
        0b01110,
        0b10001,
        0b10001,
        0b01111,
        0b00001,
        0b00010,
        0b01100
    },
    [' '] = {
        0b00000,
        0b00000,
        0b00000,
        0b00000,
        0b00000,
        0b00000,
        0b00000
    },
    [':'] = {
        0b00000,
        0b00100,
        0b00100,
        0b00000,
        0b00100,
        0b00100,
        0b00000
    },
    ['!'] = {
        0b00100,
        0b00100,
        0b00100,
        0b00100,
        0b00100,
        0b00000,
        0b00100
    },
    ['.'] = {
        0b00000,
        0b00000,
        0b00000,
        0b00000,
        0b00000,
        0b00000,
        0b00100
    },
    ['-'] = {
        0b00000,
        0b00000,
        0b00000,
        0b11111,
        0b00000,
        0b00000,
        0b00000
    },
    ['/'] = {
        0b00001,
        0b00010,
        0b00010,
        0b00100,
        0b01000,
        0b01000,
        0b10000
    },
    ['('] = {
        0b00010,
        0b00100,
        0b01000,
        0b01000,
        0b01000,
        0b00100,
        0b00010
    },
    [')'] = {
        0b01000,
        0b00100,
        0b00010,
        0b00010,
        0b00010,
        0b00100,
        0b01000
    },
    ['+'] = {
        0b00000,
        0b00100,
        0b00100,
        0b11111,
        0b00100,
        0b00100,
        0b00000
    },
    ['='] = {
        0b00000,
        0b00000,
        0b11111,
        0b00000,
        0b11111,
        0b00000,
        0b00000
    },
    ['?'] = {
        0b01110,
        0b10001,
        0b00001,
        0b00110,
        0b00100,
        0b00000,
        0b00100
    },
    [','] = {
        0b00000,
        0b00000,
        0b00000,
        0b00000,
        0b00000,
        0b00100,
        0b01000
    },
    ['_'] = {
        0b00000,
        0b00000,
        0b00000,
        0b00000,
        0b00000,
        0b00000,
        0b11111
    },
    ['a'] = {
        0b00000,
        0b00000,
        0b01110,
        0b00001,
        0b01111,
        0b10001,
        0b01111
    },
    ['b'] = {
        0b10000,
        0b10000,
        0b10110,
        0b11001,
        0b10001,
        0b10001,
        0b11110
    },
    ['c'] = {
        0b00000,
        0b00000,
        0b01110,
        0b10000,
        0b10000,
        0b10001,
        0b01110
    },
    ['d'] = {
        0b00001,
        0b00001,
        0b01101,
        0b10011,
        0b10001,
        0b10001,
        0b01111
    },
    ['e'] = {
        0b00000,
        0b00000,
        0b01110,
        0b10001,
        0b11111,
        0b10000,
        0b01110
    },
    ['f'] = {
        0b00110,
        0b01001,
        0b01000,
        0b11100,
        0b01000,
        0b01000,
        0b01000
    },
    ['g'] = {
        0b00000,
        0b01111,
        0b10001,
        0b10001,
        0b01111,
        0b00001,
        0b01110
    },
    ['h'] = {
        0b10000,
        0b10000,
        0b10110,
        0b11001,
        0b10001,
        0b10001,
        0b10001
    },
    ['i'] = {
        0b00100,
        0b00000,
        0b01100,
        0b00100,
        0b00100,
        0b00100,
        0b01110
    },
    ['j'] = {
        0b00010,
        0b00000,
        0b00110,
        0b00010,
        0b00010,
        0b10010,
        0b01100
    },
    ['k'] = {
        0b10000,
        0b10000,
        0b10010,
        0b10100,
        0b11000,
        0b10100,
        0b10010
    },
    ['l'] = {
        0b01100,
        0b00100,
        0b00100,
        0b00100,
        0b00100,
        0b00100,
        0b01110
    },
    ['m'] = {
        0b00000,
        0b00000,
        0b11010,
        0b10101,
        0b10101,
        0b10001,
        0b10001
    },
    ['n'] = {
        0b00000,
        0b00000,
        0b10110,
        0b11001,
        0b10001,
        0b10001,
        0b10001
    },
    ['o'] = {
        0b00000,
        0b00000,
        0b01110,
        0b10001,
        0b10001,
        0b10001,
        0b01110
    },
    ['p'] = {
        0b00000,
        0b00000,
        0b11110,
        0b10001,
        0b11110,
        0b10000,
        0b10000
    },
    ['q'] = {
        0b00000,
        0b00000,
        0b01101,
        0b10011,
        0b01111,
        0b00001,
        0b00001
    },
    ['r'] = {
        0b00000,
        0b00000,
        0b10110,
        0b11001,
        0b10000,
        0b10000,
        0b10000
    },
    ['s'] = {
        0b00000,
        0b00000,
        0b01110,
        0b10000,
        0b01110,
        0b00001,
        0b11110
    },
    ['t'] = {
        0b01000,
        0b01000,
        0b11100,
        0b01000,
        0b01000,
        0b01001,
        0b00110
    },
    ['u'] = {
        0b00000,
        0b00000,
        0b10001,
        0b10001,
        0b10001,
        0b10011,
        0b01101
    },
    ['v'] = {
        0b00000,
        0b00000,
        0b10001,
        0b10001,
        0b10001,
        0b01010,
        0b00100
    },
    ['w'] = {
        0b00000,
        0b00000,
        0b10001,
        0b10001,
        0b10101,
        0b10101,
        0b01010
    },
    ['x'] = {
        0b00000,
        0b00000,
        0b10001,
        0b01010,
        0b00100,
        0b01010,
        0b10001
    },
    ['y'] = {
        0b00000,
        0b00000,
        0b10001,
        0b10001,
        0b01111,
        0b00001,
        0b01110
    },
    ['z'] = {
        0b00000,
        0b00000,
        0b11111,
        0b00010,
        0b00100,
        0b01000,
        0b11111
    }
};

#define FONT_CHAR_WIDTH 5
#define FONT_CHAR_HEIGHT 7
#define FONT_MAX_CHAR 128

static void draw_char(Renderer *r, char c, int x, int y, float scale, float cr, float cg, float cb) {
    unsigned char ch = (unsigned char)c;
    if (ch >= FONT_MAX_CHAR) return;
    const unsigned char *glyph = FONT_5X7[ch];
    if (!glyph) return;
    for (int row = 0; row < FONT_CHAR_HEIGHT; row++) {
        for (int col = 0; col < FONT_CHAR_WIDTH; col++) {
            if (glyph[row] & (1 << (FONT_CHAR_WIDTH - 1 - col))) {
                float px = x + col * scale;
                float py = y + row * scale;
                renderer_draw_rect(r, (int)px, (int)py,
                                   (int)(scale + 0.5f), (int)(scale + 0.5f),
                                   cr, cg, cb, 1.0f);
            }
        }
    }
}

static void get_tile_color(int tile_id, float *cr, float *cg, float *cb) {
    switch (tile_id) {
        case 0:  *cr = 0.6f;  *cg = 0.4f;  *cb = 0.2f;  break;
        case 1:  *cr = 0.0f;  *cg = 0.8f;  *cb = 0.0f;  break;
        case 2:  *cr = 0.5f;  *cg = 0.5f;  *cb = 0.5f;  break;
        case 3:  *cr = 0.2f;  *cg = 0.2f;  *cb = 0.8f;  break;
        case 4:  *cr = 0.8f;  *cg = 0.8f;  *cb = 0.0f;  break;
        case 5:  *cr = 0.7f;  *cg = 0.0f;  *cb = 0.0f;  break;
        case 6:  *cr = 0.0f;  *cg = 0.6f;  *cb = 0.6f;  break;
        case 7:  *cr = 0.9f;  *cg = 0.9f;  *cb = 0.9f;  break;
        case 8:  *cr = 0.4f;  *cg = 0.7f;  *cb = 0.2f;  break;
        case 9:  *cr = 0.6f;  *cg = 0.3f;  *cb = 0.1f;  break;
        default: *cr = 1.0f;  *cg = 0.0f;  *cb = 1.0f;  break;
    }
}

static void setup_ortho(int w, int h) {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, w, h, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

int renderer_init(Renderer *r) {
    memset(r, 0, sizeof(*r));
    r->windowed_w = 1280;
    r->windowed_h = 720;

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return -1;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    r->window = SDL_CreateWindow("Growtopia Clone",
                                  SDL_WINDOWPOS_CENTERED,
                                  SDL_WINDOWPOS_CENTERED,
                                   1280, 720,
                                   SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (!r->window) {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        return -1;
    }

    r->gl_context = SDL_GL_CreateContext(r->window);
    if (!r->gl_context) {
        fprintf(stderr, "SDL_GL_CreateContext failed: %s\n", SDL_GetError());
        return -1;
    }

    SDL_GL_MakeCurrent(r->window, r->gl_context);
    SDL_GL_SetSwapInterval(1);

    gl_mip_gen = (void (*)(GLenum))SDL_GL_GetProcAddress("glGenerateMipmap");
    if (!gl_mip_gen)
        gl_mip_gen = (void (*)(GLenum))SDL_GL_GetProcAddress("glGenerateMipmapEXT");

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_TEXTURE_2D);

    setup_ortho(1280, 720);

    return 0;
}

void renderer_shutdown(Renderer *r) {
    for (int i = 0; i < r->atlas_frame_count; i++) {
        if (r->atlas_textures[i]) {
            glDeleteTextures(1, &r->atlas_textures[i]);
            r->atlas_textures[i] = 0;
        }
    }
    r->atlas_frame_count = 0;
    if (r->gl_context) {
        SDL_GL_DeleteContext(r->gl_context);
        r->gl_context = NULL;
    }
    if (r->window) {
        SDL_DestroyWindow(r->window);
        r->window = NULL;
    }
    SDL_Quit();
}

void renderer_clear(Renderer *r, float r_col, float g_col, float b_col) {
    (void)r;
    glClearColor(r_col, g_col, b_col, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}

void renderer_present(Renderer *r) {
    SDL_GL_SwapWindow(r->window);
}

void renderer_draw_rect(Renderer *r, int x, int y, int w, int h,
                         float r_col, float g, float b_col, float a) {
    (void)r;
    glColor4f(r_col, g, b_col, a);
    glBegin(GL_QUADS);
    glVertex2f((float)x, (float)y);
    glVertex2f((float)(x + w), (float)y);
    glVertex2f((float)(x + w), (float)(y + h));
    glVertex2f((float)x, (float)(y + h));
    glEnd();
}

static unsigned int renderer_current_atlas(Renderer *r) {
    if (r->atlas_frame_count <= 1) return r->atlas_textures[0];
    Uint32 t = SDL_GetTicks();
    int frame = (t / ATLAS_FRAME_MS) % r->atlas_frame_count;
    return r->atlas_textures[frame];
}

void renderer_draw_tile(Renderer *r, int screen_x, int screen_y, int tile_id, int frame) {
    (void)frame;
    if (r->atlas_frame_count == 0) {
        float cr, cg, cb;
        get_tile_color(tile_id, &cr, &cg, &cb);
        renderer_draw_rect(r, screen_x, screen_y, TILE_SIZE, TILE_SIZE, cr, cg, cb, 1.0f);
        return;
    }
    float u0, v0, u1, v1;
    renderer_atlas_uv(tile_id, &u0, &v0, &u1, &v1);
    glEnable(GL_TEXTURE_2D);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glBegin(GL_QUADS);
    glTexCoord2f(u0, v0); glVertex2f((float)screen_x, (float)screen_y);
    glTexCoord2f(u1, v0); glVertex2f((float)(screen_x + TILE_SIZE), (float)screen_y);
    glTexCoord2f(u1, v1); glVertex2f((float)(screen_x + TILE_SIZE), (float)(screen_y + TILE_SIZE));
    glTexCoord2f(u0, v1); glVertex2f((float)screen_x, (float)(screen_y + TILE_SIZE));
    glEnd();
    glDisable(GL_TEXTURE_2D);
}

void renderer_draw_tile_scaled(Renderer *r, int screen_x, int screen_y, int w, int h,
                                int tile_id, int frame) {
    (void)frame;
    if (r->atlas_frame_count == 0) {
        float cr, cg, cb;
        get_tile_color(tile_id, &cr, &cg, &cb);
        renderer_draw_rect(r, screen_x, screen_y, w, h, cr, cg, cb, 1.0f);
        return;
    }
    float u0, v0, u1, v1;
    renderer_atlas_uv(tile_id, &u0, &v0, &u1, &v1);
    glEnable(GL_TEXTURE_2D);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glBegin(GL_QUADS);
    glTexCoord2f(u0, v0); glVertex2f((float)screen_x, (float)screen_y);
    glTexCoord2f(u1, v0); glVertex2f((float)(screen_x + w), (float)screen_y);
    glTexCoord2f(u1, v1); glVertex2f((float)(screen_x + w), (float)(screen_y + h));
    glTexCoord2f(u0, v1); glVertex2f((float)screen_x, (float)(screen_y + h));
    glEnd();
    glDisable(GL_TEXTURE_2D);
}

void renderer_draw_tile_border(Renderer *r, int screen_x, int screen_y)
{
    (void)r;
    glColor4f(0.0f, 0.0f, 0.0f, 0.15f);
    glBegin(GL_LINES);
    glVertex2f((float)screen_x, (float)(screen_y + TILE_SIZE));
    glVertex2f((float)(screen_x + TILE_SIZE), (float)(screen_y + TILE_SIZE));
    glVertex2f((float)(screen_x + TILE_SIZE), (float)screen_y);
    glVertex2f((float)(screen_x + TILE_SIZE), (float)(screen_y + TILE_SIZE));
    glEnd();
}

void renderer_draw_text(Renderer *r, const char *text, int x, int y,
                         float scale, float r_col, float g, float b_col) {
    int cx = x;
    for (size_t i = 0; text[i] != '\0'; i++) {
        draw_char(r, text[i], cx, y, scale, r_col, g, b_col);
        cx += (int)((FONT_CHAR_WIDTH + 1) * scale);
    }
}

void renderer_draw_number(Renderer *r, int num, int x, int y,
                           float scale, float r_col, float g, float b_col) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%d", num);
    renderer_draw_text(r, buf, x, y, scale, r_col, g, b_col);
}

int renderer_text_width(Renderer *r, const char *text, float scale) {
    (void)r;
    size_t len = strlen(text);
    if (len == 0) return 0;
    return (int)(len * (FONT_CHAR_WIDTH + 1) * scale - scale);
}

void renderer_toggle_fullscreen(Renderer *r) {
    r->fullscreen = !r->fullscreen;
    if (r->fullscreen) {
        SDL_GetWindowPosition(r->window, &r->windowed_x, &r->windowed_y);
        SDL_GetWindowSize(r->window, &r->windowed_w, &r->windowed_h);
        SDL_SetWindowFullscreen(r->window, SDL_WINDOW_FULLSCREEN_DESKTOP);
    } else {
        SDL_SetWindowFullscreen(r->window, 0);
        SDL_SetWindowPosition(r->window, r->windowed_x, r->windowed_y);
        SDL_SetWindowSize(r->window, r->windowed_w, r->windowed_h);
    }
}

int renderer_is_fullscreen(Renderer *r) {
    return r->fullscreen;
}

void renderer_get_size(Renderer *r, int *w, int *h) {
    SDL_GetWindowSize(r->window, w, h);
}

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
    if (r->atlas_frame_count > 0) {
        glBindTexture(GL_TEXTURE_2D, renderer_current_atlas(r));
    }
}

void renderer_end_tile_batch(Renderer *r) {
    (void)r;
}

void renderer_begin_ui(Renderer *r) {
    renderer_get_size(r, &g_screen_w, &g_screen_h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, g_screen_w, g_screen_h, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glViewport(0, 0, g_screen_w, g_screen_h);
    if (r->atlas_frame_count > 0) {
        glBindTexture(GL_TEXTURE_2D, r->atlas_textures[0]);
    }
}

void renderer_end_ui(Renderer *r) {
    (void)r;
}

unsigned int renderer_load_texture(const unsigned char *data, int width, int height, int channels) {
    unsigned int tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    GLenum fmt = (channels == 4) ? GL_RGBA : GL_RGB;
    glTexImage2D(GL_TEXTURE_2D, 0, fmt, width, height, 0, fmt, GL_UNSIGNED_BYTE, data);
    if (gl_mip_gen) {
        gl_mip_gen(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    } else {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
    return tex;
}

void renderer_generate_atlas(Renderer *r) {
    r->atlas_cols = ATLAS_COLS;
    r->atlas_rows = ATLAS_ROWS;
    r->atlas_frame_count = MAX_ATLAS_FRAMES;
    unsigned char *atlas_buf = (unsigned char *)malloc(ATLAS_SIZE * ATLAS_SIZE * 4);
    if (!atlas_buf) { r->atlas_frame_count = 0; return; }
    for (int f = 0; f < MAX_ATLAS_FRAMES; f++) {
        block_texture_generate_atlas_frame(atlas_buf, f);
        r->atlas_textures[f] = renderer_load_texture(atlas_buf, ATLAS_SIZE, ATLAS_SIZE, 4);
    }
    free(atlas_buf);
}

void renderer_atlas_uv(int sprite_id, float *u0, float *v0, float *u1, float *v1) {
    int col = sprite_id % ATLAS_COLS;
    int row = sprite_id / ATLAS_COLS;
    int x0 = col * ATLAS_SLOT + ATLAS_PAD;
    int y0 = row * ATLAS_SLOT + ATLAS_PAD;
    *u0 = (float)x0 / (float)ATLAS_SIZE;
    *v0 = (float)y0 / (float)ATLAS_SIZE;
    *u1 = *u0 + (float)TILE_TEX_SIZE / (float)ATLAS_SIZE;
    *v1 = *v0 + (float)TILE_TEX_SIZE / (float)ATLAS_SIZE;
}

void renderer_draw_world(Renderer *r, World *w, Camera *cam) {
    float visible_w = g_screen_w / cam->zoom;
    float visible_h = g_screen_h / cam->zoom;
    float cam_left = cam->x - visible_w / 2.0f;
    float cam_top = cam->y - visible_h / 2.0f;
    int start_x = (int)(cam_left / TILE_SIZE) - 1;
    int start_y = (int)(cam_top / TILE_SIZE) - 1;
    int end_x = start_x + (int)(visible_w / TILE_SIZE) + 3;
    int end_y = start_y + (int)(visible_h / TILE_SIZE) + 3;

    if (start_x < 0) start_x = 0;
    if (start_y < 0) start_y = 0;
    if (end_x > w->width) end_x = w->width;
    if (end_y > w->height) end_y = w->height;

    for (int y = start_y; y < end_y; y++) {
        for (int x = start_x; x < end_x; x++) {
            Tile *t = world_get_tile(w, x, y);
            if (!t) continue;

            int sx, sy;
            camera_world_to_screen(cam, x * TILE_SIZE, y * TILE_SIZE, &sx, &sy);

            if (t->bg != BLOCK_AIR) {
                int sprite = block_get_sprite(t->bg);
                renderer_draw_tile(r, sx, sy, sprite, 0);
            }

            if (t->fg != BLOCK_AIR) {
                int sprite = block_get_sprite(t->fg);
                if (t->growth_stage > 0 && t->growth_stage < GROWTH_COMPLETE) {
                    int dirt_sprite = block_get_sprite(BLOCK_DIRT);
                    renderer_draw_tile(r, sx, sy, dirt_sprite, 0);
                    float height_factor = 0.3f + 0.7f * (t->growth_stage / (float)GROWTH_COMPLETE);
                    int draw_h = (int)(TILE_SIZE * height_factor);
                    renderer_draw_tile_scaled(r, sx, sy + TILE_SIZE - draw_h,
                        TILE_SIZE, draw_h, sprite, 0);
                } else if (t->growth_stage >= GROWTH_COMPLETE) {
                    renderer_draw_tile(r, sx, sy, sprite, 0);
                    int leaf_sprite = block_get_sprite(BLOCK_LEAVES);
                    renderer_draw_tile_scaled(r, sx - 4, sy - 12,
                        TILE_SIZE + 8, TILE_SIZE / 2 + 12, leaf_sprite, 0);
                    renderer_draw_tile_border(r, sx, sy);
                } else {
                    renderer_draw_tile(r, sx, sy, sprite, 0);
                    renderer_draw_tile_border(r, sx, sy);
                    if (t->fg == BLOCK_PORTAL) {
                        float pulse = 0.5f + 0.5f * sinf((float)SDL_GetTicks() / 300.0f);
                        renderer_draw_rect(r, sx, sy, TILE_SIZE, TILE_SIZE,
                            0.6f * pulse, 0.2f * pulse, 0.9f * pulse, 0.3f);
                    }
                }
            }
        }
    }

}

void renderer_draw_break_progress(Renderer *r, World *w, Player *p, Camera *cam) {
    if (p->breaking) {
        int bsx, bsy;
        camera_world_to_screen(cam, p->break_x * TILE_SIZE, p->break_y * TILE_SIZE, &bsx, &bsy);
        Tile *bt = world_get_tile(w, p->break_x, p->break_y);
        if (bt) {
            int break_time = block_get_break_time(bt->fg);
            float progress = (break_time > 0) ? (float)p->break_timer / break_time : 0.0f;
            renderer_draw_rect(r, bsx, bsy, TILE_SIZE, TILE_SIZE,
                1.0f, 1.0f, 1.0f, progress * 0.5f);
        }
    }
}

void renderer_draw_player(Renderer *r, Player *p, Camera *cam) {
    int psx, psy;
    camera_world_to_screen(cam, p->x - PLAYER_WIDTH / 2, p->y - PLAYER_HEIGHT, &psx, &psy);
    renderer_draw_rect(r, psx, psy, PLAYER_WIDTH, PLAYER_HEIGHT,
        1.0f, 0.8f, 0.6f, 1.0f);
    if (p->equipped_pants) {
        int cr, cg, cb;
        item_get_color(p->equipped_pants, &cr, &cg, &cb);
        renderer_draw_rect(r, psx, psy + 36, PLAYER_WIDTH, 12,
            cr / 255.0f, cg / 255.0f, cb / 255.0f, 1.0f);
    }
    if (p->equipped_shirt) {
        int cr, cg, cb;
        item_get_color(p->equipped_shirt, &cr, &cg, &cb);
        renderer_draw_rect(r, psx, psy + 20, PLAYER_WIDTH, 16,
            cr / 255.0f, cg / 255.0f, cb / 255.0f, 1.0f);
    }
    if (p->equipped_hat) {
        int cr, cg, cb;
        item_get_color(p->equipped_hat, &cr, &cg, &cb);
        renderer_draw_rect(r, psx, psy, PLAYER_WIDTH, 8,
            cr / 255.0f, cg / 255.0f, cb / 255.0f, 1.0f);
    }
    renderer_draw_rect(r, psx + 4, psy + 4, 6, 6, 0.0f, 0.0f, 0.0f, 1.0f);
    renderer_draw_rect(r, psx + 14, psy + 4, 6, 6, 0.0f, 0.0f, 0.0f, 1.0f);
}
