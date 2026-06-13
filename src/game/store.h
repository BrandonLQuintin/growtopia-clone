#ifndef STORE_H
#define STORE_H

#include <stdint.h>
#include "../engine/input.h"
#include "inventory.h"

#define STORE_CAT_BLOCKS 0
#define STORE_CAT_SEEDS 1
#define STORE_CAT_TOOLS 2
#define STORE_CAT_CLOTHING 3
#define STORE_CAT_SPECIAL 4
#define STORE_CAT_COUNT 5

#define STORE_COLS           5
#define STORE_SLOT_SIZE      48
#define STORE_CELL_W        100
#define STORE_CELL_H         80
#define STORE_PANEL_Y        60
#define STORE_PANEL_PAD      20
#define STORE_TAB_H          28
#define STORE_TAB_Y_OFFSET   36
#define STORE_ITEM_Y_GAP     16

typedef struct {
    uint16_t item_id;
    int price;
} StoreEntry;

typedef struct {
    StoreEntry entries[256];
    int count;
} StoreCategory;

typedef struct {
    StoreCategory categories[STORE_CAT_COUNT];
} Store;

void store_init(Store *s);
int store_buy(Store *s, int category, int index, int *gems, uint16_t *bought_item, int *bought_count);
int store_sell_price(uint16_t item_id);
int store_get_items(Store *s, int category, StoreEntry **out_entries, int *out_count);
void store_handle_click(Store *s, Inventory *inv, Input *in, int *gems, int category);

#endif
