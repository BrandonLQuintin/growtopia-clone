#ifndef EXPLOSIVE_H
#define EXPLOSIVE_H

#include "../world/world.h"
#include "../game/player.h"
#include "../engine/camera.h"
#include "../engine/input.h"
#include "../engine/renderer.h"

#define BOMB_THROW_SPEED    350.0f
#define BOMB_GRAVITY        980.0f
#define BOMB_RADIUS_TILES   3
#define BOMB_DAMAGE         50
#define BOMB_KNOCKBACK      420.0f
#define BOMB_MAX_FLIGHT_S   5.0f
#define BOMB_FLASH_S        0.25f

int  explosive_throw(Player *p, Camera *cam, Input *in);
void explosive_update(World *w, Player *p, float dt);
void explosive_render(Renderer *r, Camera *cam);
void explosive_reset(void);

#endif
