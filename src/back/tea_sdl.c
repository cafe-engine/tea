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

#include "tea.h"
#include "tea_api.h"
#include "SDL_error.h"
#include "SDL_render.h"
#include "SDL_surface.h"
#include "SDL_video.h"

#include "cstar.h"

typedef SDL_Texture* Texture;
typedef SDL_Texture* RenderTarget;
typedef void* Shader;

struct te_Shader {
    Shader _shader;
};

struct te_Texture {
    int usage;
    void *handle;
    unsigned int width, height;
    unsigned int wrap[2];
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
};

/* Window */
int tea_window_init(te_Window *window, const char *title, int width, int height, int flags) {
    window->flags = flags;

    int sdlw_flags = SDL_WINDOW_SHOWN;
    if (flags & TEA_WINDOW_RESIZABLE) sdlw_flags |= SDL_WINDOW_RESIZABLE;
    if (flags & TEA_WINDOW_FULLSCREEN) sdlw_flags |= SDL_WINDOW_FULLSCREEN;

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

/* Render */
int tea_init_render_mode(te_RenderMode *mode) {
    
    tea()->mode.pixel_format[TEA_PIXELFORMAT_UNKNOWN] = 0;
    tea()->mode.pixel_format[TEA_RGB] = SDL_PIXELFORMAT_RGB888;
    tea()->mode.pixel_format[TEA_RGBA] = SDL_PIXELFORMAT_BGR888;

    tea()->mode.pixel_format[TEA_RGBA] = SDL_PIXELFORMAT_RGBA32;
    tea()->mode.pixel_format[TEA_ARGB] = SDL_PIXELFORMAT_ARGB32;
    tea()->mode.pixel_format[TEA_BGRA] = SDL_PIXELFORMAT_BGRA32;
    tea()->mode.pixel_format[TEA_ABGR] = SDL_PIXELFORMAT_ABGR32;

    return 1;
}

int tea_render_init(te_Render *r, te_Window *window, int flags) {
    if (!r) cst_tracefatal("te_Render cannot be NULL");
    if (!window) cst_fatal("te_Window cannot be NULL");
    memset(r, 0, sizeof(*r));
    int sdlflags = SDL_RENDERER_ACCELERATED;
    r->handle = SDL_CreateRenderer(window->handle, -1, sdlflags);
    
    r->stat.clear_color = BLACK;
    r->stat.draw_color = WHITE;
    r->stat.draw_mode = TEA_LINE;
    r->stat.tex = NULL;
    return 1;
}

int tea_render_deinit(te_Render *render) {
    if (!render) return 0;

    SDL_DestroyRenderer(render->handle);
    return 1;
}

int tea_render_begin(te_Render *render) { return 1; }

int tea_render_end(te_Render *render) { return 1; }

int tea_set_render_target(te_Render *r, te_Texture *target) {
    SDL_Texture *tex = NULL;
    if (target->usage != TEA_TEXTURE_TARGET) {
        tea_error("wrong texture usage type: %d", target->usage);
        return 0;
    }
    if (target) tex = target->handle;

    SDL_SetRenderTarget(render()->handle, tex);

    return 1;
}

int tea_render_swap(te_Render *render) {
  SDL_RenderPresent(render->handle);
  return 1;
}

int tea_render_clear(te_Render *r) {
    te_Color *c = &r->stat.clear_color;
    SDL_SetRenderDrawColor(r->handle, c->r, c->g, c->b, c->a);
    SDL_RenderClear(r->handle);
    c = &r->stat.draw_color;
    SDL_SetRenderDrawColor(r->handle, c->r, c->g, c->b, c->a);
    return 1;
}

int tea_render_color(te_Render *r, te_Color color) {
    te_Color *c = &color;
    SDL_SetRenderDrawColor(r->handle, c->r, c->g, c->b, c->a);
    return 1;
}

int tea_render_mode(te_Render *r, int mode) {
    tea()->mode.pixel_format[TEA_PIXELFORMAT_UNKNOWN] = 0;
    tea()->mode.pixel_format[TEA_RGB] = SDL_PIXELFORMAT_RGB888;
    tea()->mode.pixel_format[TEA_RGBA] = SDL_PIXELFORMAT_BGR888;

    tea()->mode.pixel_format[TEA_RGBA] = SDL_PIXELFORMAT_RGBA32;
    tea()->mode.pixel_format[TEA_ARGB] = SDL_PIXELFORMAT_ARGB32;
    tea()->mode.pixel_format[TEA_BGRA] = SDL_PIXELFORMAT_BGRA32;
    tea()->mode.pixel_format[TEA_ABGR] = SDL_PIXELFORMAT_ABGR32;
    return 1;
}

/* Texture */
int tea_init_texture(te_Texture *tex, void *data, int w, int h, int format, int usage) {
    if (!tex) { tea_error("te_Texture cannot be NULL"); return 0; }

    if (format < 0 || format >= TEA_PIXELFORMAT_COUNT) {
        tea_error("invalid pixel format: %d", format);
        return 0;
    }
    tex->channels = 3;
    tex->width = w;
    tex->height = h;
    tex->usage = usage;
    if (format == TEA_RGBA) tex->channels = 4;

    if (!data) {
        tex->handle = SDL_CreateTexture(render()->handle, pixel_format(format), SDL_TEXTUREACCESS_STREAMING, w, h);
        return 1;
    }

    int depth, pitch;
    Uint32 _format;

    depth = 8*tex->channels;
    pitch = tex->channels*w;
    _format = pixel_format(format);

    SDL_Surface *surf = SDL_CreateRGBSurfaceWithFormatFrom(data, w, h, depth, pitch, _format);
    if (!surf) {
        tea_error("failed to create SDL_Surface: %s", SDL_GetError());
        return 0;
    }
    tex->handle = SDL_CreateTextureFromSurface(render()->handle, surf);
    SDL_FreeSurface(surf);
    if (!tex->handle) {
        tea_error("failed to create SDL_Texture: %s", SDL_GetError());
        return 0;
    }


    return 1;
}

int tea_deinit_texture(te_Texture *tex) {
    if (!tex) {
        tea_error("texture is NULL");
        return 0;
    }

    return 1;
}


/* Draw Functions */
int tea_point(TEA_TNUM x, TEA_TNUM y) {
    SDL_RenderDrawPoint(render()->handle, x, y);
    return 1;
}

int tea_line(te_Point p0, te_Point p1) {
    SDL_RenderDrawLine(render()->handle, p0.x, p0.y, p1.x, p1.y);
    return 1;
}

// Draw Rects

int _draw_fill_rect(te_Rect *rect) {
  SDL_Rect r;
  r.x = rect->x;
  r.y = rect->y;
  r.w = rect->w;
  r.h = rect->h;
  SDL_RenderFillRect(render()->handle, &r);
  return 1;
}
int _draw_line_rect(te_Rect *rect) {
  SDL_Rect r;
  r.x = rect->x;
  r.y = rect->y;
  r.w = rect->w;
  r.h = rect->h;
  SDL_RenderDrawRect(render()->handle, &r);
  return 1;
}

typedef int(*RenderRectFn)(te_Rect*);
static RenderRectFn rect_fn[2] = {_draw_line_rect, _draw_fill_rect};

int tea_rect(te_Rect *r) {
    if (!r) return 0;
    te_Transform *t = &render()->stat.transform;
    rect_fn[render()->stat.draw_mode](&TEA_RECT(r->x+t->translate.x, r->y+t->translate.y, r->w*t->scale.x, r->h*t->scale.y));
    return 1;
}

/* Circle */
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
    return 1;
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
    return 1;
}

int tea_circle(te_Point p, TEA_TNUM radius) {
    if (render()->stat.draw_mode == TEA_FILL) return tea_circle_fill(p, radius);
    else return tea_circle_line(p, radius);
    return 1;
}

/* Triangle */
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

// Draw textures

void* tea_alloc_texture() {
    return malloc(sizeof(struct te_Texture));
}

int tea_texture(te_Texture *tex, te_Rect *dest, te_Rect *src) {
  te_Point sz;
  sz.x = tex->width;
  sz.y = tex->height;
  SDL_Rect d;
  d.x = dest ? dest->x : 0;
  d.y = dest ? dest->y : 0;
  d.w = dest ? dest->w : sz.x;
  d.h = dest ? dest->h : sz.y;

  SDL_Rect s;
  s.x = src ? src->x : 0;
  s.y = src ? src->y : 0;
  s.w = src ? src->w : sz.x;
  s.h = src ? src->h : sz.y;

  SDL_RenderCopy(render()->handle, tex->handle, &s, &d);
  return 1;
}

int tea_texture_ex(te_Texture *tex, te_Rect *dest, te_Rect *src, TEA_TNUM angle, te_Point origin, int flip) {
  te_Point size;
  size.x = tex->width;
  size.y = tex->height;
  SDL_Rect d;
  te_Transform *t = &render()->stat.transform;
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

  SDL_RenderCopyEx(render()->handle, tex->handle, &s, &d, angle, &sdl_origin, (SDL_RendererFlip)flip);
  return 1;
}

int tea_print(te_Font *font, const char *text, TEA_TNUM x, TEA_TNUM y) {
    if (!font) return 0;
    if (!text) return 0;
    char *p = (char*)text;

    te_Color *dc = &render()->stat.draw_color;

    SDL_SetTextureColorMod(font->tex->handle, dc->r, dc->g, dc->b);
    SDL_SetTextureAlphaMod(font->tex->handle, dc->a);

    while (*p) {
        te_Rect r;
        tea_font_char_rect(font, *p, &r);
        int index = (int)*p;
        tea_texture(font->tex, &TEA_RECT(x, y, r.w, r.h), &r);
        x += font->c[index].ax;

        p++;
    }
    return 1;
}

#if 0
void tea_draw_text(te_Font *font, const char *text, te_Point pos) {
  if (!text) return;
  char *p = (char*)text;

  te_Color *dc = &render()->stat.draw_color;

  SDL_SetTextureColorMod(font->tex->handle, dc->r, dc->g, dc->b);
  SDL_SetTextureAlphaMod(font->tex->handle, dc->a);

  while(*p) {
    te_Rect r;
    tea_font_char_rect(font, *p, &r);

    int index = (int)*p;
    tea_texture(font->tex, &TEA_RECT(pos.x, pos.y, r.w, r.h), &r);
    // tea_draw_texture(font->tex, &r, pos);
    pos.x += font->c[index].ax;

    p++;
  }
}
#endif


static int cross_prod(te_Point v0, te_Point v1) {
  return v0.x*v1.y - v0.y*v1.x;
}

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


        font->c[i].ax = ax * font->scale;
        font->c[i].ay = 0;
        font->c[i].bl = bl * font->scale;
        font->c[i].bw = w;
        font->c[i].bh = h;
        font->c[i].bt = font->baseline + y0;

        tw += w;
        th = MAX(th, h);
    }

    // font->tex = tea_create(tw, th, TEA_RGBA, SDL_TEXTUREACCESS_STREAMING);
    font->tex = tea_create_texture(NULL, tw, th, TEA_RGBA, TEA_TEXTURE_STREAM);

    SDL_SetTextureBlendMode(font->tex->handle, SDL_BLENDMODE_BLEND);

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
    

        if (SDL_LockTexture(font->tex->handle, &r, (void**)&pixels, &pitch) != 0) cst_fatal("failed to lock texture: %s", SDL_GetError());
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
        SDL_UnlockTexture(font->tex->handle);
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
        return 0;
    }
    return 1;
}

int tea_font_char_rect(te_Font *font, const unsigned int c, te_Rect *r) {
    if (!font) {
        cst_trace("te_Font cannot be NULL");
        return 0;
    }
    if (c == '\n' || c == '\t') return 1;
    if (c >= MAX_FONT_CHAR) return 0;

    if (r) *r = TEA_RECT(font->c[c].tx, 0, font->c[c].bw, font->c[c].bh);
    return 1;
}

int tea_font_get_text_width(te_Font *font, const char *text, int len);
int tea_font_get_text_height(te_Font *font, const char *text, int len);

/* Shader */
#if 0
int not_supported(tea_shader(te_Shader *shader, unsigned int vert, unsigned int frag), -1);
int not_supported(tea_shader_load_default(unsigned int *out_v, unsigned int *out_f), -1);
int not_supported(tea_shader_destroy(te_Shader *shader), -1);
int not_supported(tea_sader_compile(const char *source, int type), -1);

int not_supported(tea_shader_load_program(unsigned int vert, unsigned int frag), -1);
int not_supported(tea_shader_send(te_Shader *shader, const char *name, void *value, int uniform_type), -1);

int not_supported(tea_shader_send_count(te_Shader *shader, const char *name, int count, void *value, int uniform_type), -1);
#endif
