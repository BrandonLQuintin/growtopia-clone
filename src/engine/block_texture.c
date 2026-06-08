#include "block_texture.h"
#include "prng.h"
#include "../world/block.h"
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

static void tex_door(unsigned char *buf, int size)
{
    Prng p;
    prng_seed(&p, 888);
    px_fill(buf, size, 160, 110, 50, 255);
    for (int y = 0; y < size; y++) {
        px_set(buf, size, 2, y, 130, 85, 35, 255);
        px_set(buf, size, 3, y, 130, 85, 35, 255);
        px_set(buf, size, size - 3, y, 130, 85, 35, 255);
        px_set(buf, size, size - 4, y, 130, 85, 35, 255);
    }
    for (int x = 4; x < size - 4; x++) {
        px_set(buf, size, x, 4, 140, 95, 40, 255);
        px_set(buf, size, x, size / 2, 140, 95, 40, 255);
    }
    px_set(buf, size, size - 7, size / 2 - 2, 200, 180, 60, 255);
    px_set(buf, size, size - 7, size / 2 - 1, 200, 180, 60, 255);
    px_set(buf, size, size - 7, size / 2, 200, 180, 60, 255);
    px_noise(buf, size, &p, 160, 110, 50, 10, 0.15f);
}

static void tex_sign(unsigned char *buf, int size)
{
    Prng p;
    prng_seed(&p, 222);
    px_fill(buf, size, 0, 0, 0, 0);
    for (int y = size / 2; y < size; y++) {
        px_set(buf, size, size / 2 - 1, y, 140, 95, 40, 255);
        px_set(buf, size, size / 2, y, 140, 95, 40, 255);
    }
    for (int y = 2; y < size / 2 - 1; y++) {
        for (int x = 4; x < size - 4; x++) {
            px_set(buf, size, x, y, 220, 200, 160, 255);
        }
    }
    for (int x = 3; x < size - 3; x++) {
        px_set(buf, size, x, 2, 140, 95, 40, 255);
        px_set(buf, size, x, size / 2 - 2, 140, 95, 40, 255);
    }
    for (int y = 2; y < size / 2 - 1; y++) {
        px_set(buf, size, 4, y, 140, 95, 40, 255);
        px_set(buf, size, size - 5, y, 140, 95, 40, 255);
    }
    px_noise(buf, size, &p, 220, 200, 160, 12, 0.2f);
}

static void tex_portal(unsigned char *buf, int size)
{
    Prng p;
    prng_seed(&p, 6666);
    px_fill(buf, size, 140, 50, 200, 255);
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            int dx = x - size / 2;
            int dy = y - size / 2;
            int dist = dx * dx + dy * dy;
            if (dist < 64) {
                px_set(buf, size, x, y, 180, 80, 255, 255);
            } else if (dist < 144) {
                px_set(buf, size, x, y, 160, 60, 230, 255);
            }
        }
    }
    for (int i = 0; i < 5; i++) {
        int cx = prng_range(&p, 4, size - 4);
        int cy = prng_range(&p, 4, size - 4);
        for (int j = 0; j < 6; j++) {
            px_set(buf, size, cx + j, cy, 200, 120, 255, 255);
        }
    }
    px_noise(buf, size, &p, 140, 50, 200, 25, 0.3f);
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
    {8,   tex_door},
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
    {22,  tex_sign},
    {23,  tex_bedrock},
    {24,  tex_wood},
    {25,  tex_wood},
    {26,  tex_portal},
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
