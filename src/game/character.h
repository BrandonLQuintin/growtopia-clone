#ifndef CHARACTER_H
#define CHARACTER_H

#include <stdint.h>
#include <stddef.h>
#include "inventory.h"
#include "../engine/world_select.h"

#define CHAR_NAME_MAX 20
#define CHAR_MAGIC "CHAR"
#define CHAR_VERSION 1

typedef struct {
    char      name[CHAR_NAME_MAX + 1];
    char      last_world[WORLD_NAME_MAX + 1];
    int       gems;
    int       health;
    uint16_t  equipped[3];
    Inventory inventory;
} Character;

void character_path(const char *name, char *out, size_t out_size);
int  character_load(Character *c, const char *path);
int  character_save(const Character *c, const char *path);

#endif
