#include "ui.h"
#include "../game/store.h"
#include "../game/player.h"
#include <string.h>
#include <stdio.h>

#define SLOT_SIZE 48
#define HOTBAR_SLOTS 9
#define INV_COLS 9
#define INV_ROWS 4
#define STORE_COLS 5
#define STORE_TAB_COUNT 5

static const char *store_tab_names[STORE_TAB_COUNT] = {
    "BLOCKS", "SEEDS", "TOOLS", "CLOTHES", "SPECIAL"
};

static Store s_store;
static int s_store_init = 0;

static void ensure_store(void)
{
    if (!s_store_init) {
        store_init(&s_store);
        s_store_init = 1;
    }
}

void ui_init(UI *ui)
{
    memset(ui, 0, sizeof(UI));
    ui->hotbar_selection = 0;
    ui->store_category = 0;
    ui->state = UI_STATE_NONE;
    ui->selected_slot = -1;
    ui->drag_from_slot = -1;
    ui->tooltip_slot = -1;
}

static int point_in_rect(int px, int py, int rx, int ry, int rw, int rh)
{
    return px >= rx && px < rx + rw && py >= ry && py < ry + rh;
}

void ui_update(UI *ui, Input *input, Renderer *renderer)
{
    (void)renderer;
    int mx = input->mouse_x;
    int my = input->mouse_y;

    for (int i = 0; i < ui->slot_count; i++) {
        UISlot *s = &ui->slots[i];
        s->hovered = point_in_rect(mx, my, s->x, s->y, s->size, s->size);
    }

    if (!input_is_mouse_clicked(input, 1)) return;

    if (ui->state == UI_STATE_INVENTORY) {
        for (int i = 0; i < ui->slot_count; i++) {
            if (ui->slots[i].hovered) {
                if (ui->selected_slot < 0) {
                    ui->selected_slot = i;
                    ui->drag_from_slot = -1;
                } else if (ui->selected_slot == i) {
                    ui->selected_slot = -1;
                    ui->drag_from_slot = -1;
                } else {
                    ui->drag_from_slot = ui->selected_slot;
                    ui->selected_slot = i;
                }
                break;
            }
        }
        int close_x = (g_screen_w + INV_COLS * SLOT_SIZE) / 2 + 4;
        int close_y = (g_screen_h - INV_ROWS * SLOT_SIZE) / 2 - 44;
        if (point_in_rect(mx, my, close_x, close_y, 24, 24)) {
            ui_close_all(ui);
        }
    }

    if (ui->state == UI_STATE_STORE) {
        ensure_store();
        int tab_w = 110;
        int tab_h = 28;
        int tabs_total = STORE_TAB_COUNT * tab_w;
        int tabs_x = (g_screen_w - tabs_total) / 2;
        int tabs_y = 60 + 36;
        for (int i = 0; i < STORE_TAB_COUNT; i++) {
            if (point_in_rect(mx, my, tabs_x + i * tab_w, tabs_y, tab_w, tab_h)) {
                ui->store_category = i;
                break;
            }
        }
        int panel_w = STORE_COLS * 100 + 40;
        int close_x = (g_screen_w + panel_w) / 2 - 28;
        int close_y = 60 + 6;
        if (point_in_rect(mx, my, close_x, close_y, 24, 24)) {
            ui_close_all(ui);
        }
    }
}

void ui_render(UI *ui, Renderer *renderer)
{
    (void)ui;
    (void)renderer;
}

void ui_toggle_inventory(UI *ui)
{
    if (ui->state == UI_STATE_INVENTORY) {
        ui->state = UI_STATE_NONE;
    } else {
        ui->state = UI_STATE_INVENTORY;
    }
    ui->selected_slot = -1;
    ui->drag_from_slot = -1;
}

void ui_toggle_store(UI *ui)
{
    if (ui->state == UI_STATE_STORE) {
        ui->state = UI_STATE_NONE;
    } else {
        ui->state = UI_STATE_STORE;
    }
    ui->store_category = 0;
    ui->selected_slot = -1;
    ui->drag_from_slot = -1;
}

void ui_close_all(UI *ui)
{
    ui->state = UI_STATE_NONE;
    ui->selected_slot = -1;
    ui->drag_from_slot = -1;
}

int ui_get_hotbar_selection(UI *ui)
{
    return ui->hotbar_selection;
}

static void render_slot_bg(Renderer *r, int x, int y, int size, int selected, int hovered)
{
    renderer_draw_rect(r, x, y, size, size, 0.15f, 0.15f, 0.15f, 0.9f);
    if (selected) {
        renderer_draw_rect(r, x - 2, y - 2, size + 4, size + 4, 1.0f, 1.0f, 0.0f, 1.0f);
        renderer_draw_rect(r, x, y, size, size, 0.15f, 0.15f, 0.15f, 0.9f);
    } else if (hovered) {
        renderer_draw_rect(r, x - 1, y - 1, size + 2, size + 2, 0.7f, 0.7f, 0.7f, 1.0f);
        renderer_draw_rect(r, x, y, size, size, 0.15f, 0.15f, 0.15f, 0.9f);
    }
    renderer_draw_rect(r, x, y, size, 1, 0.5f, 0.5f, 0.5f, 1.0f);
    renderer_draw_rect(r, x, y + size - 1, size, 1, 0.0f, 0.0f, 0.0f, 1.0f);
    renderer_draw_rect(r, x, y, 1, size, 0.5f, 0.5f, 0.5f, 1.0f);
    renderer_draw_rect(r, x + size - 1, y, 1, size, 0.0f, 0.0f, 0.0f, 1.0f);
}

static void render_slot_item(Renderer *r, int x, int y, int size, int item_id, int count)
{
    if (item_id == 0) return;
    int cr, cg, cb;
    item_get_color((uint16_t)item_id, &cr, &cg, &cb);
    int pad = 6;
    renderer_draw_rect(r, x + pad, y + pad, size - pad * 2, size - pad * 2,
        cr / 255.0f, cg / 255.0f, cb / 255.0f, 1.0f);
    if (count > 0) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%d", count);
        int tw = renderer_text_width(r, buf, 1.0f);
        renderer_draw_rect(r, x + size - tw - 6, y + size - 12, tw + 4, 11, 0.0f, 0.0f, 0.0f, 0.6f);
        renderer_draw_text(r, buf, x + size - tw - 4, y + size - 11, 1.0f, 1.0f, 1.0f, 1.0f);
    }
}

void ui_render_hotbar(UI *ui, Renderer *renderer, uint16_t *hotbar_items, int *hotbar_counts, int selected)
{
    int total_w = HOTBAR_SLOTS * SLOT_SIZE;
    int start_x = (g_screen_w - total_w) / 2;
    int start_y = g_screen_h - SLOT_SIZE - 8;

    renderer_draw_rect(renderer, start_x - 4, start_y - 4, total_w + 8, SLOT_SIZE + 8,
        0.0f, 0.0f, 0.0f, 0.5f);

    ui->slot_count = 0;
    for (int i = 0; i < HOTBAR_SLOTS && ui->slot_count < UI_MAX_SLOTS; i++) {
        int sx = start_x + i * SLOT_SIZE;
        int sy = start_y;
        UISlot *s = &ui->slots[ui->slot_count];
        s->x = sx;
        s->y = sy;
        s->size = SLOT_SIZE;
        s->item_id = hotbar_items[i];
        s->count = hotbar_counts[i];
        s->selected = (i == selected);
        ui->slot_count++;

        render_slot_bg(renderer, sx, sy, SLOT_SIZE, i == selected, s->hovered);
        render_slot_item(renderer, sx, sy, SLOT_SIZE, hotbar_items[i], hotbar_counts[i]);
    }

    for (int i = 0; i < ui->slot_count; i++) {
        if (ui->slots[i].hovered && ui->slots[i].item_id != 0) {
            const char *name = item_get_name((uint16_t)ui->slots[i].item_id);
            if (name) {
                int tw = renderer_text_width(renderer, name, 1.5f);
                int th = 12;
                int tx = ui->slots[i].x + SLOT_SIZE / 2 - tw / 2;
                int ty = ui->slots[i].y - th - 8;
                if (tx < 2) tx = 2;
                if (tx + tw + 6 > g_screen_w) tx = g_screen_w - tw - 6;
                renderer_draw_rect(renderer, tx - 4, ty - 2, tw + 8, th + 6, 0.0f, 0.0f, 0.0f, 0.85f);
                renderer_draw_text(renderer, name, tx, ty, 1.5f, 1.0f, 1.0f, 1.0f);
            }
            break;
        }
    }
}

void ui_render_inventory_screen(UI *ui, Renderer *renderer, uint16_t *inv_items, int *inv_counts, int inv_size)
{
    renderer_draw_rect(renderer, 0, 0, g_screen_w, g_screen_h, 0.0f, 0.0f, 0.0f, 0.6f);

    int grid_w = INV_COLS * SLOT_SIZE;
    int grid_h = INV_ROWS * SLOT_SIZE;
    int panel_pad = 10;
    int title_h = 36;
    int panel_w = grid_w + panel_pad * 2;
    int panel_h = grid_h + title_h + panel_pad * 2;
    int panel_x = (g_screen_w - panel_w) / 2;
    int panel_y = (g_screen_h - panel_h) / 2;

    renderer_draw_rect(renderer, panel_x, panel_y, panel_w, panel_h,
        0.1f, 0.1f, 0.1f, 0.95f);
    renderer_draw_rect(renderer, panel_x, panel_y, panel_w, 2, 0.4f, 0.4f, 0.4f, 1.0f);
    renderer_draw_rect(renderer, panel_x, panel_y + panel_h - 2, panel_w, 2, 0.0f, 0.0f, 0.0f, 1.0f);
    renderer_draw_rect(renderer, panel_x, panel_y, 2, panel_h, 0.4f, 0.4f, 0.4f, 1.0f);
    renderer_draw_rect(renderer, panel_x + panel_w - 2, panel_y, 2, panel_h, 0.0f, 0.0f, 0.0f, 1.0f);

    int title_w = renderer_text_width(renderer, "Inventory", 2.0f);
    renderer_draw_text(renderer, "Inventory",
        panel_x + (panel_w - title_w) / 2, panel_y + 6, 2.0f, 1.0f, 1.0f, 1.0f);

    int close_x = panel_x + panel_w - 28;
    int close_y = panel_y + 6;
    renderer_draw_rect(renderer, close_x, close_y, 20, 20, 0.6f, 0.15f, 0.15f, 1.0f);
    renderer_draw_text(renderer, "X", close_x + 6, close_y + 3, 1.5f, 1.0f, 1.0f, 1.0f);

    int grid_x = panel_x + panel_pad;
    int grid_y = panel_y + title_h + panel_pad;

    ui->slot_count = 0;
    for (int i = 0; i < inv_size && ui->slot_count < UI_MAX_SLOTS; i++) {
        int col = i % INV_COLS;
        int row = i / INV_COLS;
        int sx = grid_x + col * SLOT_SIZE;
        int sy = grid_y + row * SLOT_SIZE;

        UISlot *s = &ui->slots[ui->slot_count];
        s->x = sx;
        s->y = sy;
        s->size = SLOT_SIZE;
        s->item_id = inv_items[i];
        s->count = inv_counts[i];
        s->selected = (ui->drag_from_slot == i);
        ui->slot_count++;

        int is_sel = (ui->drag_from_slot == i);
        render_slot_bg(renderer, sx, sy, SLOT_SIZE, is_sel, s->hovered);
        render_slot_item(renderer, sx, sy, SLOT_SIZE, inv_items[i], inv_counts[i]);
    }

    for (int i = 0; i < ui->slot_count; i++) {
        if (ui->slots[i].hovered && ui->slots[i].item_id != 0) {
            const char *name = item_get_name((uint16_t)ui->slots[i].item_id);
            if (name) {
                char label[64];
                if (item_is_seed((uint16_t)ui->slots[i].item_id)) {
                    snprintf(label, sizeof(label), "%s (Seed)", name);
                } else {
                    snprintf(label, sizeof(label), "%s", name);
                }
                int tw = renderer_text_width(renderer, label, 1.5f);
                int th = 12;
                int tx = ui->slots[i].x + SLOT_SIZE / 2 - tw / 2;
                int ty = ui->slots[i].y - th - 8;
                if (tx < 2) tx = 2;
                if (tx + tw + 6 > g_screen_w) tx = g_screen_w - tw - 6;
                if (ty < 2) ty = ui->slots[i].y + SLOT_SIZE + 4;
                renderer_draw_rect(renderer, tx - 4, ty - 2, tw + 8, th + 6, 0.0f, 0.0f, 0.0f, 0.85f);
                renderer_draw_text(renderer, label, tx, ty, 1.5f, 1.0f, 1.0f, 1.0f);
            }
            break;
        }
    }
}

void ui_render_store_screen(UI *ui, Renderer *renderer, int gems)
{
    ensure_store();

    renderer_draw_rect(renderer, 0, 0, g_screen_w, g_screen_h, 0.0f, 0.0f, 0.0f, 0.6f);

    int panel_w = STORE_COLS * 100 + 40;
    int panel_h = 480;
    int panel_x = (g_screen_w - panel_w) / 2;
    int panel_y = 60;

    renderer_draw_rect(renderer, panel_x, panel_y, panel_w, panel_h,
        0.1f, 0.1f, 0.1f, 0.95f);
    renderer_draw_rect(renderer, panel_x, panel_y, panel_w, 2, 0.4f, 0.4f, 0.4f, 1.0f);
    renderer_draw_rect(renderer, panel_x, panel_y + panel_h - 2, panel_w, 2, 0.0f, 0.0f, 0.0f, 1.0f);
    renderer_draw_rect(renderer, panel_x, panel_y, 2, panel_h, 0.4f, 0.4f, 0.4f, 1.0f);
    renderer_draw_rect(renderer, panel_x + panel_w - 2, panel_y, 2, panel_h, 0.0f, 0.0f, 0.0f, 1.0f);

    int title_w = renderer_text_width(renderer, "Store", 2.0f);
    renderer_draw_text(renderer, "Store",
        panel_x + (panel_w - title_w) / 2, panel_y + 6, 2.0f, 1.0f, 1.0f, 1.0f);

    int close_x = panel_x + panel_w - 28;
    int close_y = panel_y + 6;
    renderer_draw_rect(renderer, close_x, close_y, 20, 20, 0.6f, 0.15f, 0.15f, 1.0f);
    renderer_draw_text(renderer, "X", close_x + 6, close_y + 3, 1.5f, 1.0f, 1.0f, 1.0f);

    char gem_buf[32];
    snprintf(gem_buf, sizeof(gem_buf), "Gems: %d", gems);
    int gw = renderer_text_width(renderer, gem_buf, 1.5f);
    renderer_draw_text(renderer, gem_buf, panel_x + panel_w - gw - 16, panel_y + 10, 1.5f,
        1.0f, 0.9f, 0.0f);

    int tab_w = 110;
    int tab_h = 28;
    int tabs_total = STORE_TAB_COUNT * tab_w;
    int tabs_x = (g_screen_w - tabs_total) / 2;
    int tabs_y = panel_y + 36;

    for (int i = 0; i < STORE_TAB_COUNT; i++) {
        int tx = tabs_x + i * tab_w;
        if (i == ui->store_category) {
            renderer_draw_rect(renderer, tx, tabs_y, tab_w, tab_h,
                0.3f, 0.3f, 0.6f, 1.0f);
        } else {
            renderer_draw_rect(renderer, tx, tabs_y, tab_w, tab_h,
                0.2f, 0.2f, 0.2f, 0.9f);
        }
        renderer_draw_rect(renderer, tx, tabs_y, tab_w, 1, 0.5f, 0.5f, 0.5f, 1.0f);
        renderer_draw_rect(renderer, tx, tabs_y + tab_h - 1, tab_w, 1, 0.0f, 0.0f, 0.0f, 1.0f);
        renderer_draw_rect(renderer, tx, tabs_y, 1, tab_h, 0.5f, 0.5f, 0.5f, 1.0f);
        renderer_draw_rect(renderer, tx + tab_w - 1, tabs_y, 1, tab_h, 0.0f, 0.0f, 0.0f, 1.0f);
        int tw = renderer_text_width(renderer, store_tab_names[i], 1.2f);
        renderer_draw_text(renderer, store_tab_names[i],
            tx + (tab_w - tw) / 2, tabs_y + 6, 1.2f, 1.0f, 1.0f, 1.0f);
    }

    StoreEntry *entries;
    int entry_count;
    store_get_items(&s_store, ui->store_category, &entries, &entry_count);

    int items_x = panel_x + 20;
    int items_y = tabs_y + tab_h + 16;
    int cell_w = 100;
    int cell_h = 80;

    for (int i = 0; i < entry_count; i++) {
        int col = i % STORE_COLS;
        int row = i / STORE_COLS;
        int cx = items_x + col * cell_w;
        int cy = items_y + row * cell_h;

        renderer_draw_rect(renderer, cx, cy, SLOT_SIZE, SLOT_SIZE,
            0.15f, 0.15f, 0.15f, 0.9f);
        renderer_draw_rect(renderer, cx, cy, SLOT_SIZE, 1, 0.4f, 0.4f, 0.4f, 1.0f);
        renderer_draw_rect(renderer, cx, cy + SLOT_SIZE - 1, SLOT_SIZE, 1, 0.0f, 0.0f, 0.0f, 1.0f);
        renderer_draw_rect(renderer, cx, cy, 1, SLOT_SIZE, 0.4f, 0.4f, 0.4f, 1.0f);
        renderer_draw_rect(renderer, cx + SLOT_SIZE - 1, cy, 1, SLOT_SIZE, 0.0f, 0.0f, 0.0f, 1.0f);

        int cr, cg, cb;
        item_get_color(entries[i].item_id, &cr, &cg, &cb);
        renderer_draw_rect(renderer, cx + 6, cy + 6, SLOT_SIZE - 12, SLOT_SIZE - 12,
            cr / 255.0f, cg / 255.0f, cb / 255.0f, 1.0f);

        const char *name = item_get_name(entries[i].item_id);
        if (name) {
            int max_chars = 10;
            char truncated[32];
            int len = 0;
            while (name[len] && len < max_chars && len < 30) {
                truncated[len] = name[len];
                len++;
            }
            truncated[len] = '\0';
            renderer_draw_text(renderer, truncated, cx, cy + SLOT_SIZE + 2, 1.0f,
                0.8f, 0.8f, 0.8f);
        }

        char price_buf[16];
        snprintf(price_buf, sizeof(price_buf), "%dg", entries[i].price);
        renderer_draw_text(renderer, price_buf, cx + SLOT_SIZE + 4, cy + SLOT_SIZE / 2 - 6,
            1.0f, 1.0f, 0.9f, 0.0f);
    }
}

void ui_render_hud(UI *ui, Renderer *renderer, int gems, int health)
{
    (void)ui;

    int bar_w = 240;
    int bar_h = 22;
    int bar_x = 12;
    int bar_y = 12;

    renderer_draw_rect(renderer, bar_x, bar_y, bar_w, bar_h, 0.2f, 0.2f, 0.2f, 0.8f);
    renderer_draw_rect(renderer, bar_x, bar_y, bar_w, 1, 0.4f, 0.4f, 0.4f, 1.0f);
    renderer_draw_rect(renderer, bar_x, bar_y + bar_h - 1, bar_w, 1, 0.0f, 0.0f, 0.0f, 1.0f);
    renderer_draw_rect(renderer, bar_x, bar_y, 1, bar_h, 0.4f, 0.4f, 0.4f, 1.0f);
    renderer_draw_rect(renderer, bar_x + bar_w - 1, bar_y, 1, bar_h, 0.0f, 0.0f, 0.0f, 1.0f);

    if (health > 0 && MAX_HEALTH > 0) {
        int fill_w = (bar_w * health) / MAX_HEALTH;
        if (fill_w > bar_w) fill_w = bar_w;
        float hp_ratio = (float)health / MAX_HEALTH;
        float r = 1.0f - hp_ratio;
        float g = hp_ratio;
        renderer_draw_rect(renderer, bar_x + 1, bar_y + 1, fill_w - 2, bar_h - 2, r, g, 0.0f, 0.9f);
    }

    char hp_buf[16];
    snprintf(hp_buf, sizeof(hp_buf), "%d/%d", health, MAX_HEALTH);
    int hpw = renderer_text_width(renderer, hp_buf, 1.5f);
    renderer_draw_text(renderer, hp_buf, bar_x + (bar_w - hpw) / 2, bar_y + 3, 1.5f, 1.0f, 1.0f, 1.0f);

    int gem_size = 20;
    int gem_x = g_screen_w - 180;
    int gem_y = 12;
    renderer_draw_rect(renderer, gem_x, gem_y, gem_size, gem_size, 1.0f, 0.85f, 0.0f, 1.0f);
    renderer_draw_rect(renderer, gem_x + 4, gem_y - 4, gem_size - 8, 4, 1.0f, 0.85f, 0.0f, 1.0f);
    renderer_draw_rect(renderer, gem_x + 4, gem_y + gem_size, gem_size - 8, 4, 1.0f, 0.85f, 0.0f, 1.0f);

    char gem_buf[16];
    snprintf(gem_buf, sizeof(gem_buf), "%d", gems);
    renderer_draw_text(renderer, gem_buf, gem_x + gem_size + 8, gem_y + 2, 2.0f, 1.0f, 0.9f, 0.0f);
}
