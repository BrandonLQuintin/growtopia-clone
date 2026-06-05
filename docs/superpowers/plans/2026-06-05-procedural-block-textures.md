# Procedural Block Texture Atlas Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace flat colored rectangle blocks with 32x32 procedural pixel-art textures stored in a GPU texture atlas.

**Architecture:** Generate all block textures procedurally at startup into a single 512x512 texture atlas. Each block type has a dedicated draw function using a seeded PRNG for deterministic output. The renderer draws blocks as textured quads sampling from the atlas via UV coordinates computed from sprite IDs.

**Tech Stack:** C11, SDL2, OpenGL 1.x (legacy fixed-function), existing renderer infrastructure

---

### Task 1: Add seeded PRNG utility

**Files:**
- Create: `src/engine/prng.h`
- Create: `src/engine/prng.c`

- [ ] **Step 1: Create prng.h header**

```c
#ifndef PRNG_H
#define PRNG_H

#include <stdint.h>

typedef struct {
    uint32_t state;
} Prng;

void prng_seed(Prng *p, uint32_t seed);
float prng_float(Prng *p);
int prng_range(Prng *p, int min, int max);

#endif
```

- [ ] **Step 2: Create prng.c implementation**

```c
#include "prng.h"

void prng_seed(Prng *p, uint32_t seed)
{
    p->state = seed ? seed : 1;
}

float prng_float(Prng *p)
{
    p->state = p->state * 1664525u + 1013904223u;
    return (float)(p->state >> 1) / (float)0x7FFFFFFFu;
}

int prng_range(Prng *p, int min, int max)
{
    float f = prng_float(p);
    return min + (int)(f * (max - min + 1));
}
```

- [ ] **Step 3: Build to verify compilation**

Run: `make clean && make`
Expected: Compiles with zero warnings. prng.o produced.

- [ ] **Step 4: Commit**

```bash
git add src/engine/prng.h src/engine/prng.c
git commit -m "Add seeded PRNG utility for procedural textures"
```

---

### Task 2: Create block_texture module with pixel buffer helpers and dispatch table

**Files:**
- Create: `src/engine/block_texture.h`
- Create: `src/engine/block_texture.c`

- [ ] **Step 1: Create block_texture.h header**

```c
#ifndef BLOCK_TEXTURE_H
#define BLOCK_TEXTURE_H

#include <stdint.h>

#define ATLAS_SIZE 512
#define ATLAS_COLS 16
#define TILE_TEX_SIZE 32

void block_texture_init(void);
void block_texture_generate(int sprite_id, unsigned char *buffer);
void block_texture_generate_atlas(unsigned char *atlas_buffer);

#endif
```

- [ ] **Step 2: Create block_texture.c with helpers and dispatch table skeleton**

This file defines pixel-level helper functions and a dispatch table mapping sprite IDs to per-type draw functions. Each draw function fills a 32x32 RGBA buffer.

```c
#include "block_texture.h"
#include "prng.h"
#include "world/block.h"
#include <string.h>

static void px_set(unsigned char *buf, int size, int x, int y,
                   unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
    if (x < 0 || x >= size || y < 0 || y >= size) return;
    int idx = (y * size + x) * 4;
    buf[idx + 0] = r;
    buf[idx + 1] = g;
    buf[idx + 2] = b;
    buf[idx + 3] = a;
}

static void px_fill(unsigned char *buf, int size,
                    unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
    for (int i = 0; i < size * size; i++) {
        buf[i * 4 + 0] = r;
        buf[i * 4 + 1] = g;
        buf[i * 4 + 2] = b;
        buf[i * 4 + 3] = a;
    }
}

static void px_noise(unsigned char *buf, int size, Prng *p,
                     unsigned char cr, unsigned char cg, unsigned char cb,
                     int variation, float coverage)
{
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            float f = prng_float(p);
            if (f < coverage) {
                int v = prng_range(p, -variation, variation);
                px_set(buf, size, x, y,
                       (unsigned char)(cr + v > 255 ? 255 : cr + v < 0 ? 0 : cr + v),
                       (unsigned char)(cg + v > 255 ? 255 : cg + v < 0 ? 0 : cg + v),
                       (unsigned char)(cb + v > 255 ? 255 : cb + v < 0 ? 0 : cb + v),
                       255);
            }
        }
    }
}

static void tex_dirt(unsigned char *buf, int size)
{
    Prng p;
    prng_seed(&p, 42);
    px_fill(buf, size, 139, 90, 43, 255);
    px_noise(buf, size, &p, 139, 90, 43, 25, 0.5f);
    for (int i = 0; i < 5; i++) {
        int bx = prng_range(&p, 4, 24);
        int by = prng_range(&p, 4, 24);
        px_set(buf, size, bx, by, 110, 70, 30, 255);
        px_set(buf, size, bx + 1, by, 100, 60, 25, 255);
    }
    for (int i = 0; i < 3; i++) {
        int rx = prng_range(&p, 6, 24);
        int ry = prng_range(&p, 14, 28);
        px_set(buf, size, rx, ry, 80, 50, 20, 255);
    }
}

static void tex_stone(unsigned char *buf, int size)
{
    Prng p;
    prng_seed(&p, 99);
    px_fill(buf, size, 128, 128, 128, 255);
    px_noise(buf, size, &p, 128, 128, 128, 22, 0.4f);
    for (int i = 0; i < 4; i++) {
        int cx = prng_range(&p, 4, 22);
        int cy = prng_range(&p, 4, 22);
        int len = prng_range(&p, 3, 8);
        for (int j = 0; j < len; j++)
            px_set(buf, size, cx + j, cy + (j % 2), 75, 75, 75, 255);
    }
    for (int i = 0; i < 3; i++) {
        int bx = prng_range(&p, 2, 26);
        int by = prng_range(&p, 2, 26);
        px_set(buf, size, bx, by, 155, 155, 155, 255);
        px_set(buf, size, bx + 1, by, 150, 150, 150, 255);
        px_set(buf, size, bx, by + 1, 150, 150, 150, 255);
    }
}

static void tex_grass(unsigned char *buf, int size)
{
    Prng p;
    prng_seed(&p, 7);
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            if (y < size / 4) {
                int g = 150 + prng_range(&p, -20, 15);
                px_set(buf, size, x, y, 50, (unsigned char)g, 50, 255);
            } else if (y < size * 11 / 32) {
                if (prng_float(&p) < 0.4f)
                    px_set(buf, size, x, y, 50, 150, 50, 255);
                else
                    px_set(buf, size, x, y, 139, 90, 43, 255);
            } else {
                int v = prng_range(&p, -15, 10);
                px_set(buf, size, x, y,
                       (unsigned char)(139 + v), (unsigned char)(90 + v), (unsigned char)(43 + v), 255);
            }
        }
    }
    for (int i = 0; i < 6; i++) {
        int bx = prng_range(&p, 0, 30);
        px_set(buf, size, bx, 0, 30, 130, 30, 255);
        px_set(buf, size, bx, 1, 35, 140, 35, 255);
    }
    px_set(buf, size, 15, 0, 40, 160, 40, 255);
    px_set(buf, size, 16, 0, 35, 150, 35, 255);
}

static void tex_brick(unsigned char *buf, int size)
{
    px_fill(buf, size, 180, 80, 50, 255);
    int bh = size / 4;
    for (int row = 0; row < 4; row++) {
        int by = row * bh;
        for (int x = 0; x < size; x++) {
            px_set(buf, size, x, by, 210, 200, 180, 255);
            px_set(buf, size, x, by + bh - 1, 210, 200, 180, 255);
        }
        int off = (row % 2) * (size / 2);
        for (int y = by; y < by + bh; y++) {
            px_set(buf, size, off, y, 210, 200, 180, 255);
            px_set(buf, size, off + size / 2, y, 210, 200, 180, 255);
        }
    }
    Prng p;
    prng_seed(&p, 500);
    for (int row = 0; row < 4; row++) {
        int by = row * bh + 1;
        int off = (row % 2) * (size / 2);
        for (int bx = off + 1; bx < off + size / 2 - 1 && bx < size - 1; bx++) {
            for (int yy = by; yy < by + bh - 2; yy++) {
                float f = prng_float(&p);
                if (f < 0.08f)
                    px_set(buf, size, bx, yy, 170, 70, 40, 255);
                else if (f < 0.14f)
                    px_set(buf, size, bx, yy, 195, 95, 60, 255);
            }
        }
    }
}

static void tex_wood(unsigned char *buf, int size)
{
    Prng p;
    prng_seed(&p, 55);
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            int cr = 160, cg = 110, cb = 50;
            if (y % 6 == 0 || (y + 3) % 6 == 0) { cr -= 25; cg -= 20; cb -= 10; }
            float f = prng_float(&p);
            if (f < 0.06f) { cr += 10; cg += 8; cb += 5; }
            else if (f < 0.1f) { cr -= 12; cg -= 8; cb -= 5; }
            px_set(buf, size, x, y, (unsigned char)cr, (unsigned char)cg, (unsigned char)cb, 255);
        }
    }
    for (int dy = -3; dy <= 3; dy++) {
        for (int dx = -3; dx <= 3; dx++) {
            if (dx * dx + dy * dy <= 9) {
                int d = dx * dx + dy * dy;
                if (d > 4)
                    px_set(buf, size, 16 + dx, 18 + dy, 130, 85, 35, 255);
                else
                    px_set(buf, size, 16 + dx, 18 + dy, 110, 70, 28, 255);
            }
        }
    }
}

static void tex_sand(unsigned char *buf, int size)
{
    Prng p;
    prng_seed(&p, 33);
    px_fill(buf, size, 210, 190, 130, 255);
    px_noise(buf, size, &p, 210, 190, 130, 15, 0.3f);
    for (int i = 0; i < 8; i++) {
        int sx = prng_range(&p, 2, 28);
        int sy = prng_range(&p, 2, 28);
        px_set(buf, size, sx, sy, 190, 170, 115, 255);
    }
    for (int x = 0; x < size; x++) {
        if (x % 7 < 4) px_set(buf, size, x, 14, 195, 175, 120, 255);
        if ((x + 3) % 9 < 5) px_set(buf, size, x, 24, 200, 180, 125, 255);
    }
}

static void tex_glass(unsigned char *buf, int size)
{
    px_fill(buf, size, 180, 220, 255, 255);
    for (int i = 0; i < size; i++) {
        px_set(buf, size, 0, i, 160, 200, 240, 255);
        px_set(buf, size, size - 1, i, 160, 200, 240, 255);
        px_set(buf, size, i, 0, 160, 200, 240, 255);
        px_set(buf, size, i, size - 1, 160, 200, 240, 255);
    }
    for (int i = 2; i < 20 && i + 4 < size; i++) {
        px_set(buf, size, i + 4, i, 255, 255, 255, 255);
        if (i < 14)
            px_set(buf, size, i + 6, i + 1, 230, 245, 255, 255);
    }
    px_set(buf, size, 24, 6, 255, 255, 255, 255);
    px_set(buf, size, 25, 7, 240, 250, 255, 255);
}

static void tex_bedrock(unsigned char *buf, int size)
{
    Prng p;
    prng_seed(&p, 666);
    px_fill(buf, size, 40, 40, 42, 255);
    px_noise(buf, size, &p, 40, 40, 42, 12, 0.3f);
    for (int i = 0; i < 3; i++) {
        int cx = prng_range(&p, 5, 22);
        int cy = prng_range(&p, 8, 22);
        for (int j = 0; j < 6; j++)
            px_set(buf, size, cx + j, cy + (j % 2), 28, 28, 30, 255);
    }
}

static void tex_water(unsigned char *buf, int size)
{
    Prng p;
    prng_seed(&p, 700);
    px_fill(buf, size, 50, 100, 200, 255);
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            int wave = (x + y * 2) % 8;
            if (wave < 2) {
                px_set(buf, size, x, y, 70, 130, 220, 255);
            } else if (wave < 4) {
                px_set(buf, size, x, y, 40, 80, 180, 255);
            }
        }
    }
    px_noise(buf, size, &p, 50, 100, 200, 15, 0.2f);
}

static void tex_lava(unsigned char *buf, int size)
{
    Prng p;
    prng_seed(&p, 800);
    px_fill(buf, size, 220, 80, 20, 255);
    px_noise(buf, size, &p, 220, 80, 20, 30, 0.4f);
    for (int i = 0; i < 6; i++) {
        int cx = prng_range(&p, 2, 20);
        int cy = prng_range(&p, 2, 20);
        int len = prng_range(&p, 4, 10);
        for (int j = 0; j < len; j++) {
            px_set(buf, size, cx + j, cy + (j % 3), 255, 200, 50, 255);
            if (j + 1 < len)
                px_set(buf, size, cx + j + 1, cy + (j % 3), 255, 160, 30, 255);
        }
    }
}

static void tex_ice(unsigned char *buf, int size)
{
    px_fill(buf, size, 180, 220, 250, 255);
    Prng p;
    prng_seed(&p, 900);
    px_noise(buf, size, &p, 180, 220, 250, 15, 0.25f);
    for (int i = 0; i < 3; i++) {
        int cx = prng_range(&p, 4, 20);
        int cy = prng_range(&p, 4, 20);
        int len = prng_range(&p, 3, 8);
        for (int j = 0; j < len; j++)
            px_set(buf, size, cx + j, cy + (j % 2), 150, 200, 240, 255);
    }
    for (int i = 2; i < 16 && i + 4 < size; i++) {
        px_set(buf, size, i + 4, i, 230, 245, 255, 255);
    }
}

static void tex_snow(unsigned char *buf, int size)
{
    Prng p;
    prng_seed(&p, 1000);
    px_fill(buf, size, 240, 245, 255, 255);
    px_noise(buf, size, &p, 240, 245, 255, 10, 0.3f);
    for (int i = 0; i < 8; i++) {
        int sx = prng_range(&p, 2, 28);
        int sy = prng_range(&p, 2, 28);
        px_set(buf, size, sx, sy, 220, 230, 248, 255);
    }
}

static void tex_leaves(unsigned char *buf, int size)
{
    Prng p;
    prng_seed(&p, 444);
    px_fill(buf, size, 30, 130, 30, 255);
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            int g = 130 + prng_range(&p, -30, 25);
            px_set(buf, size, x, y, 30, (unsigned char)g, 30, 255);
        }
    }
}

static void tex_dirt_bg(unsigned char *buf, int size)
{
    Prng p;
    prng_seed(&p, 1042);
    px_fill(buf, size, 160, 110, 65, 255);
    px_noise(buf, size, &p, 160, 110, 65, 15, 0.4f);
}

static void tex_stone_bg(unsigned char *buf, int size)
{
    Prng p;
    prng_seed(&p, 1099);
    px_fill(buf, size, 150, 150, 150, 255);
    px_noise(buf, size, &p, 150, 150, 150, 15, 0.35f);
    for (int i = 0; i < 2; i++) {
        int cx = prng_range(&p, 4, 22);
        int cy = prng_range(&p, 4, 22);
        int len = prng_range(&p, 3, 6);
        for (int j = 0; j < len; j++)
            px_set(buf, size, cx + j, cy + (j % 2), 120, 120, 120, 255);
    }
}

static void tex_grass_bg(unsigned char *buf, int size)
{
    Prng p;
    prng_seed(&p, 1007);
    px_fill(buf, size, 70, 170, 70, 255);
    px_noise(buf, size, &p, 70, 170, 70, 18, 0.4f);
}

static void tex_wood_bg(unsigned char *buf, int size)
{
    Prng p;
    prng_seed(&p, 1055);
    px_fill(buf, size, 190, 150, 90, 255);
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            if (y % 6 == 0) px_set(buf, size, x, y, 170, 130, 75, 255);
        }
    }
    px_noise(buf, size, &p, 190, 150, 90, 10, 0.2f);
}

static void tex_brick_bg(unsigned char *buf, int size)
{
    px_fill(buf, size, 200, 110, 80, 255);
    int bh = size / 4;
    for (int row = 0; row < 4; row++) {
        int by = row * bh;
        for (int x = 0; x < size; x++) {
            px_set(buf, size, x, by, 220, 210, 190, 255);
            px_set(buf, size, x, by + bh - 1, 220, 210, 190, 255);
        }
        int off = (row % 2) * (size / 2);
        for (int y = by; y < by + bh; y++) {
            px_set(buf, size, off, y, 220, 210, 190, 255);
            px_set(buf, size, off + size / 2, y, 220, 210, 190, 255);
        }
    }
}

static void tex_cloth_bg(unsigned char *buf, int size)
{
    Prng p;
    prng_seed(&p, 1100);
    px_fill(buf, size, 220, 200, 180, 255);
    px_noise(buf, size, &p, 220, 200, 180, 10, 0.25f);
    for (int y = 0; y < size; y += 4) {
        for (int x = 0; x < size; x++) {
            px_set(buf, size, x, y, 210, 190, 170, 255);
        }
    }
}

static void tex_default(unsigned char *buf, int size)
{
    px_fill(buf, size, 255, 0, 255, 255);
}

typedef void (*tex_fn)(unsigned char *, int);

static const struct {
    int sprite_id;
    tex_fn fn;
} tex_dispatch[] = {
    {1,   tex_dirt},
    {2,   tex_stone},
    {3,   tex_grass},
    {4,   tex_bedrock},
    {5,   tex_wood},
    {6,   tex_wood_bg},
    {7,   tex_leaves},
    {8,   tex_wood},
    {9,   tex_brick},
    {10,  tex_glass},
    {11,  tex_sand},
    {12,  tex_water},
    {13,  tex_lava},
    {14,  tex_cloth_bg},
    {15,  tex_stone},
    {16,  tex_stone},
    {17,  tex_dirt},
    {18,  tex_stone},
    {19,  tex_stone},
    {20,  tex_ice},
    {21,  tex_snow},
    {22,  tex_wood},
    {23,  tex_bedrock},
    {24,  tex_wood},
    {25,  tex_wood},
    {26,  tex_stone},
    {27,  tex_wood},
    {129, tex_dirt_bg},
    {130, tex_stone_bg},
    {131, tex_grass_bg},
    {133, tex_wood_bg},
    {137, tex_brick_bg},
    {142, tex_cloth_bg},
    {257, tex_dirt},
    {258, tex_stone},
    {259, tex_grass},
    {261, tex_wood},
    {263, tex_leaves},
    {265, tex_brick},
    {267, tex_sand},
    {271, tex_stone},
    {272, tex_stone},
    {273, tex_dirt},
    {274, tex_stone},
    {276, tex_ice},
    {277, tex_snow},
    {284, tex_grass},
    {285, tex_grass},
    {286, tex_dirt},
    {287, tex_leaves},
};
#define TEX_DISPATCH_COUNT (sizeof(tex_dispatch) / sizeof(tex_dispatch[0]))

void block_texture_generate(int sprite_id, unsigned char *buffer)
{
    tex_default(buffer, TILE_TEX_SIZE);
    for (int i = 0; i < (int)TEX_DISPATCH_COUNT; i++) {
        if (tex_dispatch[i].sprite_id == sprite_id) {
            tex_dispatch[i].fn(buffer, TILE_TEX_SIZE);
            return;
        }
    }
}

void block_texture_generate_atlas(unsigned char *atlas_buffer)
{
    memset(atlas_buffer, 0, ATLAS_SIZE * ATLAS_SIZE * 4);
    for (int i = 0; i < BLOCK_DEF_COUNT; i++) {
        int sid = BLOCK_DEFS[i].tile_sprite_id;
        int col = sid % ATLAS_COLS;
        int row = sid / ATLAS_COLS;
        int ox = col * TILE_TEX_SIZE;
        int oy = row * TILE_TEX_SIZE;
        unsigned char tile_buf[TILE_TEX_SIZE * TILE_TEX_SIZE * 4];
        block_texture_generate(sid, tile_buf);
        for (int ty = 0; ty < TILE_TEX_SIZE; ty++) {
            for (int tx = 0; tx < TILE_TEX_SIZE; tx++) {
                int si = (ty * TILE_TEX_SIZE + tx) * 4;
                int di = ((oy + ty) * ATLAS_SIZE + (ox + tx)) * 4;
                atlas_buffer[di + 0] = tile_buf[si + 0];
                atlas_buffer[di + 1] = tile_buf[si + 1];
                atlas_buffer[di + 2] = tile_buf[si + 2];
                atlas_buffer[di + 3] = tile_buf[si + 3];
            }
        }
    }
}
```

- [ ] **Step 3: Build to verify compilation**

Run: `make clean && make`
Expected: Compiles with zero warnings.

- [ ] **Step 4: Commit**

```bash
git add src/engine/block_texture.h src/engine/block_texture.c
git commit -m "Add block texture generation with procedural patterns"
```

---

### Task 3: Implement atlas generation in renderer

**Files:**
- Modify: `src/engine/renderer.c` — replace `renderer_generate_atlas()` stub
- Modify: `src/engine/renderer.h` — add atlas_uv function declaration

- [ ] **Step 1: Add include for block_texture.h in renderer.c**

Add `#include "block_texture.h"` after the existing includes at the top of `src/engine/renderer.c`.

- [ ] **Step 2: Implement renderer_generate_atlas() in renderer.c**

Replace the existing stub:
```c
void renderer_generate_atlas(Renderer *r)
{
    (void)r;
}
```

With:
```c
void renderer_generate_atlas(Renderer *r)
{
    r->atlas_cols = ATLAS_COLS;
    unsigned char *atlas_buf = (unsigned char *)malloc(ATLAS_SIZE * ATLAS_SIZE * 4);
    if (!atlas_buf) return;
    block_texture_generate_atlas(atlas_buf);
    r->atlas_texture = renderer_load_texture(atlas_buf, ATLAS_SIZE, ATLAS_SIZE, 4);
    free(atlas_buf);
}
```

- [ ] **Step 3: Add atlas_uv helper declaration in renderer.h**

Add before the `#endif`:
```c
void renderer_atlas_uv(int sprite_id, float *u0, float *v0, float *u1, float *v1);
```

- [ ] **Step 4: Add atlas_uv implementation in renderer.c**

Add the function:
```c
void renderer_atlas_uv(int sprite_id, float *u0, float *v0, float *u1, float *v1)
{
    int col = sprite_id % ATLAS_COLS;
    int row = sprite_id / ATLAS_COLS;
    *u0 = (float)(col * TILE_TEX_SIZE) / (float)ATLAS_SIZE;
    *v0 = (float)(row * TILE_TEX_SIZE) / (float)ATLAS_SIZE;
    *u1 = *u0 + (float)TILE_TEX_SIZE / (float)ATLAS_SIZE;
    *v1 = *v0 + (float)TILE_TEX_SIZE / (float)ATLAS_SIZE;
}
```

- [ ] **Step 5: Add freeglut header-free stdlib include if needed for malloc**

The file already includes `<stdlib.h>`, so no change needed.

- [ ] **Step 6: Build to verify compilation**

Run: `make clean && make`
Expected: Compiles with zero warnings.

- [ ] **Step 7: Commit**

```bash
git add src/engine/renderer.c src/engine/renderer.h
git commit -m "Implement atlas generation in renderer"
```

---

### Task 4: Switch renderer_draw_tile to textured quads

**Files:**
- Modify: `src/engine/renderer.c` — rewrite `renderer_draw_tile()` and `renderer_draw_tile_scaled()`

- [ ] **Step 1: Rewrite renderer_draw_tile()**

Replace the existing function:
```c
void renderer_draw_tile(Renderer *r, int screen_x, int screen_y, int tile_id, int frame)
{
    (void)frame;
    float cr, cg, cb;
    get_tile_color(tile_id, &cr, &cg, &cb);
    renderer_draw_rect(r, screen_x, screen_y, TILE_SIZE, TILE_SIZE, cr, cg, cb, 1.0f);
}
```

With:
```c
void renderer_draw_tile(Renderer *r, int screen_x, int screen_y, int tile_id, int frame)
{
    (void)frame;
    if (r->atlas_texture == 0) {
        float cr, cg, cb;
        get_tile_color(tile_id, &cr, &cg, &cb);
        renderer_draw_rect(r, screen_x, screen_y, TILE_SIZE, TILE_SIZE, cr, cg, cb, 1.0f);
        return;
    }
    float u0, v0, u1, v1;
    renderer_atlas_uv(tile_id, &u0, &v0, &u1, &v1);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, r->atlas_texture);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glBegin(GL_QUADS);
    glTexCoord2f(u0, v0); glVertex2f((float)screen_x, (float)screen_y);
    glTexCoord2f(u1, v0); glVertex2f((float)(screen_x + TILE_SIZE), (float)screen_y);
    glTexCoord2f(u1, v1); glVertex2f((float)(screen_x + TILE_SIZE), (float)(screen_y + TILE_SIZE));
    glTexCoord2f(u0, v1); glVertex2f((float)screen_x, (float)(screen_y + TILE_SIZE));
    glEnd();
    glDisable(GL_TEXTURE_2D);
}
```

Note: `tile_id` here is actually a sprite ID passed from the caller. The parameter name stays `tile_id` to match the existing declaration in renderer.h.

- [ ] **Step 2: Rewrite renderer_draw_tile_scaled()**

Replace the existing function:
```c
void renderer_draw_tile_scaled(Renderer *r, int screen_x, int screen_y, int w, int h,
                                int tile_id, int frame)
{
    (void)frame;
    float cr, cg, cb;
    get_tile_color(tile_id, &cr, &cg, &cb);
    renderer_draw_rect(r, screen_x, screen_y, w, h, cr, cg, cb, 1.0f);
}
```

With:
```c
void renderer_draw_tile_scaled(Renderer *r, int screen_x, int screen_y, int w, int h,
                                int tile_id, int frame)
{
    (void)frame;
    if (r->atlas_texture == 0) {
        float cr, cg, cb;
        get_tile_color(tile_id, &cr, &cg, &cb);
        renderer_draw_rect(r, screen_x, screen_y, w, h, cr, cg, cb, 1.0f);
        return;
    }
    float u0, v0, u1, v1;
    renderer_atlas_uv(tile_id, &u0, &v0, &u1, &v1);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, r->atlas_texture);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glBegin(GL_QUADS);
    glTexCoord2f(u0, v0); glVertex2f((float)screen_x, (float)screen_y);
    glTexCoord2f(u1, v0); glVertex2f((float)(screen_x + w), (float)screen_y);
    glTexCoord2f(u1, v1); glVertex2f((float)(screen_x + w), (float)(screen_y + h));
    glTexCoord2f(u0, v1); glVertex2f((float)screen_x, (float)(screen_y + h));
    glEnd();
    glDisable(GL_TEXTURE_2D);
}
```

- [ ] **Step 3: Build to verify compilation**

Run: `make clean && make`
Expected: Compiles with zero warnings.

- [ ] **Step 4: Commit**

```bash
git add src/engine/renderer.c
git commit -m "Switch draw_tile to textured quads with atlas UVs"
```

---

### Task 5: Update main.c tile rendering to use sprite IDs and call atlas generation

**Files:**
- Modify: `src/main.c` — update `game_init()` and `game_render()`

- [ ] **Step 1: Add atlas generation call in game_init()**

After `camera_init(...)` and the camera setup lines (around line 110), add:

```c
    renderer_generate_atlas(&g->renderer);
```

- [ ] **Step 2: Update background tile rendering in game_render()**

In the tile rendering loop (around line 437-442), replace the background drawing:

From:
```c
            if (t->bg != BLOCK_AIR) {
                int br, bg, bb;
                block_get_color(t->bg, &br, &bg, &bb);
                renderer_draw_rect(&g->renderer, sx, sy, TILE_SIZE, TILE_SIZE,
                    br / 255.0f * 0.6f, bg / 255.0f * 0.6f, bb / 255.0f * 0.6f, 1.0f);
            }
```

To:
```c
            if (t->bg != BLOCK_AIR) {
                int sprite = block_get_sprite(t->bg);
                renderer_draw_tile(&g->renderer, sx, sy, sprite, 0);
            }
```

- [ ] **Step 3: Update foreground tile rendering in game_render()**

Replace the entire foreground block (the `if (t->fg != BLOCK_AIR)` section, approximately lines 444-467):

From:
```c
            if (t->fg != BLOCK_AIR) {
                if (t->growth_stage > 0 && t->growth_stage < GROWTH_COMPLETE) {
                    int r, gr, b;
                    block_get_color(t->fg, &r, &gr, &b);
                    float height_factor = 0.3f + 0.7f * (t->growth_stage / (float)GROWTH_COMPLETE);
                    int draw_h = (int)(TILE_SIZE * height_factor);
                    renderer_draw_rect(&g->renderer, sx, sy + TILE_SIZE - draw_h, TILE_SIZE, draw_h,
                        r / 255.0f, gr / 255.0f, b / 255.0f, 1.0f);
                } else if (t->growth_stage >= GROWTH_COMPLETE) {
                    int r, gr, b;
                    block_get_color(t->fg, &r, &gr, &b);
                    renderer_draw_rect(&g->renderer, sx, sy, TILE_SIZE, TILE_SIZE,
                        r / 255.0f, gr / 255.0f, b / 255.0f, 1.0f);
                    int lr, lg, lb;
                    block_get_color(BLOCK_LEAVES, &lr, &lg, &lb);
                    renderer_draw_rect(&g->renderer, sx - 4, sy - 12, TILE_SIZE + 8, TILE_SIZE / 2 + 12,
                        lr / 255.0f, lg / 255.0f, lb / 255.0f, 0.9f);
                } else {
                    int r, gr, b;
                    block_get_color(t->fg, &r, &gr, &b);
                    renderer_draw_rect(&g->renderer, sx, sy, TILE_SIZE, TILE_SIZE,
                        r / 255.0f, gr / 255.0f, b / 255.0f, 1.0f);
                }
            }
```

To:
```c
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
                } else {
                    renderer_draw_tile(&g->renderer, sx, sy, sprite, 0);
                }
            }
```

- [ ] **Step 4: Build to verify compilation**

Run: `make clean && make`
Expected: Compiles with zero warnings.

- [ ] **Step 5: Commit**

```bash
git add src/main.c
git commit -m "Switch tile rendering to use sprite IDs and atlas textures"
```

---

### Task 6: Add tile border rendering for grid separation

**Files:**
- Modify: `src/engine/renderer.c` — add `renderer_draw_tile_border()` function
- Modify: `src/engine/renderer.h` — add declaration

- [ ] **Step 1: Add declaration in renderer.h**

Add before `#endif`:
```c
void renderer_draw_tile_border(Renderer *r, int screen_x, int screen_y);
```

- [ ] **Step 2: Add implementation in renderer.c**

```c
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
```

- [ ] **Step 3: Add border calls in main.c render loop**

In `game_render()`, after each `renderer_draw_tile()` call for foreground tiles (inside the `if (t->fg != BLOCK_AIR)` block), add `renderer_draw_tile_border(&g->renderer, sx, sy);` after each foreground tile is drawn. Place it right before the closing brace of each growth stage branch.

The result should be:

```c
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
```

Note: Border is only drawn on fully-grown and normal blocks, not on growing plants (they're partial height).

- [ ] **Step 4: Build to verify compilation**

Run: `make clean && make`
Expected: Compiles with zero warnings.

- [ ] **Step 5: Commit**

```bash
git add src/engine/renderer.h src/engine/renderer.c src/main.c
git commit -m "Add subtle tile borders for grid separation"
```

---

### Task 7: Update UI inventory/store slots to use textured tiles

**Files:**
- Modify: `src/engine/ui.c` — update `render_slot_item()` to use atlas textures

- [ ] **Step 1: Add include for block.h in ui.c**

Add at the top of `ui.c`:
```c
#include "world/block.h"
```

- [ ] **Step 2: Update render_slot_item() to use atlas textures**

Replace the existing `render_slot_item` function (lines 160-175):

From:
```c
static void render_slot_item(Renderer *r, int x, int y, int size, int item_id, int count)
{
    if (item_id == 0) return;
    int cr, cg, cb;
    item_get_color((uint16_t)item_id, &cr, &cg, &cb);
    int pad = 6;
    renderer_draw_rect(r, x + pad, y + pad, size - pad * 2, size - pad * 2,
        cr / 255.0f, cg / 255.0f, cb / 255.0f, 1.0f);
    if (count > 0) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%d", count);
        int tw = renderer_text_width(r, buf, 1.0f);
        renderer_draw_rect(r, x + size - tw - 6, y + size - 12, tw + 4, 11, 0.0f, 0.0f, 0.0f, 0.6f);
        renderer_draw_text(r, buf, x + size - tw - 4, y + size - 11, 1.0f, 1.0f, 1.0f, 1.0f);
    }
}
```

To:
```c
static void render_slot_item(Renderer *r, int x, int y, int size, int item_id, int count)
{
    if (item_id == 0) return;
    int sprite = block_get_sprite((uint16_t)item_id);
    if (sprite > 0) {
        int pad = 6;
        renderer_draw_tile_scaled(r, x + pad, y + pad, size - pad * 2, size - pad * 2, sprite, 0);
    } else {
        int cr, cg, cb;
        item_get_color((uint16_t)item_id, &cr, &cg, &cb);
        int pad = 6;
        renderer_draw_rect(r, x + pad, y + pad, size - pad * 2, size - pad * 2,
            cr / 255.0f, cg / 255.0f, cb / 255.0f, 1.0f);
    }
    if (count > 0) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%d", count);
        int tw = renderer_text_width(r, buf, 1.0f);
        renderer_draw_rect(r, x + size - tw - 6, y + size - 12, tw + 4, 11, 0.0f, 0.0f, 0.0f, 0.6f);
        renderer_draw_text(r, buf, x + size - tw - 4, y + size - 11, 1.0f, 1.0f, 1.0f, 1.0f);
    }
}
```

- [ ] **Step 3: Build to verify compilation**

Run: `make clean && make`
Expected: Compiles with zero warnings.

- [ ] **Step 4: Commit**

```bash
git add src/engine/ui.c
git commit -m "Update UI slots to render items with atlas textures"
```

---

### Task 8: Build, run, and visually verify

**Files:** None (verification only)

- [ ] **Step 1: Clean build**

Run: `make clean && make`
Expected: Zero warnings, zero errors.

- [ ] **Step 2: Run the game**

Run: `./growtopia`
Expected: Game window opens showing textured blocks instead of flat colors. Dirt has noise/speckles, stone has cracks, grass has blades on top, brick has mortar lines, wood has grain, etc. Background tiles render with their textures. Inventory slots show scaled-down textures. Growing plants show dirt base with partial plant overlay. Tile borders visible as subtle dark lines.

- [ ] **Step 3: Verify all UI still works**

- Press E to open inventory — items should render with small textured icons
- Press B to open store — items should render with textures
- Walk around — camera/collision should be unchanged
- Place/break blocks — should work as before
- Break a growing plant — should work as before

- [ ] **Step 4: Final commit if any fixups needed**

```bash
git add -A
git commit -m "Fix visual issues from texture atlas integration"
```
