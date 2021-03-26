#ifndef TEA_API_H
#define TEA_API_H

#include "tea.h"

#define tea() (&_tea_ctx)
#define window() tea()->_window
#define render() tea()->_render

#define render_stat() (&render()->_stat)
#define render_mode() (&_tea_mode)
#define pixel_format(id) render_mode()->pixel_format[(id)]

#define not_supported(fn, res) fn { \
    cst_traceerror("not supported by this render"); \
    return res; \
} \

typedef struct te_RenderStat te_RenderStat;
typedef struct te_RenderMode te_RenderMode;

struct te_RenderStat {
   te_Color _draw_color;
   te_Color _clear_color;
   int _draw_mode;

   te_Transform _transform;
   te_Texture *_texture;
};

struct te_RenderMode {
    int tex_filtes[2];
    int tex_wrap[4];
    int pixel_format[TEA_PIXELFORMAT_COUNT];    
};

struct te_Timer {
    Uint32 prev_time, current_time;
    float delta;
    Uint32 prev_fps_time, frame, fps;
};

struct te_Input {
    struct { const Uint8 *state; Uint8 old_state[TEA_KEY_COUNT]; } keyboard;
    struct {
        Uint8 state[TEA_BUTTON_COUNT], old_state[TEA_BUTTON_COUNT];
        TEA_VALUE x, y;
        TEA_VALUE scroll_x, scroll_y;
    } mouse;
};

struct Tea {
    te_Render *_render;
    te_Window *_window;
    te_Event event;

    struct te_Input input;
    struct te_Timer timer;
};

Tea _tea_ctx;
te_RenderMode _tea_mode;
te_RenderStat _tea_stat;

TE_API te_Render* tea_render_create(te_Window *window, int flags);
TE_API int tea_render_destroy(te_Render *render);

TE_API int tea_render_begin(te_Render *render);
TE_API int tea_render_end(te_Render *render);

TE_API int tea_render_swap(te_Render *render);

#endif /* TEA_API_H */
