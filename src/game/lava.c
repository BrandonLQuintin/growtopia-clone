#include "lava.h"
#include "../world/block.h"
#include "../engine/prng.h"
#include "../engine/renderer.h"
#include <stdio.h>

void lava_generate_pools(World *w)
{
    int num_pools = prng_range(&w->rng, LAVA_POOL_MIN, LAVA_POOL_MAX);
    for (int n = 0; n < num_pools; n++) {
        int cx = prng_range(&w->rng, 0, w->width - 1);
        int cy_start = prng_range(&w->rng, LAVA_POOL_DEPTH_MIN, LAVA_POOL_DEPTH_MAX);

        int floor_y = -1;
        for (int y = cy_start; y < w->height - 1 && y < cy_start + 10; y++) {
            if (world_is_solid(w, cx, y + 1)) {
                floor_y = y;
                break;
            }
        }
        if (floor_y < 0) continue;

        int stack_x[LAVA_POOL_MAX_TILES];
        int stack_y[LAVA_POOL_MAX_TILES];
        int stack_n = 0;
        int placed = 0;

        Tile *start = world_get_tile(w, cx, floor_y);
        if (!start || start->fg != BLOCK_AIR) continue;
        start->fg = BLOCK_LAVA;
        stack_x[stack_n] = cx;
        stack_y[stack_n] = floor_y;
        stack_n++;
        placed++;

        while (stack_n > 0 && placed < LAVA_POOL_MAX_TILES) {
            stack_n--;
            int x = stack_x[stack_n];
            int y = stack_y[stack_n];

            const int dxs[3] = {0, -1, 1};
            for (int i = 0; i < 3 && placed < LAVA_POOL_MAX_TILES; i++) {
                int nx = x + dxs[i];
                int ny = (dxs[i] == 0) ? y + 1 : y;
                if (nx < 0 || nx >= w->width) continue;
                if (ny < 27 || ny >= w->height) continue;
                Tile *t = world_get_tile(w, nx, ny);
                if (!t || t->fg != BLOCK_AIR) continue;
                Tile *below_target = world_get_tile(w, nx, ny + 1);
                if (!below_target || !block_is_solid_with_data(below_target->fg, below_target->extra_data)) continue;
                t->fg = BLOCK_LAVA;
                stack_x[stack_n] = nx;
                stack_y[stack_n] = ny;
                stack_n++;
                placed++;
            }
        }
    }
}

void lava_update(World *w, Player *p, float dt)
{
    (void)w;
    (void)p;
    (void)dt;
}
