#include <tea.h>
#include <stdio.h>

int main(int argc, char ** argv) {
    te_config_t conf = tea_config_init("hello", 640, 380);
    tea_init(&conf);

    te_point_t center;
    tea_window_size(&center, 0, 0);
    center.x /= 2;
    center.y /= 2;

    te_joystick_t *joy = tea_joystick(0);
    printf("%d\n", tea_joystick_is_gamepad(joy));
    void *pad = tea_joystick_gamepad(joy);

    printf("%p %p\n", joy, pad);

    while (!tea_should_close()) {
        tea_begin();
        tea_clear(TEA_COLOR(75, 90, 90, 255));
        tea_color(TEA_WHITE);
        tea_line(0, 0, 35, 98);
        tea_mode(1);
        tea_rect(0, 0, 32, 32);
        tea_circle(center.x, center.y, 16);
        tea_end();
    }
    return tea_deinit();
}
