#ifndef INVENTORY_H
#define INVENTORY_H

#include <stdint.h>

#define INVENTORY_SIZE 36
#define HOTBAR_SIZE 9

typedef struct {
    uint16_t items[INVENTORY_SIZE];
    int counts[INVENTORY_SIZE];
} Inventory;

void inventory_init(Inventory *inv);
int inventory_add(Inventory *inv, uint16_t item_id, int count);
int inventory_remove(Inventory *inv, uint16_t item_id, int count);
int inventory_count_item(Inventory *inv, uint16_t item_id);
int inventory_find_slot(Inventory *inv, uint16_t item_id);
int inventory_has_item(Inventory *inv, uint16_t item_id, int count);
int inventory_get_hotbar_item(Inventory *inv, int slot);
int inventory_get_hotbar_count(Inventory *inv, int slot);
void inventory_swap_slots(Inventory *inv, int a, int b);
int inventory_save(Inventory *inv, const char *path);
int inventory_load(Inventory *inv, const char *path);
int inventory_save_profile(Inventory *inv, int gems, int health, const char *path);
int inventory_load_profile(Inventory *inv, int *gems, int *health, const char *path);

#endif
