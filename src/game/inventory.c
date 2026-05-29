#include "inventory.h"
#include <string.h>
#include <stdio.h>

void inventory_init(Inventory *inv)
{
    memset(inv, 0, sizeof(*inv));
}

int inventory_add(Inventory *inv, uint16_t item_id, int count)
{
    for (int i = 0; i < INVENTORY_SIZE; i++) {
        if (inv->items[i] == item_id && inv->counts[i] > 0) {
            inv->counts[i] += count;
            return 0;
        }
    }
    for (int i = 0; i < INVENTORY_SIZE; i++) {
        if (inv->items[i] == 0 && inv->counts[i] == 0) {
            inv->items[i] = item_id;
            inv->counts[i] = count;
            return 0;
        }
    }
    return -1;
}

int inventory_remove(Inventory *inv, uint16_t item_id, int count)
{
    int slot = inventory_find_slot(inv, item_id);
    if (slot < 0)
        return -1;
    if (inv->counts[slot] < count)
        return -1;
    inv->counts[slot] -= count;
    if (inv->counts[slot] <= 0) {
        inv->items[slot] = 0;
        inv->counts[slot] = 0;
    }
    return 0;
}

int inventory_count_item(Inventory *inv, uint16_t item_id)
{
    int total = 0;
    for (int i = 0; i < INVENTORY_SIZE; i++) {
        if (inv->items[i] == item_id)
            total += inv->counts[i];
    }
    return total;
}

int inventory_find_slot(Inventory *inv, uint16_t item_id)
{
    for (int i = 0; i < INVENTORY_SIZE; i++) {
        if (inv->items[i] == item_id && inv->counts[i] > 0)
            return i;
    }
    return -1;
}

int inventory_has_item(Inventory *inv, uint16_t item_id, int count)
{
    return inventory_count_item(inv, item_id) >= count;
}

int inventory_get_hotbar_item(Inventory *inv, int slot)
{
    if (slot < 0 || slot >= HOTBAR_SIZE)
        return 0;
    return inv->items[slot];
}

int inventory_get_hotbar_count(Inventory *inv, int slot)
{
    if (slot < 0 || slot >= HOTBAR_SIZE)
        return 0;
    return inv->counts[slot];
}

void inventory_swap_slots(Inventory *inv, int a, int b)
{
    if (a < 0 || a >= INVENTORY_SIZE || b < 0 || b >= INVENTORY_SIZE || a == b)
        return;
    uint16_t tmp_item = inv->items[a];
    int tmp_count = inv->counts[a];
    inv->items[a] = inv->items[b];
    inv->counts[a] = inv->counts[b];
    inv->items[b] = tmp_item;
    inv->counts[b] = tmp_count;
}

int inventory_save(Inventory *inv, const char *path)
{
    FILE *f = fopen(path, "wb");
    if (!f)
        return -1;
    for (int i = 0; i < INVENTORY_SIZE; i++) {
        fwrite(&inv->items[i], sizeof(uint16_t), 1, f);
        fwrite(&inv->counts[i], sizeof(int), 1, f);
    }
    fclose(f);
    return 0;
}

int inventory_load(Inventory *inv, const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f)
        return -1;
    for (int i = 0; i < INVENTORY_SIZE; i++) {
        if (fread(&inv->items[i], sizeof(uint16_t), 1, f) != 1 ||
            fread(&inv->counts[i], sizeof(int), 1, f) != 1) {
            fclose(f);
            return -1;
        }
    }
    fclose(f);
    return 0;
}
