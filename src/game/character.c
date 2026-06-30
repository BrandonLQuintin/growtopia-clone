#include "character.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include "player.h"
#include "../world/world.h"

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

static int name_cmp(const void *a, const void *b)
{
    const char *sa = (const char *)a;
    const char *sb = (const char *)b;
    return strcmp(sa, sb);
}

int character_exists(const char *name)
{
    char path[256];
    character_path(name, path, sizeof(path));
    FILE *f = fopen(path, "rb");
    if (f) { fclose(f); return 1; }
    return 0;
}

int character_name_valid(const char *name)
{
    int len = (int)strlen(name);
    if (len < 1 || len > CHAR_NAME_MAX) return 0;
    for (int i = 0; i < len; i++) {
        char c = name[i];
        if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9'))) return 0;
    }
    return 1;
}

int character_delete(const char *name)
{
    char path[256];
    character_path(name, path, sizeof(path));
    return remove(path);
}

int character_rename(const char *old_name, const char *new_name)
{
    char old_path[256];
    char new_path[256];
    character_path(old_name, old_path, sizeof(old_path));
    character_path(new_name, new_path, sizeof(new_path));
    if (rename(old_path, new_path) != 0) return -1;

    Character c;
    memset(&c, 0, sizeof(c));
    if (character_load(&c, new_path) != 0) return -1;
    snprintf(c.name, sizeof(c.name), "%s", new_name);
    return character_save(&c, new_path);
}

int character_list(char names[][CHAR_NAME_MAX + 1], int *count, int max)
{
    *count = 0;
    DIR *d = opendir("res/chars");
    if (!d) return -1;

    struct dirent *de;
    while ((de = readdir(d)) != NULL) {
        const char *nm = de->d_name;
        int len = (int)strlen(nm);
        if (len < 5) continue;
        if (strcmp(nm + len - 4, ".dat") != 0) continue;
        if (*count >= max) break;
        int blen = len - 4;
        if (blen > CHAR_NAME_MAX) blen = CHAR_NAME_MAX;
        char base[CHAR_NAME_MAX + 1];
        memcpy(base, nm, blen);
        base[blen] = '\0';
        snprintf(names[*count], CHAR_NAME_MAX + 1, "%s", base);
        (*count)++;
    }
    closedir(d);
    qsort(names, *count, sizeof(names[0]), name_cmp);
    return 0;
}

void character_apply_defaults(Character *c)
{
    memset(c, 0, sizeof(*c));
    inventory_add(&c->inventory, BLOCK_DIRT, 50);
    inventory_add(&c->inventory, BLOCK_STONE, 30);
    inventory_add(&c->inventory, BLOCK_WOOD, 20);
    inventory_add(&c->inventory, SEED_DIRT, 10);
    inventory_add(&c->inventory, SEED_GRASS, 5);
    inventory_add(&c->inventory, SEED_WOOD, 5);
    c->gems = 999999;
    c->health = MAX_HEALTH;
}

int character_migrate_from_profile(const char *profile_path)
{
    DIR *d = opendir("res/chars");
    int empty = 1;
    if (d) {
        struct dirent *de;
        while ((de = readdir(d)) != NULL) {
            int len = (int)strlen(de->d_name);
            if (len >= 5 && strcmp(de->d_name + len - 4, ".dat") == 0) {
                empty = 0;
                break;
            }
        }
        closedir(d);
    }
    if (!empty) return 0;

    FILE *f = fopen(profile_path, "rb");
    if (!f) return 0;
    fclose(f);

    Character c;
    memset(&c, 0, sizeof(c));
    uint16_t equipped[3] = {0, 0, 0};
    if (inventory_load_profile(&c.inventory, &c.gems, &c.health, equipped, profile_path) != 0) {
        return -1;
    }
    c.equipped[0] = equipped[0];
    c.equipped[1] = equipped[1];
    c.equipped[2] = equipped[2];
    snprintf(c.name, sizeof(c.name), "default");
    c.last_world[0] = '\0';

    char out[256];
    character_path("default", out, sizeof(out));
    return character_save(&c, out);
}
