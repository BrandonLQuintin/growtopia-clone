#ifndef WORLD_SELECT_H
#define WORLD_SELECT_H

#include "renderer.h"

#define WORLD_NAME_MAX 20
#define RECENT_WORLDS_MAX 8

typedef struct {
    char input_text[WORLD_NAME_MAX + 1];
    int input_cursor;
    float cursor_timer;
    char recent_names[RECENT_WORLDS_MAX][WORLD_NAME_MAX + 1];
    int recent_count;
    int hovered_recent;
    int hovered_enter;
    int mouse_clicked;
    int submitted;
} WorldSelect;

void world_select_init(WorldSelect *ws);
void world_select_handle_event(WorldSelect *ws, SDL_Event *e);
void world_select_update(WorldSelect *ws, float dt);
void world_select_render(WorldSelect *ws, Renderer *r);
void world_select_load_recent(WorldSelect *ws);
void world_select_add_recent(WorldSelect *ws, const char *name);

#endif
