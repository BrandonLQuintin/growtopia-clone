#ifndef UI_H
#define UI_H

#include <stdint.h>
#include <SDL2/SDL.h>
#include "renderer.h"
#include "input.h"
#include "../world/items.h"
#include "../world/world.h"

#define UI_MAX_BUTTONS 128
#define UI_MAX_SLOTS 64

typedef enum {
    UI_STATE_NONE,
    UI_STATE_INVENTORY,
    UI_STATE_STORE,
    UI_STATE_CRAFTING,
    UI_STATE_SIGN_EDIT
} UIState;

typedef struct {
    int x, y, w, h;
    const char *text;
    int hovered;
    int clicked;
} UIButton;

typedef struct {
    int x, y, size;
    int item_id;
    int count;
    int hovered;
    int selected;
} UISlot;

typedef struct {
    UIState state;
    UIButton buttons[UI_MAX_BUTTONS];
    int button_count;
    UISlot slots[UI_MAX_SLOTS];
    int slot_count;
    int selected_slot;
    int hotbar_selection;
    int drag_item_id;
    int drag_count;
    int drag_from_slot;
    int tooltip_slot;
    int store_category;
    int store_scroll;
    int inventory_scroll;
    int sign_edit_x, sign_edit_y;
    char sign_edit_text[SIGN_TEXT_MAX_LEN + 1];
    int sign_edit_cursor;
    float sign_edit_cursor_timer;
} UI;

void ui_init(UI *ui);
void ui_update(UI *ui, Input *input, Renderer *renderer);
void ui_render(UI *ui, Renderer *renderer);

void ui_toggle_inventory(UI *ui);
void ui_toggle_store(UI *ui);
void ui_close_all(UI *ui);

void ui_render_hotbar(UI *ui, Renderer *renderer, uint16_t *hotbar_items, int *hotbar_counts, int selected);
void ui_render_inventory_screen(UI *ui, Renderer *renderer, uint16_t *inv_items, int *inv_counts, int inv_size);
void ui_render_store_screen(UI *ui, Renderer *renderer, int gems);
void ui_render_hud(UI *ui, Renderer *renderer, int gems, int health);

void ui_init_sign_edit(UI *ui, World *w, int tx, int ty);
void ui_finish_sign_edit(UI *ui, World *w);
void ui_render_sign_edit(UI *ui, Renderer *renderer);
void ui_update_sign_edit(UI *ui, Input *input);
int ui_handle_sign_edit_event(UI *ui, World *w, SDL_Event *e);

void ui_render_exit_confirm(Renderer *renderer);

int ui_get_hotbar_selection(UI *ui);

#endif
