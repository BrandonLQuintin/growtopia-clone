#include "renderer.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <GL/gl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

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
                                  SCREEN_WIDTH, SCREEN_HEIGHT,
                                  SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
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

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_TEXTURE_2D);

    setup_ortho(SCREEN_WIDTH, SCREEN_HEIGHT);

    return 0;
}

void renderer_shutdown(Renderer *r) {
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

void renderer_draw_tile(Renderer *r, int screen_x, int screen_y, int tile_id, int frame) {
    (void)frame;
    float cr, cg, cb;
    get_tile_color(tile_id, &cr, &cg, &cb);
    renderer_draw_rect(r, screen_x, screen_y, TILE_SIZE, TILE_SIZE, cr, cg, cb, 1.0f);
}

void renderer_draw_tile_scaled(Renderer *r, int screen_x, int screen_y, int w, int h,
                                int tile_id, int frame) {
    (void)frame;
    float cr, cg, cb;
    get_tile_color(tile_id, &cr, &cg, &cb);
    renderer_draw_rect(r, screen_x, screen_y, w, h, cr, cg, cb, 1.0f);
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

void renderer_begin_tile_batch(Renderer *r) {
    (void)r;
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void renderer_end_tile_batch(Renderer *r) {
    (void)r;
}

void renderer_begin_ui(Renderer *r) {
    (void)r;
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
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
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
    return tex;
}

void renderer_generate_atlas(Renderer *r) {
    (void)r;
}
