#include "character.h"
#include <stdio.h>
#include <string.h>

void character_path(const char *name, char *out, size_t out_size)
{
    snprintf(out, out_size, "res/chars/%s.dat", name);
}

int character_save(const Character *c, const char *path)
{
    FILE *f = fopen(path, "wb");
    if (!f) return -1;

    fwrite(CHAR_MAGIC, 4, 1, f);
    int version = CHAR_VERSION;
    fwrite(&version, sizeof(int), 1, f);

    char name_buf[64];
    memset(name_buf, 0, sizeof(name_buf));
    snprintf(name_buf, sizeof(name_buf), "%s", c->name);
    fwrite(name_buf, sizeof(name_buf), 1, f);

    char last_buf[64];
    memset(last_buf, 0, sizeof(last_buf));
    snprintf(last_buf, sizeof(last_buf), "%s", c->last_world);
    fwrite(last_buf, sizeof(last_buf), 1, f);

    fwrite(&c->gems, sizeof(int), 1, f);
    fwrite(&c->health, sizeof(int), 1, f);
    fwrite(c->equipped, sizeof(uint16_t), 3, f);

    for (int i = 0; i < INVENTORY_SIZE; i++) {
        fwrite(&c->inventory.items[i], sizeof(uint16_t), 1, f);
        fwrite(&c->inventory.counts[i], sizeof(int), 1, f);
    }

    fclose(f);
    return 0;
}

int character_load(Character *c, const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f) return -1;

    char magic[4];
    if (fread(magic, 1, 4, f) != 4 || memcmp(magic, CHAR_MAGIC, 4) != 0) {
        fclose(f);
        return -1;
    }

    int version = 0;
    if (fread(&version, sizeof(int), 1, f) != 1) {
        fclose(f);
        return -1;
    }
    if (version != CHAR_VERSION) {
        fclose(f);
        return -1;
    }

    char name_buf[64];
    char last_buf[64];
    if (fread(name_buf, sizeof(name_buf), 1, f) != 1 ||
        fread(last_buf, sizeof(last_buf), 1, f) != 1) {
        fclose(f);
        return -1;
    }
    name_buf[sizeof(name_buf) - 1] = '\0';
    last_buf[sizeof(last_buf) - 1] = '\0';
    size_t name_len = strnlen(name_buf, sizeof(c->name) - 1);
    memcpy(c->name, name_buf, name_len);
    c->name[name_len] = '\0';
    size_t last_len = strnlen(last_buf, sizeof(c->last_world) - 1);
    memcpy(c->last_world, last_buf, last_len);
    c->last_world[last_len] = '\0';

    if (fread(&c->gems, sizeof(int), 1, f) != 1 ||
        fread(&c->health, sizeof(int), 1, f) != 1 ||
        fread(c->equipped, sizeof(uint16_t), 3, f) != 3) {
        fclose(f);
        return -1;
    }

    for (int i = 0; i < INVENTORY_SIZE; i++) {
        if (fread(&c->inventory.items[i], sizeof(uint16_t), 1, f) != 1 ||
            fread(&c->inventory.counts[i], sizeof(int), 1, f) != 1) {
            fclose(f);
            return -1;
        }
    }

    fclose(f);
    return 0;
}
