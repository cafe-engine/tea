#include "tea.h"

#include <SDL.h>

int window_moved(int id, int x, int y) {
    printf("kakakakakkaaka: %dx%d\n", x, y);
    return 0;
}

int window_close(int id) {
    printf("closing\n");
    return 0;
}

int window_visible(int wid, int visible) {
    printf("visible: %d\n", visible);
    return 0;
}

int window_mouse(int wid, int enter) {
    printf("mouse entered: %d\n", enter);
    return 0;
}

int keyboard(int wid, int up, int repead, struct te_keysym_t sym) {
    if (!up && sym.scancode == TEA_KEY_A) printf("aaaaah\n");

    return 0;
}

int main(int argc, char ** argv) {
    te_config_t conf = tea_config_init("hello", 640, 380);
    tea_init(&conf);

    tea_event_window_visible(window_visible);
    tea_event_window_mouse(window_mouse);

    tea_event_key(keyboard);

    float x;
    x = 0;

    while (!tea_should_close()) {
        tea_begin();
        tea_clear(0xff0000ff);
        tea_color(TEA_WHITE);

        if (tea_key_down(TEA_KEY_A)) x += 10;

        tea_rect(x, 0, 32, 32);
        tea_circle(x+16, 16, 8);
        tea_print("olar", 0, 0);

        tea_end();
    }

    tea_deinit();

    return 0;
}
