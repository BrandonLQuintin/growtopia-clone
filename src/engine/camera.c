#include "camera.h"
#include "renderer.h"

void camera_init(Camera *cam, int world_w, int world_h) {
    cam->x = (float)world_w / 2.0f;
    cam->y = (float)world_h / 2.0f;
    cam->target_x = cam->x;
    cam->target_y = cam->y;
    cam->world_w = world_w;
    cam->world_h = world_h;
    cam->lerp_speed = 5.0f;
}

void camera_update(Camera *cam, float dt) {
    cam->x += (cam->target_x - cam->x) * cam->lerp_speed * dt;
    cam->y += (cam->target_y - cam->y) * cam->lerp_speed * dt;

    float half_w = g_screen_w / 2.0f;
    float half_h = g_screen_h / 2.0f;

    if (cam->x - half_w < 0.0f)
        cam->x = half_w;
    if (cam->x + half_w > (float)cam->world_w)
        cam->x = (float)cam->world_w - half_w;
    if (cam->y - half_h < 0.0f)
        cam->y = half_h;
    if (cam->y + half_h > (float)cam->world_h)
        cam->y = (float)cam->world_h - half_h;
}

void camera_set_target(Camera *cam, float x, float y) {
    cam->target_x = x;
    cam->target_y = y;
}

void camera_world_to_screen(Camera *cam, float wx, float wy, int *sx, int *sy) {
    *sx = (int)(wx - cam->x + g_screen_w / 2.0f);
    *sy = (int)(wy - cam->y + g_screen_h / 2.0f);
}

void camera_screen_to_world(Camera *cam, int sx, int sy, int *wx, int *wy) {
    *wx = (int)((float)sx + cam->x - g_screen_w / 2.0f);
    *wy = (int)((float)sy + cam->y - g_screen_h / 2.0f);
}
