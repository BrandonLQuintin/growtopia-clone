#ifndef INTERACT_H
#define INTERACT_H

#include <stdint.h>
#include "../world/world.h"
#include "player.h"

int interact_punch(World *w, Player *p, int tx, int ty);
int interact_wrench(World *w, int tx, int ty, int *pending_x, int *pending_y, int *pending);
const char *interact_get_sign_text(World *w, int tx, int ty);
int interact_alloc_sign(World *w, int tx, int ty);
void interact_cleanup_break(World *w, int tx, int ty);
int interact_is_portal_linked(World *w, int tx, int ty);

#define REACH_RADIUS 6

#include "../engine/input.h"
#include "../engine/camera.h"
#include "inventory.h"
#include "../engine/ui.h"
#include "../world/items.h"

int  interact_handle_click(World *w, Player *p, Inventory *inv, int hotbar_sel,
                           Input *in, Camera *cam,
                           int *sign_x, int *sign_y);
void interact_handle_break(World *w, Player *p, Inventory *inv, int hotbar_sel,
                           Input *in, Camera *cam, float dt);
void interact_handle_place(World *w, Player *p, Inventory *inv, int hotbar_sel,
                           Input *in, Camera *cam, UI *ui,
                           int *portal_pending, int *portal_x, int *portal_y);

#endif
