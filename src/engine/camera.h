#ifndef CAMERA_H
#define CAMERA_H

typedef struct {
    float x, y;
    float target_x, target_y;
    int world_w, world_h;
    float lerp_speed;
    float zoom;
    float zoom_target;
} Camera;

void camera_init(Camera *cam, int world_w, int world_h);
void camera_update(Camera *cam, float dt);
void camera_set_target(Camera *cam, float x, float y);
void camera_world_to_screen(Camera *cam, float wx, float wy, int *sx, int *sy);
void camera_screen_to_world(Camera *cam, int sx, int sy, int *wx, int *wy);
void camera_set_zoom(Camera *cam, float zoom);

#endif
