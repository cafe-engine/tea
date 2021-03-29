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
#include "SDL_pixels.h"
#include "SDL_render.h"
#include "tea_api.h"

#include "stb_image.h"
#include "stb_truetype.h"

struct te_Texture {
    int usage;
    void *data;
    unsigned int width, height;
    unsigned int wrap[2];
    int channels;
};

struct te_Font {
    te_Texture *tex;
    struct {
        int ax;
        int ay;

        int bw;
        int bh;

        int bl;
        int bt;

        float tx;
    } c[MAX_FONT_CHAR];

    unsigned char size;

    stbtt_fontinfo info;
    float ptsize;
    float scale;
    int baseline;
    void *data;
};

struct Render {
    void *sdlr;
    te_Texture screen;
    te_Texture *buffer;
    SDL_Texture *out;
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
    return 1;
}

int tea_render_init(te_Render *r, te_Window *w, int flags) {
    struct Render *render = malloc(sizeof(*render));
    r->handle = render;
    render->sdlr = SDL_CreateRenderer(w->handle, -1, SDL_RENDERER_SOFTWARE);
    tea_init_texture(&render->screen, NULL, w->width, w->height, TEA_RGBA, TEA_TEXTURE_TARGET);
    r->flags = flags;
    render->buffer = &render->screen;

    render->out = SDL_CreateTexture(render->sdlr, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, w->width, w->height);

    r->stat.clear_color = BLACK;
    r->stat.draw_color = WHITE;
    r->stat.draw_mode = TEA_LINE;
    r->stat.tex = NULL;


    return 1;
}

int tea_render_deinit(te_Render *r) {
    struct Render *render = r->handle;
    SDL_DestroyRenderer(render->sdlr);
    tea_deinit_texture(&render->screen);

    return 1;
}

int tea_render_begin(te_Render *r) { return 1; }
int tea_render_end(te_Render *r) { return 1; }

int tea_set_render_target(te_Render *r, te_Texture *tex) {
    struct Render *render = r->handle;
    if (!tex) render->buffer = &render->screen;
    else render->buffer = tex;

    return 1;
}

int tea_render_swap(te_Render *r) {
    struct Render *render = r->handle;
    te_Texture *screen = render->buffer;
    int size = screen->width*screen->height*screen->channels;
    te_Byte *data = screen->data;
#if 0
    int scale = 1;
    for (int i = 0; i < size; i += screen->channels) {
        unsigned char *d = data + i;
        SDL_SetRenderDrawColor(render->sdlr, d[0], d[1], d[2], 255);
        int index = i / screen->channels;
        int xx = index%screen->width;
        int yy = floor(index / screen->width);
        // SDL_RenderDrawPoint(render->sdlr, xx, yy);
        SDL_Rect sr;
        sr.x = xx*scale;
        sr.y = yy*scale;
        sr.h = 1*scale;
        sr.w = 1*scale;
        SDL_RenderFillRect(render->sdlr, &sr);
    }
#endif

    Uint32 *pixels;
    int pitch;

    if (SDL_LockTexture(render->out, NULL, (void**)&pixels, &pitch)) {
        cst_fatal("deu não");
    }

    SDL_PixelFormat pixelFormat;
    pixelFormat.format = screen->channels == 3 ? pixel_format(TEA_RGB) : pixel_format(TEA_RGBA);
    int chan = screen->channels;
    for (int i = 0; i < size; i += chan) {
        int index = i / screen->channels;
        unsigned char r, g, b;
        r = data[i];
        g = data[i+1];
        b = data[i+2];

        /*Uint32 color = SDL_MapRGB(&pixelFormat, data[i], data[i+1], data[i+2]);
        cst_trace("%d", i);
        pixels[index] = color;*/
        Uint32 color = 0;
        color = (color << 8) | b;
        color = (color << 8) | g;
        color = (color << 8) | r;
        pixels[index] = color;
    }

    SDL_UnlockTexture(render->out);

    SDL_RenderCopy(render->sdlr, render->out, NULL, NULL);
    SDL_RenderPresent(render->sdlr);
    return 1;
}

int tea_render_clear(te_Render *r) {
    struct Render *render = r->handle;
    int sz = render->buffer->width*render->buffer->height*render->buffer->channels;
    char *data = render->buffer->data;
    for (int i = 0; i < sz; i += render->buffer->channels) {
        data[i] = r->stat.clear_color.r;
        data[i+1] = r->stat.clear_color.g;
        data[i+2] = r->stat.clear_color.b;
        data[i+3] = r->stat.clear_color.a;
    }

    return 1;
}
int tea_render_color(te_Render *r, te_Color col) {
    return 1;
}

int tea_render_mode(te_Render *r, int mode) {
    return 1;
}

/* Texture */

int tea_texture_info(te_Texture *tex, te_TextureInfo *out) {
    if (!tex) return 0;
    if (!out) return 0;

    out->size.w = tex->width;
    out->size.h = tex->height;
    out->format = tex->channels;
    out->usage = tex->usage;
    return 1;
}

int tea_init_texture(te_Texture *tex, void *data, int w, int h, int format, int usage) {
    tex->width = w;
    tex->height = h;
    tex->usage = usage;
    int channels = 3;
    if (format == TEA_RGBA) channels = 4;

    tex->channels = channels;

    if (!data) tex->data = calloc(w*h*channels, sizeof(te_Byte));
    else tex->data = data;

    return 1;
}

int tea_deinit_texture(te_Texture *tex) {
    if (!tex) {
        // tea_error("texture is NULL");
        return 0;
    }
    free(tex->data);

    return 1;
}

/* Draw Functions */

/* Point */
int tea_point(TEA_TNUM x, TEA_TNUM y) {
    struct Render *r = render()->handle;
    te_Texture *screen = r->buffer;
    if (render()->stat.draw_color.a == 0) return 1;


    int offset = (x + (y * (screen->width))) * screen->channels;

    unsigned char *data = screen->data;
    data += offset;
    data[0] = render()->stat.draw_color.r;
    data[1] = render()->stat.draw_color.g;
    data[2] = render()->stat.draw_color.b;
    if (screen->channels > 3) data[3] = render()->stat.draw_color.a;

    return 1;
}

/* Line */
int tea_line(te_Point p0, te_Point p1) {
    TEA_TNUM dx = p1.x - p0.x;
    TEA_TNUM dy = p1.y - p0.y;
    int steps = MAX(fabs(dx),fabs(dy));
    float xinc = dx / (float)steps;
    float yinc = dy / (float)steps;

    tea_point(p0.x, p0.y);
    TEA_TNUM x = p0.x;
    TEA_TNUM y = p0.y;
    for (int v = 0; v < steps; v++) {
        x += xinc;
        y += yinc;
        tea_point(x, y);
    }

    return 1;
}

/* Rect */
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

/* Texture */
void* tea_alloc_texture() {
    return malloc(sizeof(struct te_Texture));
}

static int tea_get_pixel(te_Texture *tex, int x, int y, te_Color *pixel) {
    if (!tex) return 0;
    if (!pixel) return 0;

    int offset = (x+(y*tex->width))*tex->channels;
    unsigned char *data = tex->data;
    data += offset;

    pixel->r = data[0];
    pixel->g = data[1];
    pixel->b = data[2];
    pixel->a = data[3];

    return 1;
}

int tea_texture(te_Texture *tex, te_Rect *dest, te_Rect *src) {
    if (!tex) return 0;
    dest = dest ? dest : &TEA_RECT(0, 0, tex->width, tex->height);
    src = src ? src : &TEA_RECT(0, 0, tex->width, tex->height);
    int size = src->w*src->h*tex->channels;

    te_Color draw_col = render()->stat.draw_color;

    for (int i = 0; i < size; i += tex->channels) {
        int index = i / tex->channels;
        int xx = index % (int)src->w;
        int yy = floor(index / src->w);
        te_Color col;
        tea_get_pixel(tex, src->x+xx, src->y+yy, &col);

        tea_draw_color(col);

        tea_point(dest->x+xx, dest->y+yy);
    }

    tea_draw_color(draw_col);

    return 1;
}
int tea_texture_ex(te_Texture *tex, te_Rect *dest, te_Rect *src, TEA_TNUM angle, te_Point origin, int flip) {
    return tea_texture(tex, dest, src);
}

int tea_print(te_Font *font, const char *text, TEA_TNUM x, TEA_TNUM y) {
    if (!font) return 0;
    if (!text) return 0;
    char *p = (char*)text;

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
    memset(font->tex->data, 0, font->tex->width*font->tex->height*font->tex->channels);

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
    

        pixels = font->tex->data + (x*font->tex->channels);
        pitch = font->tex->channels*font->tex->width;
        int yy = 0;
        for (int j = 0; j < ssize; j++) {
            int xx = j % ww;
            if (j != 0 && xx == 0) yy++;
            int index = xx + (yy * (pitch / font->tex->channels));
      // printf("%d\n", index);
      // printf("teste %p, %d %d\n", pixels, pixels[index], bitmap[j]);
      // Uint32 pp = SDL_MapRGBA(&pixel_format, 255, 255, 255, bitmap[j]);
            Uint32 pp = bitmap[j];
            pp <<= 24;
            pp |= 0xffffff;
            pixels[index] = pp;
        }
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
