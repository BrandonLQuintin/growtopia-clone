#ifndef INPUT_H
#define INPUT_H

#include <SDL2/SDL.h>

#define MAX_KEYS 512

typedef struct {
    int key_down[MAX_KEYS];
    int key_pressed[MAX_KEYS];
    int mouse_buttons[5];
    int mouse_clicked[5];
    int mouse_x, mouse_y;
    int mouse_rel_x, mouse_rel_y;
} Input;

void input_init(Input *input);
void input_update(Input *input);
void input_handle_event(Input *input, SDL_Event *e);
int input_is_key_down(Input *input, SDL_Scancode key);
int input_is_key_pressed(Input *input, SDL_Scancode key);
int input_is_mouse_down(Input *input, int button);
int input_is_mouse_clicked(Input *input, int button);

#endif
