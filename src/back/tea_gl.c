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

#include "GL/gl3w.h"
#include "GL/glcorearb.h"
#include "SDL_video.h"
#include "tea.h"
#include "tea_api.h"
#include <SDL_opengl.h>

#include "linmath.h"

typedef unsigned int GLSL_Shader;

struct te_Texture {
    int usage;
    int id;
    unsigned int width, height;
    unsigned short filter[2];
    unsigned short wrap[2];
    int channels;
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

typedef union {
    float data[8];
    struct {
        te_Point pos;
        float color[4];
        te_Point texc;
    };
    struct {
        float x;
        float y;
        float r;
        float g;
        float b;
        float a;
        float s;
        float t;
    };
} te_Vertex;

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
    mat4x4 view;
};

struct Render {
    struct te_Batch batch;
    te_Shader base_shader;
    GLSL_Shader base_vert, base_frag;

    te_Texture base_texture;
    unsigned int width, height;
    unsigned int vao, vbo;
    mat4x4 world;
};

/* Window */
int tea_window_init(te_Window *window, const char *title, int width, int height, int flags) {
    window->flags = flags;

    int sdlw_flags = SDL_WINDOW_OPENGL;
    sdlw_flags |= SDL_WINDOW_SHOWN;
    if (flags & TEA_WINDOW_RESIZABLE) sdlw_flags |= SDL_WINDOW_RESIZABLE;
    if (flags & TEA_WINDOW_FULLSCREEN) sdlw_flags |= SDL_WINDOW_FULLSCREEN;

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    window->handle = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, sdlw_flags);

    window->width = width;
    window->height = height;

    return 1;
}

int tea_window_deinit(te_Window *window) {
    SDL_DestroyWindow(window->handle);
    return 1;
}

int tea_window_should_close(te_Window *window) {
    return event()->type == SDL_QUIT;
}

int tea_window_swap(te_Window *window) {
    SDL_GL_SwapWindow(window->handle);
    return 0;
}

/* Render */
int tea_init_render_mode(te_RenderMode *mode) {
    mode->pixel_format[TEA_PIXELFORMAT_UNKNOWN] = 0;
    mode->pixel_format[TEA_RGB] = GL_RGB;
    mode->pixel_format[TEA_RGBA] = GL_RGBA;
    return 1;
}

int tea_render_init(te_Render *r, te_Window *w, int flags) {
    struct Render *render = malloc(sizeof(*render));
    r->handle = render;

    SDL_GL_CreateContext(w->handle);
    if (gl3wInit()) cst_fatal("failed to init gl3w");
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

  render->base_vert = tea_compile_shader((const char*)vertexSource, GL_VERTEX_SHADER);
  render->base_frag = tea_compile_shader((const char*)fragmentSource, GL_FRAGMENT_SHADER);
    
    render->base_shader = tea_create_shader(render->base_vert, render->base_frag);

    r->stat.clear_color = BLACK;
    r->stat.draw_color = WHITE;
    r->stat.draw_mode = TEA_LINE;
    r->stat.tex = NULL;

    render->width = w->width;
    render->height = w->height;

    te_Byte *data = (te_Byte*)&WHITE;
    tea_init_texture(&render->base_texture, data, 1, 1, TEA_RGBA, TEA_TEXTURE_STATIC);

    r->stat.tex = &render->base_texture;
    mat4x4_identity(render->world);

    glGenVertexArrays(1, &render->vao);
    glGenBuffers(1, &render->vbo);

    glBindVertexArray(render->vao);
    glBindBuffer(GL_ARRAY_BUFFER, render->vbo);
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

int tea_render_deinit(te_Render *r) {
    struct Render *render = r->handle;
    tea_destroy_shader(&render->base_shader);
    glDeleteShader(render->base_frag);
    glDeleteShader(render->base_vert);

    return 1;
}

int tea_render_begin(te_Render *r) { 
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBlendEquation(GL_FUNC_ADD);

    struct Render *render = r->handle;
    glViewport(0, 0, render->width, render->height);

    tea_set_shader(render->base_shader);
    tea_shader_send_world();
    return 1; 
}
int tea_render_end(te_Render *r) { 
    tea_set_shader(0);
    return 1; 
}

int tea_set_render_target(te_Render *r, te_Texture *tex) {
    int fbo = 0;
    if (tex && tex->usage == TEA_TEXTURE_TARGET)
        fbo = (tex->id >> 8) & 0xff;

    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    return 1;
}

int tea_render_swap(te_Render *r) {
    SDL_GL_SwapWindow(window()->handle);
    return 1;
}

int tea_render_clear(te_Render *r) {
    te_Color *c = &r->stat.clear_color;
    glClearColor(c->r, c->g, c->b, c->a);
    glClear(GL_COLOR_BUFFER_BIT);
    return 1;
}
int tea_render_color(te_Render *r, te_Color col) {
    return 1;
}

int tea_render_mode(te_Render *r, int mode) {
    return 1;
}

int tea_point(TEA_TNUM x, TEA_TNUM y) {
#if 0
  SDL_RenderDrawPoint(render().render, p.x, p.y);
#endif
    return 1;
}

int tea_line(te_Point p0, te_Point p1) {
#if 0
  SDL_RenderDrawLine(render().render, p0.x, p0.y, p1.x, p1.y);
#endif

    te_Color *c = &render()->stat.draw_color;
    struct Render *render = render()->handle;

    mat4x4 model;
    mat4x4_identity(model);
    tea_shader_send(render->base_shader, "modelview", model, TEA_UNIFORM_MATRIX);

    render()->stat.tex = &render->base_texture;
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

    glBindVertexArray(render->vao);
    glBindBuffer(GL_ARRAY_BUFFER, render->vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(te_Vertex)*2, vert);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    int tid = render()->stat.tex->id & 0xff;
    glBindTexture(GL_TEXTURE_2D, tid);
    glBindVertexArray(render->vao);
    glDrawArrays(GL_LINES, 0, 2);
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    return 1;
}

// Draw Rects

int _draw_fill_rect(te_Rect *rect) {
    for (int yy = rect->y; yy < rect->h; yy++) {
        tea_line(TEA_POINT(rect->x, yy), TEA_POINT(rect->x+rect->w, yy));
    }
    return 1;
}
int _draw_line_rect(te_Rect *rect) {
  tea_line(TEA_POINT(rect->x, rect->y), TEA_POINT(rect->x+rect->w,rect->y));
  tea_line(TEA_POINT(rect->x+rect->w, rect->y), TEA_POINT(rect->x+rect->w,rect->y+rect->h));
  tea_line(TEA_POINT(rect->x+rect->w, rect->y+rect->h), TEA_POINT(rect->x,rect->y+rect->h));
  tea_line(TEA_POINT(rect->x, rect->y+rect->h), TEA_POINT(rect->x,rect->y));
  return 1;
}

typedef int(*RenderRectFn)(te_Rect*);
static RenderRectFn rect_fn[2] = {_draw_line_rect, _draw_fill_rect};

int tea_rect(te_Rect *r) {
  // tea()->draw.rect[render_stat()->_draw_mode](TEA_RECT(r.x+t->translate.x, r.y+t->translate.y, r.w*t->scale.x, r.h*t->scale.y));
  rect_fn[render()->stat.draw_mode](&TEA_RECT(r->x, r->y, r->w, r->h));
  return 1;
}

// Draw Circles

int _draw_fill_circle(te_Point p, TEA_TNUM radius) {
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
    return 1;
}

int _draw_line_circle(te_Point p, TEA_TNUM radius) {
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
    return 1;
}

typedef int(*RenderCircleFn)(te_Point,TEA_TNUM);
static RenderCircleFn circle_fn[2] = {_draw_line_circle, _draw_fill_circle};

int tea_circle(te_Point p, TEA_TNUM radius) {
  // tea()->draw.circle[render_stat()->_draw_mode](TEA_POINT(p.x+t->translate.x, p.y+t->translate.y), radius*t->scale.x);
    return circle_fn[render()->stat.draw_mode](TEA_POINT(p.x, p.y), radius);
    return 1;
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

int _draw_fill_triangle(te_Point p0, te_Point p1, te_Point p2) {
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

int _draw_line_triangle(te_Point p0, te_Point p1, te_Point p2) {
  // printf("qqqq\n");
  tea_line(p0, p1);
  tea_line(p1, p2);
  tea_line(p2, p0);
  return 1;
}

typedef int(*RenderTriangleFn)(te_Point,te_Point,te_Point);
static RenderTriangleFn triangle_fn[2] = {_draw_line_triangle, _draw_fill_triangle};

static te_Point calc_vec_scale(te_Point p0, te_Point p1, te_Point scale) {
  te_Point vec = TEA_POINT(p1.x-p0.x, p1.y-p0.y);
  vec.x *= scale.x;
  vec.y *= scale.y;
  
  return TEA_POINT(p0.x + vec.x, p0.y + vec.y);
}

int tea_draw_triangle(te_Point p0, te_Point p1, te_Point p2) {
  te_Transform *t = &render()->stat.transform;
  te_Point tpoints[3];
  
  tpoints[0] = calc_vec_scale(p0, p1, t->scale);
  tpoints[1] = calc_vec_scale(p1, p2, t->scale);
  tpoints[2] = calc_vec_scale(p2, p0, t->scale);
  
  for (int i = 0; i < 3; i++) tpoints[i] = TEA_POINT(tpoints[i].x+t->translate.x, tpoints[i].y+t->translate.y);
  
  // tea()->draw.triangle[render_stat()->_draw_mode](tpoints[0], tpoints[1], tpoints[2]);
  return triangle_fn[render()->stat.draw_mode](tpoints[0], tpoints[1], tpoints[2]);
}

// Draw textures

int tea_texture(te_Texture *tex, te_Rect *dest, te_Rect *src) {
    te_Point sz;
    sz.x = tex->width;
    sz.y = tex->height;
    te_Rect d;
    d.x = dest ? dest->x : 0;
    d.y = dest ? dest->y : 0;
    d.w = dest ? dest->w : sz.x;
    d.h = dest ? dest->h : sz.y;

    te_Rect s;
    s.x = src ? src->x : 0;
    s.y = src ? src->y : 0;
    s.w = src ? src->w : sz.x;
    s.h = src ? src->h : sz.y;

    te_Color *c = &render()->stat.draw_color;
    struct Render *render = render()->handle;

    mat4x4 model;
    mat4x4_identity(model);
    tea_shader_send(render->base_shader, "modelview", model, TEA_UNIFORM_MATRIX);

    render()->stat.tex = tex;
    te_Vertex vert[6];
    memset(&vert[1], 1, sizeof(te_Vertex));
    float width, height;
    width = tex->width;
    height = tex->height;

    float r,g,b,a;
    r = c->r / 255.f;
    g = c->g / 255.f;
    b = c->b / 255.f;
    a = c->a / 255.f;
    te_Rect norm;
    norm.x = s.x / width;
    norm.y = s.y / height;
    norm.w = s.w / width;
    norm.h = s.h / height;
    vert[0].x = d.x;
    vert[0].y = d.y;
    vert[0].r = r;
    vert[0].g = g;
    vert[0].b = b;
    vert[0].a = a;
    vert[0].s = norm.x;
    vert[0].t = norm.y;

    vert[1].x = d.x+d.w;
    vert[1].y = d.y;
    vert[1].r = r;
    vert[1].g = g;
    vert[1].b = b;
    vert[1].a = a;
    vert[1].s = norm.x + norm.w;
    vert[1].t = norm.y;

    vert[2].x = d.x+d.w;
    vert[2].y = d.y+d.h;
    vert[2].r = r;
    vert[2].g = g;
    vert[2].b = b;
    vert[2].a = a;
    vert[2].s = norm.x + norm.w;
    vert[2].t = norm.y + norm.h;

    vert[3].x = d.x+d.w;
    vert[3].y = d.y+d.h;
    vert[3].r = r;
    vert[3].g = g;
    vert[3].b = b;
    vert[3].a = a;
    vert[3].s = norm.x + norm.w;
    vert[3].t = norm.y + norm.h;

    vert[4].x = d.x;
    vert[4].y = d.y+d.h;
    vert[4].r = r;
    vert[4].g = g;
    vert[4].b = b;
    vert[4].a = a;
    vert[4].s = norm.x;
    vert[4].t = norm.y + norm.h;

    vert[5].x = d.x;
    vert[5].y = d.y;
    vert[5].r = r;
    vert[5].g = g;
    vert[5].b = b;
    vert[5].a = a;
    vert[5].s = norm.x;
    vert[5].t = norm.y;

    glBindVertexArray(render->vao);
    glBindBuffer(GL_ARRAY_BUFFER, render->vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(te_Vertex)*6, vert);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    int tid = render()->stat.tex->id & 0xff;
    glBindTexture(GL_TEXTURE_2D, tid);
    glBindVertexArray(render->vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
#if 0
  SDL_RenderCopy(render().render, tex->tex, &s, &d);
#endif
  return 1;
}

int tea_texture_ex(te_Texture *tex, te_Rect *dest, te_Rect *src, TEA_TNUM angle, te_Point origin, int flip) {
    te_Point size;
    size.x = tex->width;
    size.y = tex->height;
    te_Rect d, s;
  te_Transform *t = &render()->stat.transform;
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
  return 1;
}

int tea_print(te_Font *font, const char *text, TEA_TNUM x, TEA_TNUM y) {
  if (!text) return 0;
  char *p = (char*)text;

#if 0
  SDL_SetTextureColorMod(font->tex->tex, tea->_color.r, tea->_color.g, tea->_color.b);
  SDL_SetTextureAlphaMod(font->tex->tex, tea->_color.a);
#endif

  tea_texture(font->tex, NULL, NULL);
  return 1;
  while(*p) {
    te_Rect r;
    tea_font_char_rect(font, *p, &r);

    int index = (int)*p;
    tea_texture(font->tex, &TEA_RECT(x, y, r.w, r.h), &r);
    // tea_draw_texture(font->tex, &r, pos);
    x += font->c[index].ax;

    p++;
  }
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
  te_Point d0 = TEA_POINT(p0.x-p1.x, p0.y-p1.y);
  te_Point d1 = TEA_POINT(p2.x-p3.x, p2.y-p3.y);
  int cross = cross_prod(d1, d0);

  if (cross == 0) return 0;

  float t = (float)cross_prod(TEA_POINT(p0.x-p2.x, p0.y-p2.y), TEA_POINT(p2.x-p3.x, p2.y-p3.y)) / (float)cross;
  float u = (float)cross_prod(TEA_POINT(p0.x-p1.x, p0.y-p1.y), TEA_POINT(p0.x-p2.x, p0.y-p2.y)) / (float)cross;
  te_Point o;

  if (t <= 0 && t <= 1) o = TEA_POINT(p0.x+t*(p1.x-p0.x), p0.y+t*(p1.y-p0.y));
  else if (u <= 0 && u <= 1) o = TEA_POINT(p2.x+u*(p3.x-p2.x), p2.y+u*(p3.y-p2.y));
  else return 0;

  // printf("%f %f\n", out->x, out->y);
  if (out) *out = o;

  return 1;
}
#endif

void* tea_alloc_texture() {
    return malloc(sizeof(struct te_Texture));
}

int tea_init_texture(te_Texture *tex, void *data, int w, int h, int format, int access) {
    if (!tex) { cst_traceerror("te_Texture cannot be NULL"); return 0; }

    if (format < 0 || format >= TEA_PIXELFORMAT_COUNT) {
        cst_traceerror("invalid pixel format: %d", format);
        return 0;
    }

    int depth, pitch;
    Uint32 _format;
    tex->channels = 3;
    if (format == TEA_RGBA) tex->channels = 4;

    depth = tex->channels * 8;
    pitch = tex->channels * w;
    _format = pixel_format(format);

    glGenTextures(1, (GLuint*)&tex->id);
    glBindTexture(GL_TEXTURE_2D, tex->id);
    tex->filter[0] = GL_NEAREST;
    tex->filter[1] = GL_NEAREST;
    tex->wrap[0] = GL_CLAMP_TO_BORDER;
    tex->wrap[1] = GL_CLAMP_TO_BORDER;
    tex->usage = access;

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


/* Font */

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
        th = MAX(th, h);
    }

    font->tex = tea_create_texture(NULL, tw, th, TEA_RGBA, TEA_TEXTURE_STREAM);

    glBindTexture(GL_TEXTURE_2D, font->tex->id);
    int x = 0;
    for (i = 0; i < MAX_FONT_CHAR; i++) {
        int ww = font->c[i].bw;
        int hh = font->c[i].bh;
        int ssize = ww * hh * 4;
        int ox, oy;

        unsigned char *bitmap = stbtt_GetCodepointBitmap(&font->info, 0, font->scale, i, NULL, NULL, &ox, &oy);

        unsigned char pixels[ssize];
        for (int j = 0; j < ssize; j += 4) {
            int ii = j / 4;
            pixels[j] = 255;
            pixels[j+1] = 255;
            pixels[j+2] = 255;
            pixels[j+3] = bitmap[ii];
        }
        stbtt_FreeBitmap(bitmap, font->info.userdata);

        glTexSubImage2D(GL_TEXTURE_2D, 0, x, 0, ww, hh, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
        font->c[i].tx = (float)x / tw;
        x += font->c[i].bw;
    }
    glBindTexture(GL_TEXTURE_2D, 0);
    return 1;
}
te_Font* tea_create_font(const void *data, unsigned int buf_size, int font_size) {
    if (!data) cst_tracefatal("invalid te_Font data");
    if (buf_size <= 0) cst_tracefatal("buf_size <= 0");
    if (font_size <= 0) cst_tracefatal("font_size <= 0");
  
    te_Font *f = malloc(sizeof(*f));
    if (!f) cst_fatal("failed to alloc a block for te_Font");

    tea_font_init(f, data, buf_size, font_size);

    return f;
}

te_Font* tea_load_font(const char *filename, int font_size) {
  size_t sz;
  FILE *fp;
  fp = fopen(filename, "rb");
  if (!fp) cst_tracefatal("failed to load font: %s", filename);

  fseek(fp, 0, SEEK_END);
  sz = ftell(fp);
  fseek(fp, 0, SEEK_SET);
  unsigned char buffer[sz];

  (void)fread(buffer, 1, sz, fp);


  te_Font *font = tea_create_font(buffer, sz, font_size);
  fclose(fp);

  return font;
}


void tea_font_destroy(te_Font *font) {
  free(font);
}

int tea_font_get_rect(te_Font* font, const int c, TEA_TNUM *x, TEA_TNUM *y, te_Point *out_pos, te_Rect *r, TEA_TNUM width) {
  if (c == '\n') {
    *x = 0;
    int h;
    // SDL_QueryTexture(font->tex, NULL, NULL, NULL, &h);
    h = font->tex->height;
    *y += h;
    return 1;
  }
  return 1;
}

int tea_font_char_rect(te_Font *font, const unsigned int c, te_Rect *r) {
    if (!font) cst_trace("te_Font cannot be NULL");
  if (c == '\n' || c == '\t') return 1;
  if (c >= MAX_FONT_CHAR) return 0;

  if (r) *r = TEA_RECT(font->c[c].tx, 0, font->c[c].bw, font->c[c].bh);
  return 1;
}

int tea_font_get_text_width(te_Font *font, const char *text, int len);
int tea_font_get_text_height(te_Font *font, const char *text, int len);

/* Shader */

int tea_set_shader(te_Shader shader) {
    /*if (shader == 0) render()->current_shader = &render()->default_shader;
    else render()->current_shader = shader;

    glUseProgram(render()->current_shader->_shader);*/
    glUseProgram(shader);
    return 1;
}

te_Shader tea_create_shader(int vert, int frag) {
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


int tea_destroy_shader(te_Shader *shader) {
    if (!shader) return 0;
    glDeleteShader(*shader);
    return 1;
}

int tea_compile_shader(const char *source, int type) {
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

int tea_shader_send_world() {
    struct Render *render = render()->handle;
    mat4x4_ortho(render->world, 0.0, render->width, render->height, 0.0, 0.0, 1.0);
    tea_shader_send(render->base_shader, "world", render->world, TEA_UNIFORM_MATRIX);

    return 1;
}

int tea_shader_send(te_Shader shader, const char *name, void *value, int uniform_type) {
    return tea_shader_send_count(shader, name, 1, value, uniform_type);
    return 1;
}

int tea_shader_send_count(te_Shader shader, const char *name, int count, void *value, int uniform_type) {
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

    GLuint uniform = glGetUniformLocation(shader, (const GLchar*)name);
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
