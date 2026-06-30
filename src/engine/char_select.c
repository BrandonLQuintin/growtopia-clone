#include "char_select.h"
#include <string.h>
#include <stdio.h>

#define CS_ROW_H 30
#define CS_ROW_W 440
#define CS_VISIBLE 6
#define CS_BTN_W 26

static int point_in_rect(int px, int py, int rx, int ry, int rw, int rh)
{
    return px >= rx && px < rx + rw && py >= ry && py < ry + rh;
}

static void clamp_scroll(CharSelect *cs)
{
    int max_scroll = cs->count - CS_VISIBLE;
    if (max_scroll < 0) max_scroll = 0;
    if (cs->scroll < 0) cs->scroll = 0;
    if (cs->scroll > max_scroll) cs->scroll = max_scroll;
}

static void commit_input(CharSelect *cs);

void char_select_init(CharSelect *cs)
{
    memset(cs, 0, sizeof(*cs));
    cs->selected = -1;
    cs->hovered = -1;
    cs->renaming = -1;
    cs->deleting = -1;
    cs->creating = 0;
    char_select_refresh(cs);
}

void char_select_refresh(CharSelect *cs)
{
    cs->count = 0;
    character_list(cs->names, &cs->count, CHAR_LIST_MAX);
    for (int i = 0; i < cs->count; i++) {
        Character c;
        memset(&c, 0, sizeof(c));
        char path[256];
        character_path(cs->names[i], path, sizeof(path));
        if (character_load(&c, path) == 0) {
            cs->gems_snap[i] = c.gems;
            snprintf(cs->last_world_snap[i], sizeof(cs->last_world_snap[i]), "%s", c.last_world);
        } else {
            cs->gems_snap[i] = 0;
            cs->last_world_snap[i][0] = '\0';
        }
    }
    clamp_scroll(cs);
}

void char_select_handle_event(CharSelect *cs, SDL_Event *e)
{
    if (e->type == SDL_MOUSEBUTTONDOWN && e->button.button == SDL_BUTTON_LEFT)
        cs->mouse_clicked = 1;

    if (e->type == SDL_MOUSEWHEEL) {
        cs->scroll -= e->wheel.y;
        clamp_scroll(cs);
    }

    if (cs->deleting >= 0) {
        if (e->type == SDL_KEYDOWN) {
            if (e->key.keysym.sym == SDLK_y) {
                character_delete(cs->names[cs->deleting]);
                cs->deleting = -1;
                char_select_refresh(cs);
            } else if (e->key.keysym.sym == SDLK_n || e->key.keysym.sym == SDLK_ESCAPE) {
                cs->deleting = -1;
            }
        }
        return;
    }

    int text_mode = cs->creating || cs->renaming >= 0;
    if (text_mode) {
        if (e->type == SDL_KEYDOWN) {
            if (e->key.keysym.sym == SDLK_BACKSPACE && cs->input_cursor > 0) {
                cs->input_cursor--;
                cs->input_text[cs->input_cursor] = '\0';
            } else if (e->key.keysym.sym == SDLK_RETURN) {
                commit_input(cs);
            } else if (e->key.keysym.sym == SDLK_ESCAPE) {
                cs->creating = 0;
                cs->renaming = -1;
                cs->input_text[0] = '\0';
                cs->input_cursor = 0;
            }
        } else if (e->type == SDL_TEXTINPUT) {
            if (cs->input_cursor < CHAR_NAME_MAX) {
                char c = e->text.text[0];
                if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) {
                    if (c >= 'A' && c <= 'Z') c = c - 'A' + 'a';
                    cs->input_text[cs->input_cursor] = c;
                    cs->input_cursor++;
                }
            }
        }
    }
}

static void commit_input(CharSelect *cs)
{
    if (cs->input_cursor == 0) {
        cs->creating = 0;
        cs->renaming = -1;
        return;
    }
    if (!character_name_valid(cs->input_text)) {
        cs->creating = 0;
        cs->renaming = -1;
        cs->input_text[0] = '\0';
        cs->input_cursor = 0;
        return;
    }

    if (cs->creating) {
        if (character_exists(cs->input_text)) {
            cs->creating = 0;
            cs->input_text[0] = '\0';
            cs->input_cursor = 0;
            return;
        }
        Character c;
        character_apply_defaults(&c);
        snprintf(c.name, sizeof(c.name), "%s", cs->input_text);
        char path[256];
        character_path(cs->input_text, path, sizeof(path));
        character_save(&c, path);
        char_select_refresh(cs);
        for (int i = 0; i < cs->count; i++) {
            if (strcmp(cs->names[i], cs->input_text) == 0) {
                cs->selected = i;
                break;
            }
        }
        cs->creating = 0;
        cs->input_text[0] = '\0';
        cs->input_cursor = 0;
        return;
    }

    if (cs->renaming >= 0) {
        const char *old = cs->names[cs->renaming];
        if (strcmp(old, cs->input_text) != 0) {
            if (character_exists(cs->input_text)) {
                cs->renaming = -1;
                cs->input_text[0] = '\0';
                cs->input_cursor = 0;
                return;
            }
            character_rename(old, cs->input_text);
        }
        char_select_refresh(cs);
        cs->renaming = -1;
        cs->input_text[0] = '\0';
        cs->input_cursor = 0;
    }
}

void char_select_update(CharSelect *cs, float dt)
{
    cs->cursor_timer += dt;
    if (cs->cursor_timer >= 1.0f) cs->cursor_timer -= 1.0f;

    if (cs->deleting >= 0) {
        cs->mouse_clicked = 0;
        return;
    }

    int mx, my;
    SDL_GetMouseState(&mx, &my);

    int panel_x = (g_screen_w - CS_ROW_W) / 2;
    int list_y = 100;
    int body_w = CS_ROW_W - 2 * (CS_BTN_W + 4);
    int text_mode = cs->creating || cs->renaming >= 0;

    if (!text_mode) {
        int new_btn_w = 160, new_btn_h = 28;
        int new_x = panel_x + (CS_ROW_W - new_btn_w) / 2;
        int new_y = list_y + CS_VISIBLE * CS_ROW_H + 16;
        if (point_in_rect(mx, my, new_x, new_y, new_btn_w, new_btn_h) && cs->mouse_clicked) {
            cs->creating = 1;
            cs->input_text[0] = '\0';
            cs->input_cursor = 0;
        }
    }

    cs->hovered = -1;
    for (int i = 0; i < CS_VISIBLE; i++) {
        int idx = cs->scroll + i;
        if (idx >= cs->count) break;
        int ry = list_y + i * CS_ROW_H;

        if (!text_mode) {
            if (point_in_rect(mx, my, panel_x, ry, body_w, CS_ROW_H)) {
                cs->hovered = idx;
                if (cs->mouse_clicked) cs->selected = idx;
            }
            int rx_btn = panel_x + body_w + 4;
            if (point_in_rect(mx, my, rx_btn, ry, CS_BTN_W, CS_ROW_H) && cs->mouse_clicked) {
                snprintf(cs->input_text, sizeof(cs->input_text), "%s", cs->names[idx]);
                cs->input_cursor = (int)strlen(cs->input_text);
                cs->renaming = idx;
            }
            int dx_btn = rx_btn + CS_BTN_W + 4;
            if (point_in_rect(mx, my, dx_btn, ry, CS_BTN_W, CS_ROW_H) && cs->mouse_clicked) {
                cs->deleting = idx;
            }
        }
    }

    cs->mouse_clicked = 0;
}

void char_select_render(CharSelect *cs, Renderer *r)
{
    renderer_begin_ui(r);
    renderer_draw_rect(r, 0, 0, g_screen_w, g_screen_h, 0.05f, 0.05f, 0.15f, 1.0f);

    int panel_x = (g_screen_w - CS_ROW_W) / 2;
    int list_y = 100;
    int body_w = CS_ROW_W - 2 * (CS_BTN_W + 4);

    const char *title = "SELECT CHARACTER";
    int tw = renderer_text_width(r, title, 3.0f);
    renderer_draw_text(r, title, panel_x + (CS_ROW_W - tw) / 2, list_y - 50, 3.0f, 1.0f, 1.0f, 1.0f);

    for (int i = 0; i < CS_VISIBLE; i++) {
        int idx = cs->scroll + i;
        if (idx >= cs->count) break;
        int ry = list_y + i * CS_ROW_H;

        float br = 0.15f, bg = 0.15f, bb = 0.22f;
        if (cs->hovered == idx) { br = 0.25f; bg = 0.25f; bb = 0.38f; }
        renderer_draw_rect(r, panel_x, ry, body_w, CS_ROW_H - 2, br, bg, bb, 1.0f);

        if (cs->renaming != idx) {
            const char *nm = cs->names[idx];
            renderer_draw_text(r, nm, panel_x + 8, ry + 8, 1.3f, 1.0f, 1.0f, 1.0f);

            char gbuf[32];
            snprintf(gbuf, sizeof(gbuf), "%d g", cs->gems_snap[idx]);
            int gw = renderer_text_width(r, gbuf, 1.1f);
            renderer_draw_text(r, gbuf, panel_x + body_w - gw - 8, ry + 9, 1.1f, 1.0f, 0.85f, 0.3f);

            if (cs->last_world_snap[idx][0]) {
                char lbuf[48];
                snprintf(lbuf, sizeof(lbuf), "last: %s", cs->last_world_snap[idx]);
                int nw = renderer_text_width(r, cs->names[idx], 1.3f);
                renderer_draw_text(r, lbuf, panel_x + 8 + nw + 16, ry + 10, 1.0f, 0.6f, 0.6f, 0.6f);
            }
        }

        int rx_btn = panel_x + body_w + 4;
        renderer_draw_rect(r, rx_btn, ry, CS_BTN_W, CS_ROW_H - 2, 0.2f, 0.2f, 0.3f, 1.0f);
        int rw = renderer_text_width(r, "R", 1.2f);
        renderer_draw_text(r, "R", rx_btn + (CS_BTN_W - rw) / 2, ry + 8, 1.2f, 1.0f, 1.0f, 1.0f);

        int dx_btn = rx_btn + CS_BTN_W + 4;
        renderer_draw_rect(r, dx_btn, ry, CS_BTN_W, CS_ROW_H - 2, 0.35f, 0.15f, 0.15f, 1.0f);
        int xw = renderer_text_width(r, "X", 1.2f);
        renderer_draw_text(r, "X", dx_btn + (CS_BTN_W - xw) / 2, ry + 8, 1.2f, 1.0f, 1.0f, 1.0f);
    }

    int new_btn_w = 160, new_btn_h = 28;
    int new_x = panel_x + (CS_ROW_W - new_btn_w) / 2;
    int new_y = list_y + CS_VISIBLE * CS_ROW_H + 16;

    if (cs->creating) {
        renderer_draw_rect(r, new_x, new_y, new_btn_w, new_btn_h, 0.2f, 0.2f, 0.3f, 1.0f);
        renderer_draw_text(r, cs->input_text, new_x + 8, new_y + 8, 1.3f, 1.0f, 1.0f, 1.0f);
        if (cs->cursor_timer < 0.5f) {
            int cw = renderer_text_width(r, cs->input_text, 1.3f);
            renderer_draw_rect(r, new_x + 8 + cw, new_y + 6, 2, 18, 1.0f, 1.0f, 1.0f, 1.0f);
        }
    } else {
        renderer_draw_rect(r, new_x, new_y, new_btn_w, new_btn_h, 0.2f, 0.5f, 0.2f, 1.0f);
        int lw = renderer_text_width(r, "New Character", 1.3f);
        renderer_draw_text(r, "New Character", new_x + (new_btn_w - lw) / 2, new_y + 8, 1.3f, 1.0f, 1.0f, 1.0f);
    }

    if (cs->renaming >= 0) {
        for (int i = 0; i < CS_VISIBLE; i++) {
            int idx = cs->scroll + i;
            if (idx == cs->renaming) {
                int ry = list_y + i * CS_ROW_H;
                renderer_draw_rect(r, panel_x, ry, body_w, CS_ROW_H - 2, 0.2f, 0.2f, 0.3f, 1.0f);
                renderer_draw_text(r, cs->input_text, panel_x + 8, ry + 8, 1.3f, 1.0f, 1.0f, 1.0f);
                if (cs->cursor_timer < 0.5f) {
                    int cw = renderer_text_width(r, cs->input_text, 1.3f);
                    renderer_draw_rect(r, panel_x + 8 + cw, ry + 6, 2, 18, 1.0f, 1.0f, 1.0f, 1.0f);
                }
                break;
            }
        }
    }

    if (cs->deleting >= 0) {
        renderer_draw_rect(r, 0, 0, g_screen_w, g_screen_h, 0.0f, 0.0f, 0.0f, 0.6f);
        char msg[64];
        snprintf(msg, sizeof(msg), "Delete %s? Y/N", cs->names[cs->deleting]);
        int mw = renderer_text_width(r, msg, 2.0f);
        int bx = (g_screen_w - mw - 24) / 2;
        int by = g_screen_h / 2 - 30;
        renderer_draw_rect(r, bx, by, mw + 24, 48, 0.15f, 0.1f, 0.1f, 1.0f);
        renderer_draw_text(r, msg, bx + 12, by + 14, 2.0f, 1.0f, 1.0f, 1.0f);
    }

    const char *hint = "Click: Play   R: Rename   X: Delete   Wheel: Scroll   ESC: Back/Cancel";
    int hw = renderer_text_width(r, hint, 1.0f);
    renderer_draw_text(r, hint, (g_screen_w - hw) / 2, g_screen_h - 20, 1.0f, 0.5f, 0.5f, 0.5f);

    renderer_end_ui(r);
}
