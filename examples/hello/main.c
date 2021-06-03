#include <tea.h>
#include <stdio.h>

int main(int argc, char ** argv) {
    te_config_t conf = tea_config_init("hello", 640, 380);
    tea_init(&conf);

    te_point_t center;
    tea_window_size(&center, 0, 0);
    center.x /= 2;
    center.y /= 2;

    while (!tea_should_close()) {
        tea_begin();
        tea_color(TEA_WHITE);
        tea_circle(center.x, center.y, 16);
        tea_end();
    }
    return tea_deinit();
}
