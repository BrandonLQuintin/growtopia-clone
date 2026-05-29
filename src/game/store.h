#ifndef STORE_H
#define STORE_H

#include <stdint.h>

#define STORE_CAT_BLOCKS 0
#define STORE_CAT_SEEDS 1
#define STORE_CAT_TOOLS 2
#define STORE_CAT_CLOTHING 3
#define STORE_CAT_SPECIAL 4
#define STORE_CAT_COUNT 5

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

#endif
