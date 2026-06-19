#include "clouds.h"
#include "prng.h"
#include <math.h>

static uint32_t name_hash(const char *s) {
    uint32_t hash = 5381u;
    const unsigned char *p = (const unsigned char *)s;
    while (*p) {
        hash = hash * 33u + *p;
        p++;
    }
    return hash;
}

void clouds_init(Clouds *c, const char *world_name, int world_w, int world_h) {
    c->world_w = world_w;
    c->world_h = world_h;
    c->wrap_w = world_w;

    Prng rng;
    prng_seed(&rng, name_hash(world_name));

    for (int L = 0; L < CLOUD_LAYER_COUNT; L++) {
        c->count[L] = CLOUDS_PER_LAYER;
        int size_percent = (L == 0) ? 100 : 60;
        float scale = size_percent / 100.0f;
        for (int i = 0; i < CLOUDS_PER_LAYER; i++) {
            Cloud *cl = &c->layer[L][i];
            cl->world_x = (float)prng_range(&rng, 0, world_w);
            cl->world_y = (float)prng_range(&rng, 0, (world_h * 2) / 5);
            if (L == 0) {
                cl->drift_speed = 10.0f + 6.0f * prng_float(&rng);
            } else {
                cl->drift_speed = 4.0f + 4.0f * prng_float(&rng);
            }
            cl->size_scale = size_percent;
            cl->part_count = prng_range(&rng, 5, CLOUD_MAX_PARTS);
            for (int p = 0; p < cl->part_count; p++) {
                int base_ox = prng_range(&rng, -32, 32);
                int base_oy = prng_range(&rng, -12, 12);
                int base_w = prng_range(&rng, 12, 28);
                int base_h = prng_range(&rng, 12, 28);
                cl->ox[p] = (int)(base_ox * scale);
                cl->oy[p] = (int)(base_oy * scale);
                cl->w[p]  = (int)(base_w  * scale);
                cl->h[p]  = (int)(base_h  * scale);
            }
        }
    }
}

void clouds_update(Clouds *c, float dt) {
    for (int L = 0; L < CLOUD_LAYER_COUNT; L++) {
        for (int i = 0; i < c->count[L]; i++) {
            c->layer[L][i].world_x += c->layer[L][i].drift_speed * dt;
        }
    }
}

void clouds_render(Clouds *c, Renderer *r, const Camera *cam) {
    static const float parallax[CLOUD_LAYER_COUNT] = { 0.50f, 0.25f };
    static const float col_r[CLOUD_LAYER_COUNT]   = { 1.00f, 0.85f };
    static const float col_g[CLOUD_LAYER_COUNT]   = { 1.00f, 0.85f };
    static const float col_b[CLOUD_LAYER_COUNT]   = { 1.00f, 0.90f };
    static const float col_a[CLOUD_LAYER_COUNT]   = { 0.90f, 0.60f };

    renderer_begin_ui(r);
    for (int L = 0; L < CLOUD_LAYER_COUNT; L++) {
        for (int i = 0; i < c->count[L]; i++) {
            Cloud *cl = &c->layer[L][i];
            float eff_x = fmodf(cl->world_x, (float)c->wrap_w);
            if (eff_x < 0.0f) eff_x += (float)c->wrap_w;
            int screen_x = (int)(eff_x - (cam->x - c->world_w / 2.0f) * parallax[L]);
            int screen_y = (int)(cl->world_y - (cam->y - c->world_h / 2.0f) * parallax[L]);
            if (screen_x < -CLOUD_MAX_WIDTH || screen_x > g_screen_w + CLOUD_MAX_WIDTH) {
                continue;
            }
            for (int p = 0; p < cl->part_count; p++) {
                renderer_draw_rect(r, screen_x + cl->ox[p], screen_y + cl->oy[p],
                                   cl->w[p], cl->h[p],
                                   col_r[L], col_g[L], col_b[L], col_a[L]);
            }
        }
    }
    renderer_end_ui(r);
}
