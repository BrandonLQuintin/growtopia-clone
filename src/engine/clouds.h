#ifndef CLOUDS_H
#define CLOUDS_H

#include <stdint.h>
#include "renderer.h"
#include "camera.h"

#define CLOUD_LAYER_COUNT 2
#define CLOUDS_PER_LAYER  12
#define CLOUD_MAX_PARTS   9
#define CLOUD_MAX_WIDTH   96

typedef struct {
    int ox[CLOUD_MAX_PARTS];
    int oy[CLOUD_MAX_PARTS];
    int w[CLOUD_MAX_PARTS];
    int h[CLOUD_MAX_PARTS];
    int part_count;
    float world_x;
    float world_y;
    float drift_speed;
    int size_scale;
} Cloud;

typedef struct Clouds {
    Cloud layer[CLOUD_LAYER_COUNT][CLOUDS_PER_LAYER];
    int   count[CLOUD_LAYER_COUNT];
    int   world_w;
    int   world_h;
    int   wrap_w;
} Clouds;

void clouds_init(Clouds *c, const char *world_name, int world_w, int world_h);
void clouds_update(Clouds *c, float dt);
void clouds_render(Clouds *c, Renderer *r, const Camera *cam);

#endif
