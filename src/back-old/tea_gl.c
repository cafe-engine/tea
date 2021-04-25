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

#include "GL/glcorearb.h"
#include "tea.h"
#include "tea_api.h"
#include "SDL_error.h"
#include "SDL_render.h"
#include "SDL_surface.h"
#include "SDL_video.h"

#include "GL/gl3w.h"

#include <SDL.h>
#include <SDL_opengl.h>

#include "linmath.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "cstar.h"


typedef unsigned int Texture;
typedef unsigned int RenderTarget;
typedef unsigned int Shader;

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
    RenderTarget fbo;
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

  unsigned int atlas_vao;
  unsigned int atlas_vbo;
};

#if 0
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
    te_RenderMode _mode;
    te_RenderStat _stat;
#if 0
    struct te_Batch batch;
#endif

    te_Shader default_shader;
    unsigned int default_vert_shader;
    unsigned int default_frag_shader;
    te_Shader *current_shader;

    te_Texture default_texture;
    te_Texture *current_texture;

    mat4x4 world_view;

    unsigned int _vao;
    unsigned int _vbo;

    unsigned int width, height;
};


int tea_config_init(te_Config *conf, const char *title, int width, int height) {
  if (!conf) cst_fatal("te_Config cannot be NULL");
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
    render_mode()->pixel_format[TEA_RGB] = GL_RGB;
    render_mode()->pixel_format[TEA_RGBA] = GL_RGBA;

    return 1;
}

Tea* tea_init(te_Config *c) {
    if (c == NULL) cst_fatal("te_Config cannot be NULL");
    memset(tea(), 0, sizeof(*tea()));

    init_modes();
    c->window_flags |= SDL_WINDOW_OPENGL;
    if (SDL_Init(c->flags)) cst_fatal("failed to init SDL: %s", SDL_GetError());

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    window() = tea_window_create((const char*)c->title, c->width, c->height, c->window_flags);

    tea()->input.keyboard.state = SDL_GetKeyboardState(NULL);
    memset(tea()->input.keyboard.old_state, 0, TEA_KEY_COUNT);

    render() = tea_render_create(window(), c->render_flags);
    render_stat()->_transform.scale = tea_point(1, 1);
    render()->width = c->width;
    render()->height = c->height;

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

void tea_clear() {
    te_Color *c = &render_stat()->_clear_color;
    glClearColor(c->r, c->g, c->b, c->a);
    glClear(GL_COLOR_BUFFER_BIT);
#if 0
    SDL_SetRenderDrawColor(render()->_handle, c->r, c->g, c->b, c->a);
    SDL_RenderClear(render().render);
    c = &render_stat()->_draw_color;
    SDL_SetRenderDrawColor(render().render, c->r, c->g, c->b, c->a);
#endif
}

void tea_clear_color(te_Color color) {
  render_stat()->_clear_color = color;
}
void tea_draw_color(te_Color color) {
    render_stat()->_draw_color = color;
#if 0
    te_Color *c = &color;
    SDL_SetRenderDrawColor(render().render, c->r, c->g, c->b, c->a);
#endif
}
void tea_draw_mode(TEA_DRAW_MODE mode) {
    if (mode < TEA_LINE || mode > TEA_FILL) return;
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
#if 0
  SDL_RenderDrawPoint(render().render, p.x, p.y);
#endif
}

void tea_draw_line(te_Point p0, te_Point p1) {
#if 0
  SDL_RenderDrawLine(render().render, p0.x, p0.y, p1.x, p1.y);
#endif

    te_Color *c = &render_stat()->_draw_color;

    mat4x4 model;
    mat4x4_identity(model);
    tea_shader_send(render()->current_shader, "modelview", model, TEA_UNIFORM_MATRIX);

    te_Vertex vert[2];
    memset(&vert[1], 1, sizeof(te_Vertex));

    vert[0].x = p0.x;
    vert[0].y = p0.y;
    vert[0].r = c->r;
    vert[0].g = c->g;
    vert[0].b = c->b;
    vert[0].a = c->a;
    vert[0].s = vert[0].t = 0;

    vert[1].x = p1.x;
    vert[1].y = p1.y;
    vert[1].r = c->r;
    vert[1].g = c->g;
    vert[1].b = c->b;
    vert[1].a = c->a;
    vert[1].s = vert[1].t = 0.f;

    glBindVertexArray(render()->_vao);
    glBindBuffer(GL_ARRAY_BUFFER, render()->_vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(te_Vertex)*2, vert);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    glBindTexture(GL_TEXTURE_2D, render()->current_texture->tex);
    glBindVertexArray(render()->_vao);
    glDrawArrays(GL_LINES, 0, 2);
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

// Draw Rects

void _draw_fill_rect(te_Rect rect) {
#if 0
  SDL_RenderFillRect(render().render, &r);
#endif
  for (int yy = rect.y; yy < rect.h; yy++) {
    tea_draw_line(tea_point(rect.x, yy), tea_point(rect.x+rect.w, yy));
  }
}
void _draw_line_rect(te_Rect rect) {
  SDL_Rect r;
  r.x = rect.x;
  r.y = rect.y;
  r.w = rect.w;
  r.h = rect.h;
#if 0
  SDL_RenderDrawRect(render().render, &r);
#endif
  tea_draw_line(tea_point(rect.x, rect.y), tea_point(rect.x+rect.w,rect.y));
  tea_draw_line(tea_point(rect.x+rect.w, rect.y), tea_point(rect.x+rect.w,rect.y+rect.h));
  tea_draw_line(tea_point(rect.x+rect.w, rect.y+rect.h), tea_point(rect.x,rect.y+rect.h));
  tea_draw_line(tea_point(rect.x, rect.y+rect.h), tea_point(rect.x,rect.y));
}

static RenderRectFn rect_fn[2] = {_draw_line_rect, _draw_fill_rect};

void tea_draw_rect(te_Rect r) {
  te_Transform *t = &render_stat()->_transform;
  // tea()->draw.rect[render_stat()->_draw_mode](tea_rect(r.x+t->translate.x, r.y+t->translate.y, r.w*t->scale.x, r.h*t->scale.y));
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
        /*SDL_RenderDrawLine(render().render, p.x - x, p.y + y, p.x + x, p.y + y);
        SDL_RenderDrawLine(render().render, p.x + x, p.y - y, p.x - x, p.y - y);

    if (x != y) {
        SDL_RenderDrawLine(render().render, p.x - y, p.y + x, p.x + y, p.y + x);
        SDL_RenderDrawLine(render().render, p.x + y, p.y - x, p.x - y, p.y - x);
    }*/
  }
}

void _draw_line_circle(te_Point p, TEA_VALUE radius) {
    int x = -radius;
    int y = 0;
    int r = radius;
    int err = 2 - 2*r;

    do {
    /*SDL_RenderDrawPoint(render(), p.x - x, p.y + y);
    SDL_RenderDrawPoint(render(), p.x - y, p.y - x);
    SDL_RenderDrawPoint(render(), p.x + x, p.y - y);
    SDL_RenderDrawPoint(render(), p.x + y, p.y + x);*/
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
  // tea()->draw.circle[render_stat()->_draw_mode](tea_point(p.x+t->translate.x, p.y+t->translate.y), radius*t->scale.x);
  circle_fn[render_stat()->_draw_mode](tea_point(p.x+t->translate.x, p.y+t->translate.y), radius*t->scale.x);
}

// Draw Triangles

static void fill_bottom_flat_triangle(te_Point p0, te_Point p1, te_Point p2) {
  // float invslope0 = (p1.x - p0.x) / (p1.y - p0.y);
  // float invslope1 = (p2.x - p0.x) / (p2.y - p0.y);
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
  // printf("qqqq\n");
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
  
  // tea()->draw.triangle[render_stat()->_draw_mode](tpoints[0], tpoints[1], tpoints[2]);
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

#if 0
  SDL_RenderCopy(render().render, tex->tex, &s, &d);
#endif
}

void tea_draw_texture_ex(te_Texture *tex, te_Rect *dest, te_Rect *src, TEA_VALUE angle, te_Point origin, te_RenderFlip flip) {
  te_Point size;
  tea_texture_size(tex, &size);
    te_Rect d, s;
  te_Transform *t = &render_stat()->_transform;
  d.x = (dest ? (dest->x - t->origin.x + t->translate.x) : 0) - origin.x;
  d.y = (dest ? (dest->y - t->origin.x + t->translate.y) : 0) - origin.y;
  d.w = dest ? (dest->w*t->scale.x) : size.x - origin.x;
  d.h = dest ? (dest->h*t->scale.y) : size.y - origin.y;

  s.x = src ? src->x : 0;
  s.y = src ? src->y : 0;
  s.w = src ? src->w : size.x;
  s.h = src ? src->h : size.y;

#if 0
  SDL_Point sdl_origin = {origin.x, origin.y};
  SDL_RenderCopyEx(render().render, tex->tex, &s, &d, angle, &sdl_origin, (SDL_RendererFlip)flip);
#endif
}

void tea_draw_text(te_Font *font, const char *text, te_Point pos) {
  if (!text) return;
  char *p = (char*)text;

#if 0
  SDL_SetTextureColorMod(font->tex->tex, tea->_color.r, tea->_color.g, tea->_color.b);
  SDL_SetTextureAlphaMod(font->tex->tex, tea->_color.a);
#endif

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

int tea_render_init(te_Render *r, te_Window *window, int flags) {
    if (!r) cst_tracefatal("te_Render cannot be NULL");
    if (!window) cst_fatal("te_Window cannot be NULL");
    memset(r, 0, sizeof(*r));

    SDL_GL_CreateContext(window);
    if (gl3wInit()) cst_fatal("failed to init gl3w");

    tea_shader_load_default(&r->default_vert_shader, &r->default_frag_shader);
    tea_shader(&r->default_shader, r->default_vert_shader, r->default_frag_shader);
    
    r->_stat._clear_color = BLACK;
    r->_stat._draw_color = WHITE;
    r->_stat._draw_mode = TEA_LINE;
    r->_stat._transform = (te_Transform){{0, 0}, 0, {1, 1}};

    unsigned char *data = (unsigned char*)&WHITE;
    tea_texture_init_from_data(&r->default_texture, 1, 1, data, TEA_RGBA);

    r->_stat._texture = &r->default_texture;
    r->current_texture = &r->default_texture;

    mat4x4_identity(r->world_view);

    glGenVertexArrays(1, &r->_vao);
    glGenBuffers(1, &r->_vbo);

    glBindVertexArray(r->_vao);
    glBindBuffer(GL_ARRAY_BUFFER, r->_vbo);
    glBufferData(GL_ARRAY_BUFFER, 8*sizeof(te_Vertex), NULL, GL_DYNAMIC_DRAW); 
    
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(te_Vertex), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(te_Vertex), (void*)(sizeof(float)*2));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(te_Vertex), (void*)(sizeof(float)*6));
    glEnableVertexAttribArray(2);


    glBindVertexArray(0);
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

    glDeleteShader(render->default_vert_shader);
    glDeleteShader(render->default_frag_shader);
    tea_shader_destroy(&render->default_shader);
    return 1;
}

int tea_render_begin(te_Render *render) {
    glEnable(GL_BLEND);
    /*glEnable(GL_SCISSOR_TEST);*/
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBlendEquation(GL_FUNC_ADD);

    glViewport(0, 0, render()->width, render()->height);

    tea_set_shader(&render()->default_shader);
    tea_shader_send_world();
    return 1;
}

int tea_render_end(te_Render *render) {
    tea_set_shader(NULL);
    return 1;
}

int tea_render_swap(te_Render *render) {
    SDL_GL_SwapWindow(window());
    return 1;
}

#if 0
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

#if 0
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
#endif

int tea_texture_init(te_Texture *tex, int w, int h, unsigned int format, int access) {
    if (!tex) {
        cst_traceerror("te_Texture cannot be NULL");     
        return 0;
    }
    if (format < 0 || format >= TEA_PIXELFORMAT_COUNT) {
        cst_traceerror("invalida pixel format: %d", format);
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
    Uint32 _format;
    if (format == TEA_RGB) {
        depth = 24;
        pitch = 3*w;
        _format = pixel_format(format);
    } else {
        depth = 32;
        pitch = 4*w;
        _format = pixel_format(format);
    }

    glGenTextures(1, &tex->tex);
    glBindTexture(GL_TEXTURE_2D, tex->tex);
    tex->filter[0] = GL_NEAREST;
    tex->filter[1] = GL_NEAREST;
    tex->wrap[0] = GL_CLAMP_TO_BORDER;
    tex->wrap[1] = GL_CLAMP_TO_BORDER;

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, tex->filter[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, tex->filter[1]);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, tex->wrap[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, tex->wrap[1]);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, _format, w, h, GL_FALSE, _format, GL_UNSIGNED_BYTE, data);
  glBindTexture(GL_TEXTURE_2D, 0);
    
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
    unsigned int fbo = target ? target->fbo : 0;
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
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

    // printf("%c: %d %d %d %d\n", i, x0, y0, x1, y1);
    // printf("%d %d\n", w, h);

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

#if 0
  SDL_SetTextureBlendMode(font->tex->tex, SDL_BLENDMODE_BLEND);
#endif

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
    

#if 0
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
#endif
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

int tea_set_shader(te_Shader *shader) {
    if (!shader) render()->current_shader = &render()->default_shader;
    else render()->current_shader = shader;

    glUseProgram(render()->current_shader->_shader);
}

int tea_shader(te_Shader *shader, unsigned int vert, unsigned int frag) {
    if (!shader) {
        cst_error("te_Shader cannot be NULL");
        return -1;
    }
    shader->_shader = tea_shader_load_program(vert, frag);
    return 1;
}

int tea_shader_load_default(unsigned int *out_vert , unsigned int *out_frag) {
  const char *vertexSource = (const char*)"layout (location = 0) in vec2 in_Pos;\n"
                             "layout (location = 1) in vec4 in_Color;\n"
                             "layout (location = 2) in vec2 in_Texcoord;\n"
                             "varying vec4 v_Color;\n"
                             "varying vec2 v_Texcoord;\n"
                             "uniform mat4 world;\n"
                             "uniform mat4 modelview;\n"
                             "void main()\n"
                             "{\n"
                             "  gl_Position = world * modelview *  vec4(in_Pos.x,in_Pos.y , 0.0, 1.0);\n"
                             "  //gl_Position = vec4(in_Pos, 0.0, 1.0);\n"
                             "  v_Color = in_Color;\n"
                             "  v_Texcoord = in_Texcoord;\n"
                             "}\n";

  const char *fragmentSource = (const char*)"out vec4 FragColor;\n"
                               "varying vec4 v_Color;\n"
                               "varying vec2 v_Texcoord;\n"
                               "uniform sampler2D gm_BaseTexture;\n"
                               "void main()\n"
                               "{\n"
                               "  FragColor = v_Color * texture2D(gm_BaseTexture, v_Texcoord);\n"
                               "}\n";

  if (out_vert) *out_vert = tea_shader_compile((const char*)vertexSource, GL_VERTEX_SHADER);
  if (out_frag) *out_frag = tea_shader_compile((const char*)fragmentSource, GL_FRAGMENT_SHADER);

  return 1;
}

int tea_shader_destroy(te_Shader *shader) {
    if (!shader) return 0;

    glDeleteShader(shader->_shader);
    return 1;
}

int tea_shader_compile(const char *source, int type) {
    unsigned int shader = glCreateShader(type);

    const char *shader_tname = (const char*)(type == GL_VERTEX_SHADER ? "VERTEX" : "FRAGMENT");
    char shader_define[128];
    sprintf(shader_define, "#version 330\n#define %s_SHADER\n", shader_tname);

    GLchar const *files[] = {shader_define, source};
    GLint lengths[] = {strlen(shader_define), strlen(source)};

    glShaderSource(shader, 2, files, lengths);
    glCompileShader(shader);

    int success;
    char info_log[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, NULL, info_log);
        cst_error("%s::shader compilation failed: %s", shader_tname, info_log);
        return -1;
    }
    return shader;
}

int tea_shader_load_program(unsigned int vert, unsigned int frag) {
    int success;
    char info_log[512];

    unsigned int shader_program = glCreateProgram();
    glAttachShader(shader_program, vert);
    glAttachShader(shader_program, frag);
    glLinkProgram(shader_program);
    glGetProgramiv(shader_program, GL_LINK_STATUS, &success);

    if (!success) {
        glGetProgramInfoLog(shader_program, 512, NULL, info_log);
        cst_error("Shader linking failed: %s", info_log);
        return -1;
    }
    return shader_program;
}

int tea_shader_send_world() {
    mat4x4_ortho(render()->world_view, 0.0, (float)640, 380.f, 0.0, 0.0, 1.0);
    tea_shader_send(render()->current_shader, "world", render()->world_view, TEA_UNIFORM_MATRIX);

    return 1;
}

int tea_shader_send(te_Shader *shader, const char *name, void *value, int uniform_type) {
    return tea_shader_send_count(shader, name, 1, value, uniform_type);
}

int tea_shader_send_count(te_Shader *shader, const char *name, int count, void *value, int uniform_type) {
    if (!name) {
        cst_error("uniform name cannot be NULL");
        return -1;
    }
    if (!value) {
        cst_error("uniform value cannot be NULL");
        return -1;
    }
    if (count <= 0) {
        cst_error("uniform count <= 0");
        return -1;
    }

    GLuint uniform = glGetUniformLocation(shader->_shader, (const GLchar*)name);
    float *val = (float*)value;
    switch (uniform_type) {
        case TEA_UNIFORM_FLOAT:
            glUniform1fv(uniform, count, val); return 1;
        case TEA_UNIFORM_VEC2:
            glUniform2fv(uniform, count, val); return 1;
        case TEA_UNIFORM_VEC3:
            glUniform3fv(uniform, count, val); return 1;
        case TEA_UNIFORM_MATRIX:
            glUniformMatrix4fv(uniform, count, GL_FALSE, val); return 1;
        default:
            cst_error("invalid uniform type: %d", uniform_type);
    }

    return 0;
}
/* Debug */

void tea_error(const char *msg) {
  fprintf(stderr, "[tea] error: %s\n", msg);
  exit(1);
}
