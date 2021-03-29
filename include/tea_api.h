#ifndef TEA_API_H
#define TEA_API_H

#include "tea.h"
#include <SDL.h>

#include "cstar.h"

#define tea() (&_tea_ctx)
#define render() (&tea()->render)
#define window() (&tea()->window)
#define event() (&tea()->event)

#define MAX_FONT_CHAR 256

#define pixel_format(id) (tea()->mode.pixel_format[id])

#define TEA_ASSERT(expr, str) (void)
#define NOT_SUPPORTED(fn, res) fn { \
    fprintf(stderr, "%s: not supported by this render\n", __PRETTY_FUNCTION__); \
    return (res); \
}

#define MAX(a,b) ((a) < (b) ? (b) : (a))
#define MIN(a,b) ((a) > (b) ? (b) : (a))

typedef struct {
    int pixel_format[TEA_PIXELFORMAT_COUNT];
} te_RenderMode;

typedef struct {
    te_Color draw_color;
    te_Color clear_color;
    int draw_mode;

    te_Texture *tex;
    te_Transform transform;
} te_RenderStat;

struct te_Window {
    int flags;
    void *handle;
    unsigned width, height;
};

struct te_Render {
    int flags;
    te_RenderStat stat;
    void *handle;
};

struct te_Input {
    struct { const te_Byte *state; te_Byte old_state[TEA_KEY_COUNT]; } keyboard;
    struct {
        te_Byte state[TEA_BUTTON_COUNT];
        te_Byte old_state[TEA_BUTTON_COUNT];
        TEA_TNUM x, y;
        TEA_TNUM scrollx, scrolly;
    } mouse;
    struct {
        float laxis[2];
        float raxis[2];
        te_Byte button[16];
    } joystick[MAX_JID];
};

struct te_Timer {
    unsigned int prev_time, current_time;
    float delta;
    unsigned int prev_fps_time, frame, fps;
};

struct Tea {
    int running;
    te_RenderMode mode;

    te_Render render;
    te_Window window;
    SDL_Event event;

    char error_buf[256];
    struct te_Input input;
    struct te_Timer timer;
};

extern Tea _tea_ctx;

/* Render Mode */
TEA_API int tea_init_render_mode(te_RenderMode *mode);

/* Window */
TEA_API int tea_window_init(te_Window *window, const char *title, int width, int height, int flags);
TEA_API int tea_window_deinit(te_Window *window);

TEA_API int tea_window_size(te_Point *out);

TEA_API int tea_window_should_close(te_Window *window);
TEA_API int tea_window_swap(te_Window *window);

/* Render */
TEA_API int tea_render_init(te_Render *r, te_Window *window, int flags);
TEA_API int tea_render_deinit(te_Render *r);

TEA_API int tea_render_begin(te_Render *r);
TEA_API int tea_render_end(te_Render *r);

TEA_API int tea_set_render_target(te_Render *render, te_Texture *tex);
TEA_API int tea_render_swap(te_Render *r);

TEA_API int tea_render_clear(te_Render *r);
TEA_API int tea_render_color(te_Render *r, te_Color col);
TEA_API int tea_render_mode(te_Render *r, int mode);

/* Texture */
TEA_API void* tea_alloc_texture();
TEA_API int tea_init_texture(te_Texture *tex, void *data, int w, int h, int format, int usage);
TEA_API int tea_deinit_texture(te_Texture *tex);

/* Debug */
TEA_API int tea_error(const char *fmt, ...);
#endif /* TEA_API_H */
