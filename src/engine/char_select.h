#ifndef CHAR_SELECT_H
#define CHAR_SELECT_H

#include "renderer.h"
#include "../game/character.h"

#define CHAR_LIST_MAX 64

typedef struct {
    char names[CHAR_LIST_MAX][CHAR_NAME_MAX + 1];
    int  gems_snap[CHAR_LIST_MAX];
    char last_world_snap[CHAR_LIST_MAX][WORLD_NAME_MAX + 1];
    int  count;
    int  scroll;
    int  hovered;
    int  selected;

    int  creating;
    int  renaming;
    int  deleting;
    char input_text[CHAR_NAME_MAX + 1];
    int  input_cursor;
    float cursor_timer;
    int  mouse_clicked;
} CharSelect;

void char_select_init(CharSelect *cs);
void char_select_refresh(CharSelect *cs);
void char_select_handle_event(CharSelect *cs, SDL_Event *e);
void char_select_update(CharSelect *cs, float dt);
void char_select_render(CharSelect *cs, Renderer *r);

#endif
