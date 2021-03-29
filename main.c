#include "include/tea.h"
#include <stdio.h>

int main(int argc, char ** argv) {
    te_Config conf = {0};
    tea_config_init(&conf, NULL, 160, 95);
    conf.window_flags = TEA_WINDOW_RESIZABLE;
    if (!tea_init(&conf)) {
        printf("failed to init Tea\n");
        return 0;
    }

    te_Texture *tex = tea_load_texture("goblin.png", 0);
    te_Font *font = tea_load_font("extrude.ttf", 16);
    te_Texture *canvas = tea_create_texture(NULL, 160, 95, TEA_RGBA, TEA_TEXTURE_TARGET);

    float time = 0;
    int frame = 0;

    while (!tea_should_close()) {
        tea_begin();

        tea_set_target(canvas);
        tea_clear();
        tea_draw_mode(TEA_LINE);

        tea_draw_mode(TEA_FILL);
        tea_circle(TEA_POINT(16, 16), 8);

        time += tea_delta()*8;
        if (time > 1) {
            frame += 1;
            if (frame > 4) frame = 0;
            time = 0;
        }
        te_Rect r = TEA_RECT(frame*16, 0, 16, 16);
        tea_texture(tex, &TEA_RECT(0, 0, 16, 16), &r);
        tea_texture(tex, &TEA_RECT(0, 16, 16, 16), &TEA_RECT(32, 0, 16, 16));

        te_Point mpos;
        tea_mouse_pos(&mpos.x, &mpos.y);

        tea_line(TEA_POINT(0, 0), mpos);
        tea_print(font, "ok", mpos.x, mpos.y);
        tea_set_target(NULL);

        tea_texture(canvas, NULL, NULL);

        tea_end();
    }

    tea_quit();

    return 0;
}

#ifdef CAFE_ENGINE
#define CSTAR_IMPLEMENTATION
#include "cstar.h"
#endif
