#include "world_select.h"
#include "renderer.h"
#include <SDL2/SDL.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

static int point_in_rect(int px, int py, int rx, int ry, int rw, int rh)
{
    return px >= rx && px < rx + rw && py >= ry && py < ry + rh;
}

void world_select_init(WorldSelect *ws)
{
    memset(ws, 0, sizeof(WorldSelect));
    ws->input_cursor = 0;
    ws->cursor_timer = 0;
    ws->submitted = 0;
    world_select_load_recent(ws);
}

void world_select_handle_event(WorldSelect *ws, SDL_Event *e)
{
    if (e->type == SDL_MOUSEBUTTONDOWN && e->button.button == SDL_BUTTON_LEFT) {
        ws->mouse_clicked = 1;
    }
    if (e->type == SDL_KEYDOWN) {
        if (e->key.keysym.sym == SDLK_BACKSPACE && ws->input_cursor > 0) {
            ws->input_cursor--;
            ws->input_text[ws->input_cursor] = '\0';
        }
        if (e->key.keysym.sym == SDLK_RETURN) {
            if (ws->input_cursor > 0) {
                ws->submitted = 1;
            }
        }
    }
    if (e->type == SDL_TEXTINPUT) {
        if (ws->input_cursor < WORLD_NAME_MAX) {
            char c = e->text.text[0];
            if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) {
                if (c >= 'A' && c <= 'Z') c = c - 'A' + 'a';
                ws->input_text[ws->input_cursor] = c;
                ws->input_cursor++;
            }
        }
    }
}

void world_select_update(WorldSelect *ws, float dt)
{
    ws->cursor_timer += dt;
    if (ws->cursor_timer >= 1.0f) ws->cursor_timer -= 1.0f;

    ws->hovered_recent = -1;
    ws->hovered_enter = 0;

    int mx, my;
    SDL_GetMouseState(&mx, &my);

    int input_w = 300;
    int btn_w = 120;
    int btn_h = 24;
    int panel_w = input_w;
    int panel_x = (g_screen_w - panel_w) / 2;
    int panel_y = g_screen_h / 2 - 80;
    int btn_y = panel_y + 60;

    int enter_x = panel_x + (panel_w - btn_w) / 2;
    int enter_y = btn_y;
    ws->hovered_enter = point_in_rect(mx, my, enter_x, enter_y, btn_w, btn_h);

    if (ws->hovered_enter && ws->mouse_clicked) {
        if (ws->input_cursor > 0) {
            ws->submitted = 1;
        }
    }

    int recent_start_y = btn_y + btn_h + 20;
    int recent_btn_w = 120;
    int recent_btn_h = 28;
    for (int i = 0; i < ws->recent_count; i++) {
        int col = i % 4;
        int row = i / 4;
        int rx = panel_x + col * (recent_btn_w + 8);
        int ry = recent_start_y + row * (recent_btn_h + 4);
        if (point_in_rect(mx, my, rx, ry, recent_btn_w, recent_btn_h)) {
            ws->hovered_recent = i;
            if (ws->mouse_clicked) {
                strncpy(ws->input_text, ws->recent_names[i], WORLD_NAME_MAX);
                ws->input_cursor = (int)strlen(ws->recent_names[i]);
            }
        }
    }

    ws->mouse_clicked = 0;
}

void world_select_render(WorldSelect *ws, Renderer *r)
{
    renderer_begin_ui(r);

    renderer_draw_rect(r, 0, 0, g_screen_w, g_screen_h, 0.05f, 0.05f, 0.15f, 1.0f);

    int input_w = 300;
    int panel_w = input_w;
    int panel_x = (g_screen_w - panel_w) / 2;
    int panel_y = g_screen_h / 2 - 80;

    int title_w = renderer_text_width(r, "SEARCH WORLD", 3.0f);
    renderer_draw_text(r, "SEARCH WORLD",
        panel_x + (panel_w - title_w) / 2, panel_y - 60, 3.0f,
        1.0f, 1.0f, 1.0f);

    renderer_draw_rect(r, panel_x, panel_y, input_w, 40, 0.2f, 0.2f, 0.3f, 1.0f);
    renderer_draw_rect(r, panel_x, panel_y, input_w, 1, 0.5f, 0.5f, 0.6f, 1.0f);
    renderer_draw_rect(r, panel_x, panel_y + 39, input_w, 1, 0.0f, 0.0f, 0.0f, 1.0f);
    renderer_draw_rect(r, panel_x, panel_y, 1, 40, 0.5f, 0.5f, 0.6f, 1.0f);
    renderer_draw_rect(r, panel_x + input_w - 1, panel_y, 1, 40, 0.0f, 0.0f, 0.0f, 1.0f);

    renderer_draw_text(r, ws->input_text, panel_x + 8, panel_y + 10, 2.0f,
        1.0f, 1.0f, 1.0f);

    if (ws->cursor_timer < 0.5f) {
        int text_w = renderer_text_width(r, ws->input_text, 2.0f);
        renderer_draw_rect(r, panel_x + 8 + text_w, panel_y + 8, 2, 24,
            1.0f, 1.0f, 1.0f, 1.0f);
    }

    int btn_w = 120;
    int btn_h = 24;
    int btn_y = panel_y + 60;
    int enter_x = panel_x + (panel_w - btn_w) / 2;
    int enter_y = btn_y;

    float btn_r = 0.2f, btn_g = 0.5f, btn_b = 0.2f;
    if (ws->hovered_enter) { btn_r = 0.3f; btn_g = 0.7f; btn_b = 0.3f; }
    renderer_draw_rect(r, enter_x, enter_y, btn_w, btn_h, btn_r, btn_g, btn_b, 1.0f);
    renderer_draw_rect(r, enter_x, enter_y, btn_w, 1, btn_r + 0.2f, btn_g + 0.2f, btn_b + 0.2f, 1.0f);
    renderer_draw_rect(r, enter_x, enter_y + btn_h - 1, btn_w, 1, 0.0f, 0.0f, 0.0f, 1.0f);
    renderer_draw_rect(r, enter_x, enter_y, 1, btn_h, btn_r + 0.2f, btn_g + 0.2f, btn_b + 0.2f, 1.0f);
    renderer_draw_rect(r, enter_x + btn_w - 1, enter_y, 1, btn_h, 0.0f, 0.0f, 0.0f, 1.0f);

    int enter_text_w = renderer_text_width(r, "ENTER", 1.5f);
    renderer_draw_text(r, "ENTER",
        enter_x + (btn_w - enter_text_w) / 2, enter_y + 4, 1.5f,
        1.0f, 1.0f, 1.0f);

    if (ws->recent_count > 0) {
        int recent_start_y = btn_y + btn_h + 20;
        renderer_draw_text(r, "Recent:",
            panel_x, recent_start_y - 20, 1.5f, 0.7f, 0.7f, 0.7f);

        int recent_btn_w = 120;
        int recent_btn_h = 28;
        for (int i = 0; i < ws->recent_count; i++) {
            int col = i % 4;
            int row = i / 4;
            int rx = panel_x + col * (recent_btn_w + 8);
            int ry = recent_start_y + row * (recent_btn_h + 4);

            float rb_r = 0.2f, rb_g = 0.2f, rb_b = 0.3f;
            if (ws->hovered_recent == i) { rb_r = 0.3f; rb_g = 0.3f; rb_b = 0.5f; }
            renderer_draw_rect(r, rx, ry, recent_btn_w, recent_btn_h, rb_r, rb_g, rb_b, 1.0f);
            renderer_draw_rect(r, rx, ry, recent_btn_w, 1, rb_r + 0.2f, rb_g + 0.2f, rb_b + 0.2f, 1.0f);
            renderer_draw_rect(r, rx, ry + recent_btn_h - 1, recent_btn_w, 1, 0.0f, 0.0f, 0.0f, 1.0f);
            renderer_draw_rect(r, rx, ry, 1, recent_btn_h, rb_r + 0.2f, rb_g + 0.2f, rb_b + 0.2f, 1.0f);
            renderer_draw_rect(r, rx + recent_btn_w - 1, ry, 1, recent_btn_h, 0.0f, 0.0f, 0.0f, 1.0f);

            int tw = renderer_text_width(r, ws->recent_names[i], 1.2f);
            renderer_draw_text(r, ws->recent_names[i],
                rx + (recent_btn_w - tw) / 2, ry + 8, 1.2f,
                1.0f, 1.0f, 1.0f);
        }
    }

    int hint_w = renderer_text_width(r, "ESC: Quit", 1.0f);
    renderer_draw_text(r, "ESC: Quit",
        (g_screen_w - hint_w) / 2, g_screen_h - 24, 1.0f,
        0.5f, 0.5f, 0.5f);

    renderer_end_ui(r);
}

void world_select_load_recent(WorldSelect *ws)
{
    memset(ws->recent_names, 0, sizeof(ws->recent_names));
    ws->recent_count = 0;

    FILE *f = fopen("res/worlds/recent.txt", "r");
    if (!f) return;

    char line[WORLD_NAME_MAX + 2];
    while (ws->recent_count < RECENT_WORLDS_MAX && fgets(line, sizeof(line), f)) {
        int len = (int)strlen(line);
        while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r'))
            len--;
        line[len] = '\0';
        if (len > 0) {
            snprintf(ws->recent_names[ws->recent_count], WORLD_NAME_MAX + 1, "%s", line);
            ws->recent_count++;
        }
    }
    fclose(f);
}

void world_select_add_recent(WorldSelect *ws, const char *name)
{
    int existing = -1;
    for (int i = 0; i < ws->recent_count; i++) {
        if (strcmp(ws->recent_names[i], name) == 0) {
            existing = i;
            break;
        }
    }

    if (existing >= 0) {
        char saved[WORLD_NAME_MAX + 1];
        snprintf(saved, WORLD_NAME_MAX + 1, "%s", ws->recent_names[existing]);
        for (int i = existing; i > 0; i--) {
            snprintf(ws->recent_names[i], WORLD_NAME_MAX + 1, "%s", ws->recent_names[i - 1]);
        }
        snprintf(ws->recent_names[0], WORLD_NAME_MAX + 1, "%s", saved);
    } else {
        if (ws->recent_count >= RECENT_WORLDS_MAX) {
            ws->recent_count = RECENT_WORLDS_MAX - 1;
        }
        for (int i = ws->recent_count; i > 0; i--) {
            snprintf(ws->recent_names[i], WORLD_NAME_MAX + 1, "%s", ws->recent_names[i - 1]);
        }
        snprintf(ws->recent_names[0], WORLD_NAME_MAX + 1, "%s", name);
        ws->recent_count++;
    }

    FILE *f = fopen("res/worlds/recent.txt", "w");
    if (!f) return;
    for (int i = 0; i < ws->recent_count; i++) {
        fprintf(f, "%s\n", ws->recent_names[i]);
    }
    fclose(f);
}
