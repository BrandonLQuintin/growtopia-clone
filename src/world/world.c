#include "world.h"
#include "block.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

int world_init(World *w, int width, int height) {
    w->tiles = calloc((size_t)width * height, sizeof(Tile));
    if (!w->tiles) return -1;
    w->width = width;
    w->height = height;
    memset(w->name, 0, sizeof(w->name));
    return 0;
}

void world_free(World *w) {
    free(w->tiles);
    w->tiles = NULL;
    w->width = 0;
    w->height = 0;
}

void world_generate(World *w) {
    srand((unsigned)time(NULL));

    for (int x = 0; x < w->width; x++) {
        for (int y = 0; y < w->height; y++) {
            Tile *t = &w->tiles[y * w->width + x];
            t->fg = BLOCK_AIR;
            t->bg = BLOCK_AIR;
            t->growth_stage = 0;
            t->growth_timer = 0;
            t->extra_data = 0;

            if (y >= 27 && y <= 57) {
                t->bg = BG_DIRT;
            } else if (y >= 58) {
                t->bg = BG_STONE;
            }

            if (y == 26) {
                t->fg = BLOCK_GRASS;
                t->bg = BG_DIRT;
            } else if (y >= 27 && y <= 45) {
                t->fg = BLOCK_DIRT;
                if (rand() % 100 < 15) {
                    t->fg = BLOCK_STONE;
                }
            } else if (y >= 46 && y <= 57) {
                t->fg = BLOCK_STONE;
                if (rand() % 100 < 10) {
                    t->fg = BLOCK_DIRT;
                }
            } else if (y >= 58) {
                t->fg = BLOCK_BEDROCK;
            }
        }
    }

    int num_caves = 30 + rand() % 25;
    for (int c = 0; c < num_caves; c++) {
        int cx = rand() % w->width;
        int cy = 28 + rand() % 29;
        int radius = 2 + rand() % 4;
        for (int dy = -radius; dy <= radius; dy++) {
            for (int dx = -radius; dx <= radius; dx++) {
                if (dx * dx + dy * dy <= radius * radius) {
                    int tx = cx + dx;
                    int ty = cy + dy;
                    if (tx >= 0 && tx < w->width && ty >= 27 && ty < w->height - 2) {
                        Tile *t = &w->tiles[ty * w->width + tx];
                        if (t->fg != BLOCK_BEDROCK) {
                            t->fg = BLOCK_AIR;
                        }
                    }
                }
            }
        }
    }

    for (int x = 2; x < w->width - 2;) {
        if (rand() % 100 < 8) {
            Tile *surface = &w->tiles[26 * w->width + x];
            if (surface->fg == BLOCK_GRASS) {
                int trunk_h = 4 + rand() % 3;
                for (int ty = 0; ty < trunk_h; ty++) {
                    int y = 25 - ty;
                    if (y >= 0) {
                        w->tiles[y * w->width + x].fg = BLOCK_WOOD;
                    }
                }
                int top_y = 25 - trunk_h;
                if (top_y >= 2 && top_y < 26) {
                    for (int ly = -2; ly <= 0; ly++) {
                        for (int lx = -2; lx <= 2; lx++) {
                            int px = x + lx;
                            int py = top_y + ly;
                            if (px >= 0 && px < w->width && py >= 0 && py < 26) {
                                Tile *t = &w->tiles[py * w->width + px];
                                if (t->fg == BLOCK_AIR) {
                                    int dist = abs(lx) + abs(ly);
                                    if (dist <= 2) {
                                        t->fg = BLOCK_LEAVES;
                                    }
                                }
                            }
                        }
                    }
                }
                x += 5 + rand() % 4;
                continue;
            }
        }
        x++;
    }
}

Tile *world_get_tile(World *w, int x, int y) {
    if (x < 0 || x >= w->width || y < 0 || y >= w->height) return NULL;
    return &w->tiles[y * w->width + x];
}

int world_set_fg(World *w, int x, int y, uint16_t block_id) {
    Tile *t = world_get_tile(w, x, y);
    if (!t) return -1;
    t->fg = block_id;
    return 0;
}

int world_set_bg(World *w, int x, int y, uint16_t block_id) {
    Tile *t = world_get_tile(w, x, y);
    if (!t) return -1;
    t->bg = block_id;
    return 0;
}

int world_save(World *w, const char *path) {
    FILE *f = fopen(path, "wb");
    if (!f) return -1;

    const char magic[4] = {'G', 'R', 'O', 'W'};
    uint32_t version = 1;

    if (fwrite(magic, 1, 4, f) != 4) { fclose(f); return -1; }
    if (fwrite(&version, sizeof(uint32_t), 1, f) != 1) { fclose(f); return -1; }
    if (fwrite(&w->width, sizeof(int), 1, f) != 1) { fclose(f); return -1; }
    if (fwrite(&w->height, sizeof(int), 1, f) != 1) { fclose(f); return -1; }

    size_t tile_count = (size_t)w->width * w->height;
    if (fwrite(w->tiles, sizeof(Tile), tile_count, f) != tile_count) { fclose(f); return -1; }

    fclose(f);
    return 0;
}

int world_load(World *w, const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return -1;

    char magic[4];
    uint32_t version;
    int width, height;

    if (fread(magic, 1, 4, f) != 4) { fclose(f); return -1; }
    if (memcmp(magic, "GROW", 4) != 0) { fclose(f); return -1; }

    if (fread(&version, sizeof(uint32_t), 1, f) != 1) { fclose(f); return -1; }
    if (version != 1) { fclose(f); return -1; }

    if (fread(&width, sizeof(int), 1, f) != 1) { fclose(f); return -1; }
    if (fread(&height, sizeof(int), 1, f) != 1) { fclose(f); return -1; }

    if (world_init(w, width, height) != 0) { fclose(f); return -1; }

    size_t tile_count = (size_t)width * height;
    if (fread(w->tiles, sizeof(Tile), tile_count, f) != tile_count) {
        world_free(w);
        fclose(f);
        return -1;
    }

    fclose(f);
    return 0;
}

int world_is_solid(World *w, int x, int y) {
    Tile *t = world_get_tile(w, x, y);
    if (!t) return 1;
    return block_is_solid(t->fg);
}
