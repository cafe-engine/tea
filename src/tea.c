#include "SDL_keyboard.h"
#include "tea.h"
#include "tea_api.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "stb_image.h"

Tea _tea_ctx = {0};

int tea_config_init(te_Config *conf, const char *title, int width, int height) {
    if (!conf) return 0;
    title = title ? title : ("tea "TEA_VERSION);

    if (title) strcpy((char*)conf->title, title);
    conf->width = width;
    conf->height = height;
    conf->flags = SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_TIMER | SDL_INIT_EVENTS;
    conf->render_flags = 0;
    conf->window_flags = 0;

  return 1;
}

int tea_init(te_Config *conf) {
    if (!conf) {
        tea_error("config == NULL");
        return 0;
    }
    memset(tea(), 0, sizeof(struct Tea));

    if (SDL_Init(conf->flags)) {
        tea_error("failed to init SDL");
        return 0;
    }
    tea_init_render_mode(&tea()->mode);
    tea_window_init(window(), (const char*)conf->title, conf->width, conf->height, conf->window_flags);
    tea_render_init(render(), window(), conf->render_flags);

    tea()->input.keyboard.state = SDL_GetKeyboardState(NULL);
    memset(tea()->input.keyboard.old_state, 0, TEA_KEY_COUNT);
    tea()->running = 1;

    return 1;
}

int tea_quit() {
    tea_window_deinit(window());
    tea_render_deinit(render());

    SDL_Quit();
    return 1;
}

int tea_should_close() {
    if (tea_window_should_close(window())) return 1;
    return !tea()->running;
}

int tea_begin() {
    tea_update_input();

    tea()->timer.current_time = SDL_GetTicks();
    tea()->timer.delta = tea()->timer.current_time - tea()->timer.prev_time;
    tea()->timer.prev_time = tea()->timer.current_time;

    tea()->timer.frame++;

    SDL_Delay(TEA_FPS);
    tea_render_begin(render());
    return 1;
}

int tea_end() {
    tea_render_end(render());
    tea_render_swap(render());
    return 1;
}

float tea_delta() {
    return tea()->timer.delta / 1000.f;
}

int tea_framerate() {
    return tea()->timer.fps;
}

int tea_clear_color(te_Color color) {
    render()->stat.clear_color = color;
    return 1;
}

int tea_clear() {
    return tea_render_clear(render());
}

int tea_get_draw_mode() {
  return render()->stat.draw_mode;
}

te_Color tea_get_draw_color() {
  return render()->stat.draw_color;
}

int tea_draw_color(te_Color col) {
    render()->stat.draw_color = col;
    return tea_render_color(render(), col);
}

int tea_draw_mode(int mode) {
    render()->stat.draw_mode = mode;
    return tea_render_mode(render(), mode);
}

int tea_set_transform(te_Transform *t) {
    if (t) render()->stat.transform = *t;
    else render()->stat.transform = (te_Transform){{0,0},0,{1,1},{0,0}};
    return 0;
}

int tea_get_transform(te_Transform *out) {
    if (!out) return 0;
    *out = render()->stat.transform;
    return 1;
}

int tea_translate(TEA_TNUM x, TEA_TNUM y) {
    render()->stat.transform.translate = TEA_POINT(x, y);
    return 1;
}

int tea_rotate(TEA_TNUM angle) {
    render()->stat.transform.angle = angle;
    return 1;
}

int tea_scale(TEA_TNUM x, TEA_TNUM y) {
    render()->stat.transform.scale = TEA_POINT(x, y);
    return 1;
}

int tea_origin(TEA_TNUM x, TEA_TNUM y) {
    render()->stat.transform.origin = TEA_POINT(x, y);
    return 1;
}

te_Texture* tea_create_texture(void *data, int w, int h, int format, int usage) {
    te_Texture *tex = tea_alloc_texture();
    if (!tex) { tea_error("failed to alloc texture"); return NULL; }
    if (w <= 0) { tea_error("invalid width: %d <= 0", w); return NULL; }
    if (h <= 0) { tea_error("invalid height: %d <= ", h); return NULL; }

    if (!tea_init_texture(tex, data, w, h, format, usage)) return NULL;

    return tex;
}

te_Texture *tea_load_texture(const char *filename, int usage) {
    te_Texture *tex = tea_alloc_texture();
    if (!tex) { tea_error("failed to alloc texture"); return NULL; }
    int req_format = TEA_RGBA;
    int w, h, format;

    unsigned char *data = stbi_load(filename, &w, &h, &format, req_format);
    if (!data) {
        tea_error("failed to load image: %s", filename);
        return NULL;
    }

    if (!tea_init_texture(tex, data, w, h, format, usage)) return NULL;

    return tex;
}

te_Texture *tea_load_texture_from_memory(void *data, unsigned int size, int usage) {
    te_Texture *tex = tea_alloc_texture();
    if (!tex) { tea_error("failed to alloc texture"); return NULL; }

    int req_format = TEA_RGBA;
    int w, h, format;

    unsigned char *dt = stbi_load_from_memory(data, size, &w, &h, &format, req_format);
    if (!dt) {
        tea_error("failed to load image");
        return NULL;
    }

    if (!tea_init_texture(tex, dt, w, h, format, usage)) return NULL;

    return tex;
}

int tea_set_target(te_Texture *tex) {
    return tea_set_render_target(render(), tex);
}

#if 0
#ifndef TEA_NO_LINEFN
int tea_line(te_Point p0, te_Point p1) {
    TEA_TNUM dx = p1.x - p0.x;
    TEA_TNUM dy = p1.y - p0.y;
    int steps = MAX(fabs(dx),fabs(dy));
    float xinc = dx / (float)steps;
    float yinc = dy / (float)steps;

    tea_point(p0.x, p0.y);
    float x = p0.x;
    float y= p0.y;
    for (int v = 0; v < steps; v++) {
        x += xinc;
        y += yinc;
        tea_point(x, y);
    }

    return 1;
}
#endif

#ifndef TEA_NO_RECTFN
int tea_rect(te_Rect *r) {
    if (!r) r = &TEA_RECT(0, 0, window()->width, window()->height);
    if (render()->stat.draw_mode == TEA_FILL) for (int y = r->y; y < r->y+r->h; y++) tea_line(TEA_POINT(r->x,y), TEA_POINT(r->x+r->w,y));
    else {
        tea_line(TEA_POINT(r->x,r->y), TEA_POINT(r->x+r->w,r->y));
        tea_line(TEA_POINT(r->x+r->w,r->y), TEA_POINT(r->x+r->w,r->y+r->h));
        tea_line(TEA_POINT(r->x+r->w,r->y+r->h), TEA_POINT(r->x,r->y+r->h));
        tea_line(TEA_POINT(r->x,r->y+r->h), TEA_POINT(r->x,r->y));
    }
    return 1;
}
#endif

#ifndef TEA_NO_CIRCLEFN
static int tea_circle_fill(te_Point p, TEA_TNUM radius) {
    int x = 0;
    int y = radius;

    int P = 1 - radius;

    if (radius > 0) tea_line(TEA_POINT(p.x + radius, p.y), TEA_POINT(p.x-radius, p.y));

    while (x <= y) {
        if (P < 0) P += 2*x + 3;
        else {
            P += (2*(x-y))+5;
            y--;
        }
        x++;

        if (x > y) break;

        tea_line(TEA_POINT(p.x-x, p.y+y), TEA_POINT(p.x+x, p.y+y));
        tea_line(TEA_POINT(p.x+x, p.y-y), TEA_POINT(p.x-x, p.y-y));
        if (x != y) {
            tea_line(TEA_POINT(p.x-y, p.y+x), TEA_POINT(p.x+y, p.y+x));
            tea_line(TEA_POINT(p.x+y, p.y-x), TEA_POINT(p.x-y, p.y-x));
        }
  }
}
static int tea_circle_line(te_Point p,TEA_TNUM radius) {
    int x = -radius;
    int y = 0;
    int r = radius;
    int err = 2 - 2*r;

    do {
        tea_point(p.x-x, p.y+y);
        tea_point(p.x-y, p.y-x);
        tea_point(p.x+x, p.y-y);
        tea_point(p.x+y, p.y+x);
        r = err;
        if (r <= y) err += ++y*2+1;
        if (r > x || err > y) err += ++x*2+1;
    } while (x < 0);
}

int tea_circle(te_Point p, TEA_TNUM radius) {
    if (render()->stat.draw_mode == TEA_FILL) return tea_circle_fill(p, radius);
    else return tea_circle_line(p, radius);
    return 1;
}
#endif

#ifndef TEA_NO_TRIANGFN
static int tea_triangle_line(te_Point p0, te_Point p1, te_Point p2) {
    tea_line(p0, p1);
    tea_line(p1, p2);
    tea_line(p2, p0);
    return 1;
}
static void fill_bottom_flat_triangle(te_Point p0, te_Point p1, te_Point p2) {
  int dy = (p2.y - p0.y);
  float invslope0 = (p1.x - p0.x) / dy;
  float invslope1 = (p2.x - p0.x) / dy;

  float curx1 = p0.x;
  float curx2 = p0.x;

  int scanline_y;
  for (scanline_y = p0.y; scanline_y <= p1.y; scanline_y++) {
    tea_line(TEA_POINT(curx1, scanline_y), TEA_POINT(curx2, scanline_y));
    curx1 += invslope0;
    curx2 += invslope1;
  }
}

static void fill_top_flat_triangle(te_Point p0, te_Point p1, te_Point p2) {
  int dy = (p2.y - p0.y);
  float invslope0 = (p2.x - p0.x) / dy;
  float invslope1 = (p2.x - p1.x) / dy;

  float curx1 = p2.x;
  float curx2 = p2.x;

  int scanline_y;
  for (scanline_y = p2.y; scanline_y > p1.y; scanline_y--) {
    tea_line(TEA_POINT(curx1, scanline_y), TEA_POINT(curx2, scanline_y));
    curx1 -= invslope0;
    curx2 -= invslope1;
  }
}

static void points_ord_y(te_Point *points, int len) {
  for (int i = 0; i < len; i++) {
    for (int j = 0; j < len-1; j++) {
      if (points[j].y < points[j+1].y) continue;
      te_Point aux = points[j];
      points[j] = points[j+1];
      points[j+1] = aux;
    }
  }
}

static int tea_triangle_fill(te_Point p0, te_Point p1, te_Point p2) {
    te_Point points[3];
    points[0] = p0;
    points[1] = p1;
    points[2] = p2;

    points_ord_y(points, 3);

    if (points[1].y == points[2].y) fill_bottom_flat_triangle(points[0], points[1], points[2]);
    else if (points[0].y == points[1].y) fill_bottom_flat_triangle(points[0], points[1], points[2]);
    else {
        te_Point p = TEA_POINT(
        (points[0].x + ((points[1].y - points[0].y) / (points[2].y - points[0].y)) * (points[2].x - points[0].x)),
        points[1].y
        );

        fill_bottom_flat_triangle(points[0], points[1], p);
        fill_top_flat_triangle(points[1], p, points[2]);
    }
    return 1;
}

int tea_triangle(te_Point p0, te_Point p1, te_Point p2) {
    if (render()->stat.draw_mode == TEA_FILL) return tea_triangle_fill(p0, p1, p2);
    else return tea_triangle_line(p0, p1, p2);

    return 1;
}
#endif
#endif

/*******************
 *      Input      *
 *******************/

int tea_update_input() {
  // memcpy(tea()->input.old_key, tea()->input.key_array, TEA_KEY_COUNT);
  memcpy(tea()->input.keyboard.old_state, tea()->input.keyboard.state, TEA_KEY_COUNT);
  int mx, my;
  Uint8 mouse_state = SDL_GetMouseState(&mx, &my);
  tea()->input.mouse.x = mx;
  tea()->input.mouse.y = my;
  for (int i = 0; i < TEA_BUTTON_COUNT; i++) {
    tea()->input.mouse.old_state[i] = tea()->input.mouse.state[i];
    tea()->input.mouse.state[i] = mouse_state & SDL_BUTTON(i+1);
  }
  SDL_PollEvent(event());

  return 1;
}

int tea_keyboard_is_down(int key) {
  return tea()->input.keyboard.state[key];
}

int tea_keyboard_is_up(int key) {
  return !tea_keyboard_is_down(key);
}

int tea_keyboard_was_pressed(int key) {
  return !tea()->input.keyboard.old_state[key] && tea_keyboard_is_down(key);
}
int tea_keyboard_was_released(int key) {
  return tea()->input.keyboard.old_state[key] && tea_keyboard_is_up(key);
}

/* Mouse */

TEA_TNUM tea_mouse_x() {
    return tea()->input.mouse.x;
}

TEA_TNUM tea_mouse_y() {
    return tea()->input.mouse.y;
}

int tea_mouse_pos(TEA_TNUM *x, TEA_TNUM *y) {
    if (x) *x = tea()->input.mouse.x;
    if (y) *y = tea()->input.mouse.y;

    return 1;
}

int tea_mouse_is_down(int button) {
  return tea()->input.mouse.state[button];
}
int tea_mouse_is_up(int button) {
  return !tea_mouse_is_down(button);
}

int tea_mouse_was_pressed(int button) {
  return !tea()->input.mouse.old_state[button] && tea_mouse_is_down(button);
}
int tea_mouse_was_released(int button) {
  return tea()->input.mouse.old_state[button] && tea_mouse_is_up(button);
}

int tea_joystick_axis(int jid, int axis) {
    return 1;
}
int tea_joystick_is_down(int jid, int button) {
    return 1;
}
int tea_joystick_is_up(int jid, int button) {
    return 1;
}
int tea_joystick_was_pressed(int jid, int button) {
    return 1;
}
int tea_joystick_was_released(int jid, int button) {
    return 1;
}

/* Debug */

#include <stdarg.h>

const char* tea_geterror() {
    return tea()->error_buf;
}

int tea_error(const char *fmt, ...) {
    va_list args;

    va_start(args, fmt);
    sprintf(tea()->error_buf, fmt, args);
    va_end(args);

    return 1;
}

#ifndef CAFE_ENGINE
    #define CSTAR_IMPLEMENTATION
    #include "cstar.h"
#endif
