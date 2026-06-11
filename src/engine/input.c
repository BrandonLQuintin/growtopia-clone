#include "input.h"
#include <string.h>

void input_init(Input *input)
{
    memset(input, 0, sizeof(Input));
}

void input_update(Input *input)
{
    memset(input->key_pressed, 0, sizeof(input->key_pressed));
    memset(input->mouse_clicked, 0, sizeof(input->mouse_clicked));
    input->mouse_rel_x = 0;
    input->mouse_rel_y = 0;
    input->mouse_scroll_y = 0;
}

void input_handle_event(Input *input, SDL_Event *e)
{
    switch (e->type) {
    case SDL_KEYDOWN:
        if (e->key.keysym.scancode < MAX_KEYS && !e->key.repeat) {
            input->key_pressed[e->key.keysym.scancode] = 1;
            input->key_down[e->key.keysym.scancode] = 1;
        }
        break;
    case SDL_KEYUP:
        if (e->key.keysym.scancode < MAX_KEYS) {
            input->key_down[e->key.keysym.scancode] = 0;
        }
        break;
    case SDL_MOUSEBUTTONDOWN:
        if (e->button.button >= 1 && e->button.button <= 5) {
            int idx = e->button.button - 1;
            input->mouse_buttons[idx] = 1;
            input->mouse_clicked[idx] = 1;
        }
        break;
    case SDL_MOUSEBUTTONUP:
        if (e->button.button >= 1 && e->button.button <= 5) {
            input->mouse_buttons[e->button.button - 1] = 0;
        }
        break;
    case SDL_MOUSEMOTION:
        input->mouse_x = e->motion.x;
        input->mouse_y = e->motion.y;
        input->mouse_rel_x = e->motion.xrel;
        input->mouse_rel_y = e->motion.yrel;
        break;
    case SDL_MOUSEWHEEL:
        input->mouse_scroll_y += e->wheel.y;
        break;
    }
}

int input_is_key_down(Input *input, SDL_Scancode key)
{
    if (key < 0 || key >= MAX_KEYS) return 0;
    return input->key_down[key];
}

int input_is_key_pressed(Input *input, SDL_Scancode key)
{
    if (key < 0 || key >= MAX_KEYS) return 0;
    return input->key_pressed[key];
}

int input_is_mouse_down(Input *input, int button)
{
    if (button < 1 || button > 5) return 0;
    return input->mouse_buttons[button - 1];
}

int input_is_mouse_clicked(Input *input, int button)
{
    if (button < 1 || button > 5) return 0;
    return input->mouse_clicked[button - 1];
}
