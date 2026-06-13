#include "store.h"
#include "../engine/renderer.h"
#include "../world/world.h"
#include "../world/items.h"
#include <string.h>

#define ITEM_HAT 9100
#define ITEM_SHIRT 9101
#define ITEM_PANTS 9102

static void cat_add(StoreCategory *cat, uint16_t id, int price)
{
    cat->entries[cat->count].item_id = id;
    cat->entries[cat->count].price = price;
    cat->count++;
}

void store_init(Store *s)
{
    memset(s, 0, sizeof(Store));

    StoreCategory *cat;

    cat = &s->categories[STORE_CAT_BLOCKS];
    cat_add(cat, BLOCK_DIRT, 5);
    cat_add(cat, BLOCK_STONE, 10);
    cat_add(cat, BLOCK_GRASS, 8);
    cat_add(cat, BLOCK_WOOD, 8);
    cat_add(cat, BLOCK_BRICK, 15);
    cat_add(cat, BLOCK_GLASS, 12);
    cat_add(cat, BLOCK_SAND, 5);
    cat_add(cat, BLOCK_ROCK, 15);
    cat_add(cat, BLOCK_LIMESTONE, 20);
    cat_add(cat, BLOCK_MUD, 5);
    cat_add(cat, BLOCK_CLAY, 8);
    cat_add(cat, BLOCK_GRAVEL, 3);
    cat_add(cat, BLOCK_ICE, 12);
    cat_add(cat, BLOCK_SNOW, 10);

    cat = &s->categories[STORE_CAT_SEEDS];
    cat_add(cat, SEED_DIRT, 10);
    cat_add(cat, SEED_STONE, 20);
    cat_add(cat, SEED_GRASS, 15);
    cat_add(cat, SEED_WOOD, 15);
    cat_add(cat, SEED_BRICK, 30);
    cat_add(cat, SEED_SAND, 10);
    cat_add(cat, SEED_ROCK, 30);
    cat_add(cat, SEED_CACTUS, 50);
    cat_add(cat, SEED_FLOWER, 40);
    cat_add(cat, SEED_MUSHROOM, 45);
    cat_add(cat, SEED_BUSH, 35);

    cat = &s->categories[STORE_CAT_TOOLS];
    cat_add(cat, ITEM_WRENCH, 200);
    cat_add(cat, ITEM_PICKAXE, 500);

    cat = &s->categories[STORE_CAT_CLOTHING];
    cat_add(cat, ITEM_HAT, 100);
    cat_add(cat, ITEM_SHIRT, 150);
    cat_add(cat, ITEM_PANTS, 150);

    cat = &s->categories[STORE_CAT_SPECIAL];
    cat_add(cat, BLOCK_SIGN, 20);
    cat_add(cat, BLOCK_LOCK, 50);
    cat_add(cat, BLOCK_DOOR, 25);
    cat_add(cat, BLOCK_STORE, 1000);
    cat_add(cat, BLOCK_MAILBOX, 30);
    cat_add(cat, BLOCK_PORTAL, 500);
}

int store_buy(Store *s, int category, int index, int *gems, uint16_t *bought_item, int *bought_count)
{
    if (category < 0 || category >= STORE_CAT_COUNT) return -1;
    StoreCategory *cat = &s->categories[category];
    if (index < 0 || index >= cat->count) return -1;
    StoreEntry *entry = &cat->entries[index];
    if (*gems < entry->price) return -1;
    *gems -= entry->price;
    *bought_item = entry->item_id;
    *bought_count = 1;
    return 0;
}

int store_sell_price(uint16_t item_id)
{
    return item_get_sell_cost(item_id);
}

int store_get_items(Store *s, int category, StoreEntry **out_entries, int *out_count)
{
    if (category < 0 || category >= STORE_CAT_COUNT) return -1;
    *out_entries = s->categories[category].entries;
    *out_count = s->categories[category].count;
    return 0;
}

void store_handle_click(Store *s, Inventory *inv, Input *in, int *gems, int category)
{
    if (!input_is_mouse_clicked(in, 1)) return;

    StoreEntry *entries = NULL;
    int entry_count = 0;
    store_get_items(s, category, &entries, &entry_count);

    int panel_w = STORE_COLS * STORE_CELL_W + STORE_PANEL_PAD * 2;
    int items_x = (g_screen_w - panel_w) / 2 + STORE_PANEL_PAD;
    int tabs_y = STORE_PANEL_Y + STORE_TAB_Y_OFFSET;
    int items_y = tabs_y + STORE_TAB_H + STORE_ITEM_Y_GAP;

    for (int i = 0; i < entry_count; i++) {
        int col = i % STORE_COLS;
        int row = i / STORE_COLS;
        int cx = items_x + col * STORE_CELL_W;
        int cy = items_y + row * STORE_CELL_H;

        if (in->mouse_x >= cx && in->mouse_x < cx + STORE_SLOT_SIZE &&
            in->mouse_y >= cy && in->mouse_y < cy + STORE_SLOT_SIZE) {
            if (*gems >= entries[i].price) {
                *gems -= entries[i].price;
                inventory_add(inv, entries[i].item_id, 1);
            }
            break;
        }
    }
}
