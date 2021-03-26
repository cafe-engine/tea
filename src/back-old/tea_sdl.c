/**********************************************************************************
 *
 * MIT License
 *
 * Copyright (c) 2021 Canoi Gomes
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 **********************************************************************************/

#include "tea_api.h"
#include "SDL_error.h"
#include "SDL_render.h"
#include "SDL_surface.h"
#include "SDL_video.h"

#include <SDL.h>

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "cstar.h"

#define raw_format(id) (tea()->render.pixel_formats[(id)])
#define draw_color() tea()->render._draw_color
#define clear_color() tea()->render._clear_color
#define draw_mode() tea()->render._draw_mode

typedef SDL_Texture* Texture;
typedef SDL_Texture* RenderTarget;
typedef void* Shader;

struct te_Shader {
    Shader _shader;
};

struct te_Texture {
    Texture tex;
    unsigned int width, height;
    unsigned short filter[2];
    unsigned short wrap[2];
};

struct te_RenderTarget {
  union {
    te_Texture tex;
    struct {
      Texture _tex;
      unsigned int width, height;
      unsigned short filter[2];
      unsigned short wrap[2];
    };
  };
};

struct te_Font {
  te_Texture* tex;
  struct {
    int ax; // advance.x
    int ay; // advance.y

    int bw; // bitmap.width;
    int bh; // bitmap.rows;

    int bl; // bitmap_left;
    int bt; // bitmap_top;

    float tx; // x offset of glyph in texture coordinates
  } c[MAX_FONT_CHAR];


  unsigned char size;

  stbtt_fontinfo info;
  float ptsize;
  float scale;
  int baseline;
  void *data;

#if defined(TEA_GL_RENDER)
  unsigned int atlas_vao;
  unsigned int atlas_vbo;
#endif

};

#if defined(TEA_GL_RENDER)
struct te_Batch {
    unsigned int *indices;
    te_Vertex *vertices;
    te_Vertex *vertices_ptr;

    unsigned int total_index_count;
    unsigned int total_vertex_count;

    unsigned int max_quads;
    unsigned int max_vertices;
    unsigned int max_indices;

    unsigned int vao;
    unsigned int vbo[2];

    te_Rect clip;
    te_Texture *tex;
};
#endif

struct te_Render {
    te_RenderStat _stat;
    te_RenderMode _mode;
    SDL_Renderer *_handle;
};

static te_Config _conf;

int tea_config_init(te_Config *conf, const char *title, int width, int height) {
  if (!conf) conf = &_conf;
  title = title ? title : ("tea "TEA_VERSION);

  if (title) strcpy((char*)conf->title, title);
  conf->width = width;
  conf->height = height;
  conf->flags = SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_TIMER | SDL_INIT_EVENTS;
  conf->render_flags = SDL_RENDERER_ACCELERATED;
  conf->window_flags = SDL_WINDOW_SHOWN;

  return 1;
}

static int init_modes() {
    render_mode()->pixel_format[TEA_PIXELFORMAT_UNKNOWN] = 0;
    render_mode()->pixel_format[TEA_RGB] = SDL_PIXELFORMAT_RGB888;
    render_mode()->pixel_format[TEA_RGBA] = SDL_PIXELFORMAT_BGR888;

    render_mode()->pixel_format[TEA_RGBA] = SDL_PIXELFORMAT_RGBA32;
    render_mode()->pixel_format[TEA_ARGB] = SDL_PIXELFORMAT_ARGB32;
    render_mode()->pixel_format[TEA_BGRA] = SDL_PIXELFORMAT_BGRA32;
    render_mode()->pixel_format[TEA_ABGR] = SDL_PIXELFORMAT_ABGR32;
    return 1;
}

Tea* tea_init(te_Config *c) {
  // if (!c) c = &_conf;
    if (c == NULL) cst_fatal("te_Config cannot be NULL");
    memset(tea(), 0, sizeof(*tea()));

    init_modes();

  // memset(tea(), 0, sizeof(*tea()));
    if (SDL_Init(c->flags)) cst_fatal("failed to init SDL: %s", SDL_GetError());

    window() = tea_window_create((const char*)c->title, c->width, c->height, c->window_flags);


    tea()->input.keyboard.state = SDL_GetKeyboardState(NULL);
    memset(tea()->input.keyboard.old_state, 0, TEA_KEY_COUNT);

    render() = tea_render_create(window(), c->render_flags);
    render_stat()->_transform.scale = tea_point(1, 1);

    return tea();
}

Tea* tea_context() {
  return tea();
}

void tea_terminate() {
  tea_window_destroy(window());
  tea_render_destroy(render());

  SDL_Quit();
}

int tea_should_close() {
  return tea()->event.type == SDL_QUIT;
}

void tea_begin() {
  Tea *ctx = tea();
  // SDL_PollEvent(&ctx->event);
  tea_update_input();
  tea_clear_color(BLACK);
  tea_clear();
  //tea_render_clear((te_Color){0, 0, 0, 255});


  ctx->timer.current_time = SDL_GetTicks();
  ctx->timer.delta = ctx->timer.current_time - ctx->timer.prev_time;
  ctx->timer.prev_time = ctx->timer.current_time;
  
  // float delta = (ctx->timer.current_time - ctx->timer.prev_fps_time);
  ctx->timer.frame++;
  
  SDL_Delay(TEA_FPS);
  tea_render_begin(render());
}

void tea_end() {
    tea_render_end(render());
  tea_render_swap(render());
}

float tea_get_delta() {
  return tea()->timer.delta / 1000.f;
}

int tea_get_framerate() {
  return tea()->timer.fps;
}

/* Render */
int tea_render_init(te_Render *r, te_Window *window, int flags) {
    if (!r) cst_tracefatal("te_Render cannot be NULL");
    if (!window) cst_fatal("te_Window cannot be NULL");
    memset(r, 0, sizeof(*r));
    r->_handle = SDL_CreateRenderer(window, -1, flags);
    
    r->_stat._clear_color = BLACK;
    r->_stat._draw_color = WHITE;
    r->_stat._draw_mode = TEA_LINE;
    r->_stat._transform = (te_Transform){{0, 0}, 0, {1, 1}};

    r->_mode.pixel_format[TEA_PIXELFORMAT_UNKNOWN] = 0;
    r->_mode.pixel_format[TEA_RGB] = SDL_PIXELFORMAT_RGB888;
    r->_mode.pixel_format[TEA_RGBA] = SDL_PIXELFORMAT_BGR888;

    r->_mode.pixel_format[TEA_RGBA] = SDL_PIXELFORMAT_RGBA32;
    r->_mode.pixel_format[TEA_ARGB] = SDL_PIXELFORMAT_ARGB32;
    r->_mode.pixel_format[TEA_BGRA] = SDL_PIXELFORMAT_BGRA32;
    r->_mode.pixel_format[TEA_ABGR] = SDL_PIXELFORMAT_ABGR32;
    return 1;
}

te_Render* tea_render_create(te_Window *window, int flags) {
    if (!window) cst_fatal("te_Window cannot be NULL");
    te_Render *r = malloc(sizeof(*r));

    if (!tea_render_init(r, window, flags)) cst_fatal("failed to create render: %s", SDL_GetError());

  return r;
}

int tea_render_destroy(te_Render *render) {
    if (!render) return 0;

    SDL_DestroyRenderer(render->_handle);
    return 1;
}

int tea_render_begin(te_Render *render) {
    return 1;
}

int tea_render_end(te_Render *render) {
    return 1;
}

int tea_render_swap(te_Render *render) {
  SDL_RenderPresent(render->_handle);
  return 1;
}

void tea_clear() {
    te_Color *c = &render_stat()->_clear_color;
    SDL_SetRenderDrawColor(render()->_handle, c->r, c->g, c->b, c->a);
    SDL_RenderClear(render()->_handle);
    c = &render_stat()->_draw_color;
    SDL_SetRenderDrawColor(render()->_handle, c->r, c->g, c->b, c->a);
}

void tea_clear_color(te_Color color) {
  render_stat()->_clear_color = color;
}
void tea_draw_color(te_Color color) {
    render_stat()->_draw_color = color;
    te_Color *c = &color;
    SDL_SetRenderDrawColor(render()->_handle, c->r, c->g, c->b, c->a);
}
void tea_draw_mode(TEA_DRAW_MODE mode) {
    if (mode < TEA_LINE || mode > TEA_FILL) {
    }
    render_stat()->_draw_mode = mode;
}

te_Transform tea_get_transform() {
  return render_stat()->_transform;
}

void tea_set_transform(te_Transform *t) {
  if (!t) render_stat()->_transform = (te_Transform){{0, 0}, 0, {1, 1}};
  else render_stat()->_transform = *t;
}

void tea_set_scale(te_Point scale) {
  render_stat()->_transform.scale = scale;
}

void tea_set_translate(te_Point translate) {
  render_stat()->_transform.translate = translate;
}

void tea_set_angle(TEA_VALUE angle) {
  render_stat()->_transform.angle = angle;
}

void tea_set_origin(te_Point origin) {
  render_stat()->_transform.origin = origin;
}

void tea_draw_point(te_Point p) {
    SDL_RenderDrawPoint(render()->_handle, p.x, p.y);
}

void tea_draw_line(te_Point p0, te_Point p1) {
    SDL_RenderDrawLine(render()->_handle, p0.x, p0.y, p1.x, p1.y);
}

// Draw Rects

void _draw_fill_rect(te_Rect rect) {
  SDL_Rect r;
  r.x = rect.x;
  r.y = rect.y;
  r.w = rect.w;
  r.h = rect.h;
  SDL_RenderFillRect(render()->_handle, &r);
}
void _draw_line_rect(te_Rect rect) {
  SDL_Rect r;
  r.x = rect.x;
  r.y = rect.y;
  r.w = rect.w;
  r.h = rect.h;
  SDL_RenderDrawRect(render()->_handle, &r);
}

static RenderRectFn rect_fn[2] = {_draw_line_rect, _draw_fill_rect};

void tea_draw_rect(te_Rect r) {
  te_Transform *t = &render_stat()->_transform;
  rect_fn[render_stat()->_draw_mode](tea_rect(r.x+t->translate.x, r.y+t->translate.y, r.w*t->scale.x, r.h*t->scale.y));
}

void tea_draw_rect_(TEA_VALUE x, TEA_VALUE y, TEA_VALUE w, TEA_VALUE h) {
  tea_draw_rect(tea_rect(x, y, w, h));
}

void tea_draw_rect_point(te_Point p, te_Point s) {
  tea_draw_rect(tea_rect(p.x, p.y, s.x, s.y));
}

// Draw Circles

void _draw_fill_circle(te_Point p, TEA_VALUE radius) {
    int x = 0;
    int y = radius;

    int P = 1 - radius;

    if (radius > 0) tea_draw_line(tea_point(p.x + radius, p.y), tea_point(p.x-radius, p.y));

    while (x <= y) {
        if (P < 0) P += 2*x + 3;
        else {
            P += (2*(x-y))+5;
            y--;
        }
        x++;

        if (x > y) break;

        tea_draw_line(tea_point(p.x-x, p.y+y), tea_point(p.x+x, p.y+y));
        tea_draw_line(tea_point(p.x+x, p.y-y), tea_point(p.x-x, p.y-y));
        if (x != y) {
            tea_draw_line(tea_point(p.x-y, p.y+x), tea_point(p.x+y, p.y+x));
            tea_draw_line(tea_point(p.x+y, p.y-x), tea_point(p.x-y, p.y-x));
        }
  }
}

void _draw_line_circle(te_Point p, TEA_VALUE radius) {
    int x = -radius;
    int y = 0;
    int r = radius;
    int err = 2 - 2*r;

    do {
        tea_draw_point(tea_point(p.x-x, p.y+y));
        tea_draw_point(tea_point(p.x-y, p.y-x));
        tea_draw_point(tea_point(p.x+x, p.y-y));
        tea_draw_point(tea_point(p.x+y, p.y+x));
        r = err;
        if (r <= y) err += ++y*2+1;
        if (r > x || err > y) err += ++x*2+1;
    } while (x < 0);
}

static RenderCircleFn circle_fn[2] = {_draw_line_circle, _draw_fill_circle};

void tea_draw_circle(te_Point p, TEA_VALUE radius) {
  te_Transform *t = &render_stat()->_transform;
  circle_fn[render_stat()->_draw_mode](tea_point(p.x+t->translate.x, p.y+t->translate.y), radius*t->scale.x);
}

// Draw Triangles

static void fill_bottom_flat_triangle(te_Point p0, te_Point p1, te_Point p2) {
  int dy = (p2.y - p0.y);
  float invslope0 = (p1.x - p0.x) / dy;
  float invslope1 = (p2.x - p0.x) / dy;

  float curx1 = p0.x;
  float curx2 = p0.x;

  int scanline_y;
  for (scanline_y = p0.y; scanline_y <= p1.y; scanline_y++) {
    tea_draw_line(tea_point(curx1, scanline_y), tea_point(curx2, scanline_y));
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
    tea_draw_line(tea_point(curx1, scanline_y), tea_point(curx2, scanline_y));
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

void _draw_fill_triangle(te_Point p0, te_Point p1, te_Point p2) {
  te_Point points[3];
  points[0] = p0;
  points[1] = p1;
  points[2] = p2;

  points_ord_y(points, 3);

  if (points[1].y == points[2].y) fill_bottom_flat_triangle(points[0], points[1], points[2]);
  else if (points[0].y == points[1].y) fill_bottom_flat_triangle(points[0], points[1], points[2]);
  else {
    te_Point p = tea_point(
      (points[0].x + ((points[1].y - points[0].y) / (points[2].y - points[0].y)) * (points[2].x - points[0].x)),
      points[1].y
      );

    fill_bottom_flat_triangle(points[0], points[1], p);
    fill_top_flat_triangle(points[1], p, points[2]);
  }
}

void _draw_line_triangle(te_Point p0, te_Point p1, te_Point p2) {
  tea_draw_line(p0, p1);
  tea_draw_line(p1, p2);
  tea_draw_line(p2, p0);
}

static RenderTriangleFn triangle_fn[2] = {_draw_line_triangle, _draw_fill_triangle};

static te_Point calc_vec_scale(te_Point p0, te_Point p1, te_Point scale) {
  te_Point vec = tea_point(p1.x-p0.x, p1.y-p0.y);
  vec.x *= scale.x;
  vec.y *= scale.y;
  
  return tea_point(p0.x + vec.x, p0.y + vec.y);
}

void tea_draw_triangle(te_Point p0, te_Point p1, te_Point p2) {
  te_Transform *t = &render_stat()->_transform;
  te_Point tpoints[3];
  
  tpoints[0] = calc_vec_scale(p0, p1, t->scale);
  tpoints[1] = calc_vec_scale(p1, p2, t->scale);
  tpoints[2] = calc_vec_scale(p2, p0, t->scale);
  
  for (int i = 0; i < 3; i++) tpoints[i] = tea_point(tpoints[i].x+t->translate.x, tpoints[i].y+t->translate.y);
  
  triangle_fn[render_stat()->_draw_mode](tpoints[0], tpoints[1], tpoints[2]);
}

// Draw textures

void tea_draw_texture(te_Texture *tex, te_Rect *dest, te_Rect *src) {
  te_Point sz;
  tea_texture_size(tex, &sz);
  SDL_Rect d;
  te_Transform *t = &render_stat()->_transform;
  d.x = dest ? (dest->x - t->origin.x + t->translate.x) : 0;
  d.y = dest ? (dest->y - t->origin.y + t->translate.y) : 0;
  d.w = dest ? dest->w*t->scale.x : sz.x;
  d.h = dest ? dest->h*t->scale.y : sz.y;

  SDL_Rect s;
  s.x = src ? src->x : 0;
  s.y = src ? src->y : 0;
  s.w = src ? src->w : sz.x;
  s.h = src ? src->h : sz.y;

  SDL_RenderCopy(render()->_handle, tex->tex, &s, &d);
}

void tea_draw_texture_ex(te_Texture *tex, te_Rect *dest, te_Rect *src, TEA_VALUE angle, te_Point origin, te_RenderFlip flip) {
  te_Point size;
  tea_texture_size(tex, &size);
  SDL_Rect d;
  te_Transform *t = &render_stat()->_transform;
  d.x = (dest ? (dest->x - t->origin.x + t->translate.x) : 0) - origin.x;
  d.y = (dest ? (dest->y - t->origin.x + t->translate.y) : 0) - origin.y;
  d.w = dest ? (dest->w*t->scale.x) : size.x - origin.x;
  d.h = dest ? (dest->h*t->scale.y) : size.y - origin.y;

  SDL_Rect s;
  s.x = src ? src->x : 0;
  s.y = src ? src->y : 0;
  s.w = src ? src->w : size.x;
  s.h = src ? src->h : size.y;

  SDL_Point sdl_origin = {origin.x, origin.y};

  SDL_RenderCopyEx(render()->_handle, tex->tex, &s, &d, angle, &sdl_origin, (SDL_RendererFlip)flip);
}

void tea_draw_text(te_Font *font, const char *text, te_Point pos) {
  if (!text) return;
  char *p = (char*)text;

  te_Color *dc = &render_stat()->_draw_color;

  SDL_SetTextureColorMod(font->tex->tex, dc->r, dc->g, dc->b);
  SDL_SetTextureAlphaMod(font->tex->tex, dc->a);

  while(*p) {
    te_Rect r;
    tea_font_char_rect(font, *p, &r);

    int index = (int)*p;
    tea_draw_texture(font->tex, &tea_rect(pos.x, pos.y, r.w, r.h), &r);
    // tea_draw_texture(font->tex, &r, pos);
    pos.x += font->c[index].ax;

    p++;
  }
}

te_Window* tea_window_create(const char *title, int width, int height, int flags) {  
  te_Window *w = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, flags);

  return w;
}

void tea_window_destroy(te_Window *window) {
  if (!window) return;

  SDL_DestroyWindow(window);
}



#if defined(TEA_GL_RENDER)

struct te_Batch tea_batch_create(te_Texture *tex, int max_quads) {
    struct te_Batch batch = {0};
    if (max_quads <= 0) max_quads = MAX_QUADS;
    batch.max_quads = max_quads;
    batch.max_indices = max_quads * 4;
    batch.max_vertices = max_quads * 6;

    batch.total_index_count = 0;
    batch.total_vertex_count = 0;

    batch.vertices = malloc(batch.max_vertices * sizeof(te_Vertex));
    batch.indices = malloc(batch.max_indices * sizeof(int));

    batch.tex = tex;
    int offset = 0;

    for (int i = 0; i < batch.max_indices; i+= 6) {
        batch.indices[i] = offset;
        batch.indices[i+2] = offset + 1;
        batch.indices[i+3] = offset + 2;

        batch.indices[i+3] = offset;
        batch.indices[i+4] = offset + 2;
        batch.indices[i+5] = offset + 3;
        offset += 4;
    }



    return batch;
}

#endif

static int cross_prod(te_Point v0, te_Point v1) {
  return v0.x*v1.y - v0.y*v1.x;
}

static int get_intersection(te_Point p0, te_Point p1, te_Point p2, te_Point p3, te_Point *out) {
  if (!out) return -1;
  te_Point d0 = tea_point(p0.x-p1.x, p0.y-p1.y);
  te_Point d1 = tea_point(p2.x-p3.x, p2.y-p3.y);
  int cross = cross_prod(d1, d0);

  if (cross == 0) return 0;

  float t = (float)cross_prod(tea_point(p0.x-p2.x, p0.y-p2.y), tea_point(p2.x-p3.x, p2.y-p3.y)) / (float)cross;
  float u = (float)cross_prod(tea_point(p0.x-p1.x, p0.y-p1.y), tea_point(p0.x-p2.x, p0.y-p2.y)) / (float)cross;
  te_Point o;

  if (t <= 0 && t <= 1) o = tea_point(p0.x+t*(p1.x-p0.x), p0.y+t*(p1.y-p0.y));
  else if (u <= 0 && u <= 1) o = tea_point(p2.x+u*(p3.x-p2.x), p2.y+u*(p3.y-p2.y));
  else return 0;

  // printf("%f %f\n", out->x, out->y);
  if (out) *out = o;

  return 1;
}

int tea_texture_init(te_Texture *tex, int w, int h, unsigned int format, int access) {
    if (!tex) {
        cst_traceerror("te_Texture cannot be NULL");     
        return 0;
    }
    if (format < 0 || format >= TEA_PIXELFORMAT_COUNT) {
        cst_traceerror("invalida pixel format: %d", format);
    }
  
    tex->tex = SDL_CreateTexture(render()->_handle, pixel_format(format), access, w, h);

    if (!tex->tex) {
        cst_error("[SDL] failed to create texture: %s", SDL_GetError());
        return 0;
    }
    tex->width = w;
    tex->height = h;
    return 1;
}

int tea_texture_init_from_data(te_Texture *tex, int w, int h, void *data, int format) {
    if (!tex) { cst_traceerror("te_Texture cannot be NULL"); return 0; }

    if (format < 0 || format >= TEA_PIXELFORMAT_COUNT) {
        cst_traceerror("invalid pixel format: %d", format);
        return 0;
    }

    int depth, pitch;
    Uint32 pixel_format;
    if (format == TEA_RGB) {
        depth = 24;
        pitch = 3*w;
        pixel_format = pixel_format(format);
    } else {
        depth = 32;
        pitch = 4*w;
        pixel_format = pixel_format(format);
    }

    SDL_Surface *surf = SDL_CreateRGBSurfaceWithFormatFrom(data, w, h, depth, pitch, pixel_format);
    if (!surf) {
        cst_error("failed to create SDL_Surface: %s", SDL_GetError());
        return 0;
    }
    tex->tex = SDL_CreateTextureFromSurface(render()->_handle, surf);
    SDL_FreeSurface(surf);
    if (!tex->tex) {
        cst_error("failed to create SDL_Texture: %s", SDL_GetError());
        return 0;
    }

    tex->width = w;
    tex->height = h;

    return 1;
}

int tea_texture_init_from_file(te_Texture *tex, const char *filename) {
    if (!tex) { cst_traceerror("te_Texture cannot be NULL"); return 0; }
    if (!filename) { cst_traceerror("filename cannot be NULL"); return 0; }

    int req_format = STBI_rgb_alpha;
    int w, h, format;

    unsigned char *data = stbi_load(filename, &w, &h, &format, req_format);
    if (!tex) { cst_error("failed to load image: %s", filename); return 0; }

    int tformat = format == STBI_rgb ? TEA_RGB : TEA_RGBA;
    return tea_texture_init_from_data(tex, w, h, data, tformat);
}

te_Texture* tea_texture(int w, int h, unsigned int format, int access) {
    if (w <= 0) cst_fatal("texture width <= 0");
    if (h <= 0) cst_fatal("texture height <= 0");
  /*int i = _create_texture(w, h, format, access);
  return &tea()->textures[i];*/
    te_Texture *tex = (te_Texture*)malloc(sizeof(*tex));
    tea_texture_init(tex, w, h, format, access);

    return tex;
}

te_Texture* tea_texture_load(const char *str) {
    te_Texture *tex = (te_Texture*)malloc(sizeof(*tex));
    if (!tea_texture_init_from_file(tex, str)) {
        free(tex);
        tex = NULL;
    }

    return tex;
}

te_Texture* tea_texture_memory(unsigned char *pixels, int w, int h, unsigned int format) {
  return NULL;
}


int tea_texture_width(te_Texture *tex) {
  if (!tex) cst_fatal("invalid texture");

  // SDL_QueryTexture(tex, NULL, NULL, &w, NULL);
  return tex->width;
}
int tea_texture_height(te_Texture *tex) {
  if (!tex) cst_fatal("invalid texture");

  return tex->height;
}
void tea_texture_size(te_Texture *tex, te_Point *size) {
  if (!tex) cst_fatal("invalid texture");
  if (size) *size = tea_point(tex->width, tex->height);
}

// Render Target

te_RenderTarget* tea_render_target(int w, int h, int format) {
  te_RenderTarget *target = malloc(sizeof(*target));
  tea_texture_init(&target->tex, w, h, format, SDL_TEXTUREACCESS_TARGET);
  return target;
}

void tea_set_render_target(te_RenderTarget *target) {
    SDL_Texture *tex = target ? target->tex.tex : NULL;
    SDL_SetRenderTarget(render()->_handle, tex);
}

/* Font */

te_Font* tea_font(const void *data, size_t buf_size, int font_size) {
    if (!data) cst_tracefatal("invalid te_Font data");
    if (buf_size <= 0) cst_tracefatal("buf_size <= 0");
    if (font_size <= 0) cst_tracefatal("font_size <= 0");
  
    te_Font *f = malloc(sizeof(*f));
    if (!f) cst_fatal("failed to alloc a block for te_Font");

    tea_font_init(f, data, buf_size, font_size);

    return f;
}

te_Font* tea_font_load(const char *filename, int font_size) {
  size_t sz;
  FILE *fp;
  fp = fopen(filename, "rb");
  if (!fp) cst_tracefatal("failed to load font: %s", filename);

  fseek(fp, 0, SEEK_END);
  sz = ftell(fp);
  fseek(fp, 0, SEEK_SET);
  unsigned char buffer[sz];

  (void)fread(buffer, 1, sz, fp);


  te_Font *font = tea_font(buffer, sz, font_size);
  fclose(fp);

  return font;
}

int tea_font_init(te_Font *font, const void *data, size_t buf_size, int font_size) {
    font->data = (void*)data;
    if (!stbtt_InitFont(&font->info, data, 0)) tea_error("Failed to init font");

    int ascent, descent, line_gap;
    font->size = font_size;
    float fsize = font_size;

    font->scale = stbtt_ScaleForMappingEmToPixels(&font->info, fsize);
    stbtt_GetFontVMetrics(&font->info, &ascent, &descent, &line_gap);
    font->baseline = ascent * font->scale;

    int tw, th;
    tw = th = 0;

    int i;
    for (i = 0; i < MAX_FONT_CHAR; i++) {
        int ax, bl;
        int x0, y0, x1, y1;
        int w, h;

        stbtt_GetCodepointHMetrics(&font->info, i, &ax, &bl);
        stbtt_GetCodepointBitmapBox(&font->info, i, font->scale, font->scale, &x0, &y0, &x1, &y1);
        w = x1 - x0;
        h = y1 - y0;


        font->c[i].ax = ax * font->scale;
        font->c[i].ay = 0;
        font->c[i].bl = bl * font->scale;
        font->c[i].bw = w;
        font->c[i].bh = h;
        font->c[i].bt = font->baseline + y0;

        tw += w;
        th = tea_max(th, h);
    }

    font->tex = tea_texture(tw, th, TEA_RGBA, SDL_TEXTUREACCESS_STREAMING);

  SDL_SetTextureBlendMode(font->tex->tex, SDL_BLENDMODE_BLEND);

    int x = 0;
    for (i = 0; i < MAX_FONT_CHAR; i++) {
        int ww = font->c[i].bw;
        int hh = font->c[i].bh;
        int ssize = ww * hh;
        int ox, oy;

        unsigned char *bitmap = stbtt_GetCodepointBitmap(&font->info, 0, font->scale, i, NULL, NULL, &ox, &oy);
        Uint32 *pixels = NULL;
        int pitch;

        SDL_Rect r;
        r.x = x;
        r.y = 0;
        r.w = ww;
        r.h = hh;
    

        if (SDL_LockTexture(font->tex->tex, &r, (void**)&pixels, &pitch) != 0) cst_fatal("failed to lock texture: %s", SDL_GetError());
    // printf("%d\n", pitch);
        int yy = 0;
        for (int j = 0; j < ssize; j++) {
            int xx = j % ww;
            if (j != 0 && xx == 0) yy++;
            int index = xx + (yy * (pitch / 4));
      // printf("%d\n", index);
      // printf("teste %p, %d %d\n", pixels, pixels[index], bitmap[j]);
      // Uint32 pp = SDL_MapRGBA(&pixel_format, 255, 255, 255, bitmap[j]);
            Uint32 pp = bitmap[j];
            pp <<= 24;
            pp |= 0xffffff;
            pixels[index] = pp;
      // pixels[index] = SDL_MapRGBA(&format, 255, 255, 255, bitmap[j]);
        }
        SDL_UnlockTexture(font->tex->tex);
    // printf("%c: %d\n", i, ww);
    // if (SDL_UpdateTexture(font->tex, &r, pixels, tw*sizeof(*pixels)) != 0) {
    //   fprintf(stderr, "[tea] error: %s\n", SDL_GetError());
    //   exit(1);
    // }
        font->c[i].tx = x;

        x += font->c[i].bw;
    }
    return 1;
}

void tea_font_destroy(te_Font *font) {
  free(font);
}

void tea_font_get_rect(te_Font* font, const int c, TEA_VALUE *x, TEA_VALUE *y, te_Point *out_pos, te_Rect *r, TEA_VALUE width) {
  if (c == '\n') {
    *x = 0;
    int h;
    // SDL_QueryTexture(font->tex, NULL, NULL, NULL, &h);
    h = tea_texture_height(font->tex);
    *y += h;
    return;
  }


}

void tea_font_char_rect(te_Font *font, const unsigned int c, te_Rect *r) {
    if (!font) cst_trace("te_Font cannot be NULL");
  if (c == '\n' || c == '\t') return;
  if (c >= MAX_FONT_CHAR) return;

  if (r) *r = tea_rect(font->c[c].tx, 0, font->c[c].bw, font->c[c].bh);
}

int tea_font_get_text_width(te_Font *font, const char *text, int len);
int tea_font_get_text_height(te_Font *font, const char *text, int len);

/* Shader */

int not_supported(tea_shader(te_Shader *shader, unsigned int vert, unsigned int frag), -1);
int not_supported(tea_shader_load_default(unsigned int *out_v, unsigned int *out_f), -1);
int not_supported(tea_shader_destroy(te_Shader *shader), -1);
int not_supported(tea_sader_compile(const char *source, int type), -1);

int not_supported(tea_shader_load_program(unsigned int vert, unsigned int frag), -1);
int not_supported(tea_shader_send(te_Shader *shader, const char *name, void *value, int uniform_type), -1);

int not_supported(tea_shader_send_count(te_Shader *shader, const char *name, int count, void *value, int uniform_type), -1);
/* Debug */

void tea_error(const char *msg) {
  fprintf(stderr, "[tea] error: %s\n", msg);
  exit(1);
}
