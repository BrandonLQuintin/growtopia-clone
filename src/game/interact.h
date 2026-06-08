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

#endif
