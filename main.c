#include "include/tea.h"

int main(int argc, char ** argv) {
    te_Config conf = {0};
    tea_config_init(&conf, "Game", 640, 380);
    tea_init(&conf);

    int scale = 1;
    int x, y;
    x = y = 0;

    te_Texture *tex = tea_texture_load("goblin.png");
    te_RenderTarget *target = tea_render_target(160, 95, TEA_RGBA);

    while (!tea_should_close()) {
        tea_begin();

        if (tea_keyboard_was_pressed(TEA_KEY_ESCAPE)) break;

        if (tea_keyboard_is_down(TEA_KEY_LEFT)) x -= 10;
        else if (tea_keyboard_is_down(TEA_KEY_RIGHT)) x += 10;
        
        tea_set_render_target(target);
        tea_clear();
        tea_draw_texture(tex, &tea_rect(x, y, 16*scale, 16*scale), &tea_rect(0, 0, 16, 16));

        tea_draw_circle(tea_point(x+8, y+8), 8);
        tea_set_render_target(NULL);

        tea_draw_texture((te_Texture*)target, &tea_rect(0, 0, 160*4, 95*4), NULL);

        tea_end();
    }

    tea_terminate();

    return 0;
}
