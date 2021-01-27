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

#define STB_TRUETYPE_IMPLEMENTATION
#include "../external/stb_truetype.h"

#define STB_IMAGE_IMPLEMENTATION
#include "../external/stb_image.h"

// struct te_Texture {
// #if defined(TEA_GL_RENDER)
//   unsigned int id;
// #else
//   SDL_Texture *tex;
// #endif
//   int width, height;
//   int wrap[2], filter[2];
// };

#define tea() (&_ctx)

#if defined(TEA_GL_RENDER)
typedef struct Texture Texture;

struct Texture {
  unsigned int id;
  int width, height;
  int wrap[2], filter[2];
};

#else
typedef SDL_Texture Texture;
typedef SDL_Texture Canvas;
#endif

struct te_Font {
  te_Texture tex;
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

struct Tea {
  te_Render *render;
  te_Window *window;
  te_Event event;

  te_Color _color;
  TEA_DRAW_MODE _mode;

  Texture *textures[MAX_TEXTURES];
  te_Canvas canvas[MAX_CANVAS];

  const Uint8* key_array;

  struct {
    RenderPointFn point;
    RenderLineFn line;
    RenderRectFn rect[DRAW_MODE_COUNT];
    RenderCircleFn circle[DRAW_MODE_COUNT];
    RenderTriangleFn triangle[DRAW_MODE_COUNT];
    RenderTextureFn texture;
    RenderTextureExFn texture_ex;
  } draw;

  struct {
    unsigned char cp; // canvas pointer
    unsigned char tp; // transform pointer
    unsigned char sp; // shader pointer

    Canvas *canvas[STACK_MAX];
    te_Transform *transform[STACK_MAX];
    te_Shader *shader[STACK_MAX];
  } stack;
  
  struct {
    double prev_time;
    double time;
    float delta;
    int frame;
  } timer;

  unsigned int cp; // command pointer
  te_Command commands[COMMAND_MAX];
};

static Tea _ctx;
static te_Config _conf;

int tea_config_init(te_Config *conf, const char *title, int width, int height) {
  if (!conf) conf = &_conf;
  title = title ? title : CAT("tea ", TEA_VERSION);

  if (title) strcpy((char*)conf->title, title);
  conf->width = width;
  conf->height = height;
  conf->flags = SDL_INIT_VIDEO;
  conf->window_flags = SDL_WINDOW_SHOWN;
  conf->render_flags = SDL_RENDERER_ACCELERATED;

  return 1;
}

Tea* tea_init(te_Config *c) {
  // if (!c) c = &_conf;
  TE_ASSERT(c != NULL, "te_Config cannot be NULL");
  // memset(tea(), 0, sizeof(*tea()));

  SDL_Init(c->flags);
  Tea *ctx = tea_context();
  memset(ctx, 0, sizeof(*ctx));

  ctx->key_array = SDL_GetKeyboardState(NULL);

  memset(ctx->textures, MAX_TEXTURES, sizeof(Texture*));
  memset(ctx->canvas, MAX_CANVAS, sizeof(te_Canvas));

  ctx->window = tea_window_create((const char*)c->title, c->width, c->height, c->window_flags);
  ctx->render = tea_render_create(ctx->window, TEA_SOFTWARE_RENDER);

  ctx->draw.point = tea_render_point;
  ctx->draw.line = tea_render_line;

  ctx->draw.rect[TEA_LINE] = tea_render_rect_line;
  ctx->draw.rect[TEA_FILL] = tea_render_rect_fill;

  ctx->draw.circle[TEA_LINE] = tea_render_circle_line;
  ctx->draw.circle[TEA_FILL] = tea_render_circle_fill;

  ctx->draw.triangle[TEA_LINE] = tea_render_triangle_line;
  ctx->draw.triangle[TEA_FILL] = tea_render_triangle_fill;

  ctx->draw.texture = tea_render_texture;
  ctx->draw.texture_ex = tea_render_texture_ex;

  return ctx;
}

Tea* tea_context() {
  return &_ctx;
}

void tea_terminate() {
  // ctx = ctx ? ctx : &_ctx;
  Tea *ctx = tea();


  tea_window_destroy(ctx->window);
  tea_render_destroy(ctx->render);

  SDL_Quit();
}

void tea_push(te_Command cmd) {
  Tea *ctx = tea();
  ctx->commands[ctx->cp++] = cmd;
}
te_Command* tea_pop(Tea *ctx) {
  return &ctx->commands[--ctx->cp];
}
te_Command* tea_top(Tea *ctx) {
  return &ctx->commands[ctx->cp-1];
}
void tea_repeat(int index) {
  Tea *ctx = tea();
  tea_push(ctx->commands[ctx->cp - index]);
}

te_Command tea_command(TEA_COMMAND_ type) {
  te_Command cmd = {0};
  cmd.type = type;

  return cmd;
}

// te_Command tea_command_draw(Tea *ctx, TEA_DRAW_COMMAND_ type) {
//   te_Command cmd = {0};
//   cmd.type = TEA_COMMAND_DRAW;

//   cmd.draw.type = type;

//   return cmd;
// }
// te_Command tea_command_stack(Tea *ctx, TEA_STACK_COMMAND_ type) {
//   te_Command cmd = {0};
//   cmd.type = TEA_COMMAND_STACK;

//   cmd.stack.type = type;

//   return cmd;
// }

int tea_should_close() {
  return tea()->event.type == SDL_QUIT;
}

void tea_begin_render() {
  Tea *ctx = tea();
  ctx->cp = 0;
  SDL_PollEvent(&ctx->event);
  tea_render_clear((te_Color){0, 0, 0, 255});
}

void tea_attach_canvas(te_Canvas canvas) {
  tea_push_canvas(canvas);

}

void tea_detach_canvas(void) {

}

void tea_push_canvas(te_Canvas canvas) {
  te_Command cmd = tea_command(TEA_PUSH_CANVAS);
  cmd.stack.canvas = canvas;

  tea_push(cmd);
}

te_Canvas tea_pop_canvas() {
  // tea()->cp--;
  te_Command cmd = tea_command(TEA_POP_CANVAS);
  // cmd.s
  tea_push(cmd);
  return 0;
}

static void tea_render_circle(te_Command *cmd) {
  tea()->draw.circle[cmd->draw.fill](cmd->draw.circle.p, cmd->draw.circle.radius);
}

void tea_end_render() {
  Tea *ctx = tea();
  // printf("%d\n", ctx->cp);
  for (int i = 0; i < ctx->cp; i++) {
    te_Command *cmd = &ctx->commands[i];
    // if (cmd->type == TEA_COMMAND_DRAW) {
    tea_render_draw_color(cmd->draw.color);
    // if (cmd->type == TEA_DRAW_TRIANGLE) printf("ok\n");
    switch(cmd->type) {
      case TEA_COMMAND_NONE: break;
      case TEA_DRAW_POINT: tea_render_point(cmd->draw.point); break;
      case TEA_DRAW_LINE: tea_render_line(cmd->draw.line.p0, cmd->draw.line.p1); break;
      case TEA_DRAW_CIRCLE: tea_render_circle(cmd); break;
      case TEA_DRAW_RECT: ctx->draw.rect[cmd->draw.fill](cmd->draw.rect); break;
      case TEA_DRAW_TRIANGLE: ctx->draw.triangle[cmd->draw.fill](cmd->draw.triang.p0, cmd->draw.triang.p1, cmd->draw.triang.p2); break;
      case TEA_DRAW_TEXTURE: ctx->draw.texture(cmd->draw.texture.tex, &cmd->draw.texture.dest, &cmd->draw.texture.src); break;
      case TEA_DRAW_TEXTURE_EX: ctx->draw.texture_ex(cmd->draw.texture.tex, &cmd->draw.texture.dest, &cmd->draw.texture.src, cmd->draw.texture.angle, cmd->draw.texture.origin, cmd->draw.texture.flip); break;
      
      case TEA_PUSH_CANVAS: tea_canvas_set(&cmd->stack.canvas); break;
      case TEA_PUSH_TRANSFORM: break;
      case TEA_PUSH_SHADER: break;
      case TEA_POP_CANVAS: tea_canvas_set(NULL); break;
      case TEA_POP_TRANSFORM: break;
      case TEA_POP_SHADER: break;
      case TEA_COMMAND_COUNT: break;
    }
    // }
  }

  // SDL_RenderPresent(ctx->render->handle);
  tea_render_swap(ctx);
}

void tea_draw_color(te_Color color) {
  Tea *ctx = tea();
  ctx->_color = color;
}
void tea_draw_mode(TEA_DRAW_MODE mode) {
  Tea *ctx = tea();
  ctx->_mode = mode;
}

void tea_draw_point(te_Point p) {
  Tea *ctx = tea();
  // tea_render_draw_color(ctx->render, color);
  // tea_render_point(ctx->render, p);
  // te_Command cmd = tea_command_draw(ctx, DRAW_POINT);
  te_Command cmd = tea_command(TEA_DRAW_POINT);
  // memcpy(cmd.draw.point, p, sizeof(te_Point));
  cmd.draw.point = p;
  cmd.draw.color = ctx->_color;
  cmd.draw.fill = 0;

  tea_push(cmd);
}

void tea_draw_line(te_Point p0, te_Point p1) {
  Tea *ctx = tea();
  // tea_render_draw_color(ctx->render, color);
  // // SDL_RenderDrawLine(ctx->render->handle, p0[0], p0[1], p1[0], p1[1]);
  // tea_render_line(ctx->render, p0, p1);
  // te_Command cmd = tea_command_draw(ctx, DRAW_LINE);
  te_Command cmd = tea_command(TEA_DRAW_LINE);
  // memcpy(cmd.draw.line.p0, p0, sizeof(te_Point));
  // memcpy(cmd.draw.line.p1, p1, sizeof(te_Point));
  cmd.draw.line.p0 = p0;
  cmd.draw.line.p1 = p1;
  cmd.draw.fill = 0;
  cmd.draw.color = ctx->_color;

  tea_push(cmd);
}

void tea_draw_rect(TEA_VALUE x, TEA_VALUE y, TEA_VALUE w, TEA_VALUE h) {
  Tea *ctx = tea();
  // tea_render_draw_color(ctx->render, color);
  // te_Rect r;
  // r.x = x;
  // r.y = y;
  // r.w = w;
  // r.h = h;
  // // SDL_Rect r;
  // // r.x = x;
  // // r.y = y;
  // // r.w = w;
  // // r.h = h;
  // // SDL_RenderDrawRect(ctx->render->handle, &r);
  // switch(mode) {
  //   case TEA_FILL: tea_render_rect_fill(ctx->render, r); break;
  //   case TEA_LINE: tea_render_rect_line(ctx->render, r); break;
  // }
  // te_Command cmd = tea_command_draw(DRAW_RECT);
  te_Command cmd = tea_command(TEA_DRAW_RECT);
  cmd.draw.fill = ctx->_mode;
  cmd.draw.color = ctx->_color;
  cmd.draw.rect = tea_rect(x, y, w, h);

  tea_push(cmd);
}

void tea_draw_circle(te_Point p, TEA_VALUE radius) {
Tea *ctx = tea();
//   tea_render_draw_color(ctx->render, color);

//   switch (mode)
//   {
//     case TEA_FILL: tea_render_circle_fill(ctx->render, p, radius); break;
//     case TEA_LINE: tea_render_circle_line(ctx->render, p, radius); break;
  // }
  // te_Command cmd = tea_command_draw(DRAW_CIRCLE);
  te_Command cmd = tea_command(TEA_DRAW_CIRCLE);
  cmd.draw.fill = ctx->_mode;
  cmd.draw.color = ctx->_color;
  // memcpy(cmd.draw.circle.p, p, sizeof(te_Point));
  cmd.draw.circle.p = p;
  cmd.draw.circle.radius = radius;

  tea_push(cmd);
}

void tea_draw_triangle(te_Point p0, te_Point p1, te_Point p2) {
  Tea *ctx = tea();
  // te_Command cmd = tea_command_draw(DRAW_TRIANGLE);
  te_Command cmd = tea_command(TEA_DRAW_TRIANGLE);
  cmd.draw.fill = ctx->_mode;
  cmd.draw.color = ctx->_color;
  cmd.draw.triang.p0 = p0;
  cmd.draw.triang.p1 = p1;
  cmd.draw.triang.p2 = p2;
  // memcpy(cmd.draw.triang.p0, p0, sizeof(te_Point));
  // memcpy(cmd.draw.triang.p1, p1, sizeof(te_Point));
  // memcpy(cmd.draw.triang.p2, p2, sizeof(te_Point));

  tea_push(cmd);
}

void tea_draw_texture(te_Texture tex, te_Rect *r, te_Point p) {
  Tea *ctx = tea();
  te_Command cmd = tea_command(TEA_DRAW_TEXTURE);
  cmd.draw.texture.tex = tex;

  int x, y;
  x = y = 0;
  int w, h;
  if (r) {
    x = r->x;
    y = r->y;
    w = r->w;
    h = r->h;
  } else SDL_QueryTexture(ctx->textures[tex], NULL, NULL, &w, &h);

  cmd.draw.texture.dest = tea_rect(p.x, p.y, w, h);
  cmd.draw.texture.src = tea_rect(x, y, w, h);
  cmd.draw.color = ctx->_color;

  tea_push(cmd);
}

void tea_draw_texture_scale(te_Texture tex, te_Rect *r, te_Point p, te_Point scale) {
  Tea *ctx = tea();
  te_Command cmd = tea_command(TEA_DRAW_TEXTURE);
  cmd.draw.texture.tex = tex;

  int x, y;
  x = y = 0;
  int w, h;
  if (r) {
    x = r->x;
    y = r->y;
    w = r->w;
    h = r->h;
  } else SDL_QueryTexture(ctx->textures[tex], NULL, NULL, &w, &h);

  cmd.draw.texture.dest = tea_rect(p.x, p.y, w*scale.x, h*scale.y);
  cmd.draw.texture.src = tea_rect(x, y, w, h);
  cmd.draw.color = ctx->_color;

  tea_push(cmd);
}

void tea_draw_texture_ex(te_Texture tex, te_Rect *r, te_Point p, TEA_VALUE angle, te_Point scale, te_Point origin) {
  Tea *ctx = tea();
  te_RenderFlip flip = 0;
  if (scale.x < 0) {
    scale.x *= -1;
    flip |= TEA_FLIP_H;
  }

  if (scale.y < 0) {
    scale.y *= -1;
    flip |= TEA_FLIP_V;
  }

  te_Command cmd = tea_command(TEA_DRAW_TEXTURE_EX);
  cmd.draw.texture.tex = tex;

  int x, y;
  te_Point size;
  x = y = 0;
  if (r) {
    x = r->x;
    y = r->y;
    size.x = r->w;
    size.y = r->h;
  } else tea_texture_size(tex, &size);

  cmd.draw.texture.dest = tea_rect(p.x, p.y, size.x*scale.x, size.y*scale.y);
  cmd.draw.texture.src = tea_rect(x, y, size.x, size.y);
  cmd.draw.color = ctx->_color;
  cmd.draw.texture.angle = angle;
  cmd.draw.texture.flip = flip;
  cmd.draw.texture.origin = tea_point(origin.x*scale.x, origin.y*scale.y);


  tea_push(cmd);
}

void tea_draw_canvas(te_Canvas canvas, te_Rect *rect, te_Point pos) {
  tea_draw_texture(canvas, rect, pos);
}

void tea_draw_text(te_Font *font, const char *text, te_Point pos) {
  if (!text) return;
  Tea *tea = tea();

  char *p = (char*)text;
  // int w, h;
  // SDL_QueryTexture(font->tex, NULL, NULL, &w, &h);


  // r.x = pos.x;
  // r.y = pos.y;
  // r.w = w;
  // r.h = h;
  SDL_SetTextureColorMod(tea->textures[font->tex], tea->_color.r, tea->_color.g, tea->_color.b);
  SDL_SetTextureAlphaMod(tea->textures[font->tex], tea->_color.a);
  // tea_render_texture(tea, font->tex, &r, NULL);
  while(*p) {
    te_Rect r;
    tea_font_char_rect(font, *p, &r);
    // te_Rect dest = tea_rect(pos.x, pos.y, r.w, r.h);
    int index = (int)*p;
    tea_draw_texture(font->tex, &r, pos);
    pos.x += font->c[index].ax;
    // tea_render_texture(tea, font->tex, &dest, &r);
    p++;
  }
}

te_Window* tea_window_create(const char *title, int width, int height, int flags) {
  // te_Window *w = (te_Window*)malloc(sizeof(*w));
  // TE_ASSERT(w != NULL, "failed to create window");

  // w->handle = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, flags);

  // w->width = width;
  // w->height = height;
  te_Window *w = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, flags);

  return w;
}

void tea_window_destroy(te_Window *window) {
  if (!window) return;

  SDL_DestroyWindow(window);
}

te_Render* tea_render_create(te_Window *window, TEA_RENDER_FLAGS flags) {
  TE_ASSERT(window != NULL, "te_Window cannot be NULL");
  te_Render *r = SDL_CreateRenderer(window, -1, flags);

  // te_Render *r = (te_Render*)malloc(sizeof(*r));
  // TE_ASSERT(r != NULL, "failed to create render");

  // r->handle = SDL_CreateRenderer(window->handle, -1, flags);

  // r->draw.point = tea_render_point;
  // r->draw.line = tea_render_line;

  // r->draw.rect[TEA_LINE] = tea_render_rect_line;
  // r->draw.rect[TEA_FILL] = tea_render_rect_fill;

  // r->draw.circle[TEA_LINE] = tea_render_circle_line;
  // r->draw.circle[TEA_FILL] = tea_render_circle_fill;

  // r->draw.triangle[TEA_LINE] = tea_render_triangle_line;
  // r->draw.triangle[TEA_FILL] = tea_render_triangle_line;

  return r;
}

void tea_render_destroy(te_Render *render) {
  if (!render) return;

  SDL_DestroyRenderer(render);
  // free(render);
}

void tea_render_draw_color(te_Color color) {
  Tea *t = tea();
  SDL_SetRenderDrawColor(t->render, color.r, color.g, color.b, color.a);
}

void tea_render_clear(te_Color color) {
  Tea *t = tea();
  TE_ASSERT(t != NULL, "Tea cannot be NULL");

  SDL_SetRenderDrawColor(t->render, color.r, color.g, color.b, color.a);
  SDL_RenderClear(t->render);
}

void tea_render_swap(Tea *t) {
  TE_ASSERT(t != NULL, "Tea cannot be NULL");
  SDL_RenderPresent(t->render);
}

void tea_render_point(te_Point p) {
  Tea *t = tea();
  // tea_render_draw_color(render, color);
  TE_ASSERT(t != NULL, "Tea cannot be NULL");
  SDL_RenderDrawPoint(t->render, p.x, p.y);
}
void tea_render_line(te_Point p0, te_Point p1) {
  Tea *t = tea();
  TE_ASSERT(t != NULL, "Tea cannot be NULL");
  SDL_RenderDrawLine(t->render, p0.x, p0.y, p1.x, p1.y);
}

void tea_render_rect_fill(RECT_ARGS) {
  SDL_Rect r;
  r.x = rect.x;
  r.y = rect.y;
  r.w = rect.w;
  r.h = rect.h;
  SDL_RenderFillRect(tea()->render, &r);
}
void tea_render_rect_line(RECT_ARGS) {
  SDL_Rect r;
  r.x = rect.x;
  r.y = rect.y;
  r.w = rect.w;
  r.h = rect.h;
  SDL_RenderDrawRect(tea()->render, &r);
}

void tea_render_circle_fill(CIRC_ARGS) {
  int x = 0;
	int y = radius;

	int P = 1 - radius;

	if (radius > 0) SDL_RenderDrawLine(tea()->render, p.x + radius, p.y, p.x - radius, p.y);

	while (x <= y) {
		if (P < 0) P += 2*x + 3;
		else {
			P += (2*(x-y))+5;
			y--;
		}
		x++;

		if (x > y) break;

		SDL_RenderDrawLine(tea()->render, p.x - x, p.y + y, p.x + x, p.y + y);
		SDL_RenderDrawLine(tea()->render, p.x + x, p.y - y, p.x - x, p.y - y);

		if (x != y) {
			SDL_RenderDrawLine(tea()->render, p.x - y, p.y + x, p.x + y, p.y + x);
			SDL_RenderDrawLine(tea()->render, p.x + y, p.y - x, p.x - y, p.y - x);
		}
	}
}

void tea_render_circle_line(CIRC_ARGS) {
  int x = -radius;
	int y = 0;
	int r = radius;
	int err = 2 - 2*r;

	do {
		SDL_RenderDrawPoint(tea()->render, p.x - x, p.y + y);
		SDL_RenderDrawPoint(tea()->render, p.x - y, p.y - x);
		SDL_RenderDrawPoint(tea()->render, p.x + x, p.y - y);
		SDL_RenderDrawPoint(tea()->render, p.x + y, p.y + x);
		r = err;
		if (r <= y) err += ++y*2+1;
		if (r > x || err > y) err += ++x*2+1;
	} while (x < 0);
}

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
    tea_render_line(tea_point(curx1, scanline_y), tea_point(curx2, scanline_y));
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
    tea_render_line(tea_point(curx1, scanline_y), tea_point(curx2, scanline_y));
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

void tea_render_triangle_fill(TRIANG_ARGS) {
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

  // tea_render_triangle_line(t, p0, p1, p2);

  // if (p0.y <= p1.y) {
  //   if (p0.y <= p2.y) {
  //     pp0 = p0;
  //     pp1 = 
  //   }
  //   else {
  //     pp0 = p1;
  //   }
  // }
}

void tea_render_triangle_line(TRIANG_ARGS) {
  // printf("qqqq\n");
  tea_render_line(p0, p1);
  tea_render_line(p1, p2);
  tea_render_line(p2, p0);
}
void tea_render_texture(te_Texture id, te_Rect *dest, te_Rect *src) {
  te_Point sz;
  // SDL_QueryTexture(t, NULL, NULL, &w, &h);
  tea_texture_size(id, &sz);
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
  // printf("q\n");
  // SDL_Rect r = (SDL_Rect){dest->x, dest->y, dest->w, dest->h};


  SDL_RenderCopy(tea()->render, tea()->textures[id], &s, &d);
}

void tea_render_texture_ex(te_Texture id, te_Rect *dest, te_Rect *src, TEA_VALUE angle, te_Point origin, te_RenderFlip flip) {
  te_Point size;
  tea_texture_size(id, &size);
  SDL_Rect d;
  d.x = (dest ? dest->x : 0) - origin.x;
  d.y = (dest ? dest->y : 0) - origin.y;
  d.w = dest ? dest->w : size.x - origin.x;
  d.h = dest ? dest->h : size.y - origin.y;
  // d.x = d.y = 0;
  // d.w = d.h = 256;
  // printf("%d %d %d %d\n", d.x, d.y, d.w, d.h);

  SDL_Rect s;
  s.x = src ? src->x : 0;
  s.y = src ? src->y : 0;
  s.w = src ? src->w : size.x;
  s.h = src ? src->h : size.y;

  SDL_Point sdl_origin = {origin.x, origin.y};
  // printf("%d %d\n", sdl_origin.x, sdl_origin.y);

  SDL_RenderCopyEx(tea()->render, tea()->textures[id], &s, &d, angle, &sdl_origin, (SDL_RendererFlip)flip);
}

void tea_render_canvas(te_Canvas id, te_Rect *dest, te_Rect *src) {
  /*int w, h;
  Texture *tex = tea()->canvas[id];
  SDL_QueryTexture(tex, NULL, NULL,  &w, &h);

  te_Point size = tea_point(w, h);
  SDL_Rect r;
  if (dest) 
    r = (SDL_Rect){dest->x, dest->y, dest->w, dest->h};
  else 
    r = (SDL_Rect){0, 0, size.x, size.y};
  
  SDL_Rect s;
  if (src) s = (SDL_Rect){src->x, src->y, src->w, src->h};
  else s = (SDL_Rect){0, 0, size.x, size.y};
  
  SDL_RenderCopy(tea()->render, tex, &s, &r);*/
  tea_render_texture(id, dest, src);
}

void tea_render_text(te_Font *font, const char *text, te_Point pos) {
  if (!text) return;
  Tea *tea = tea();

  char *p = (char*)text;
  te_Rect r;
  // int w, h;
  // SDL_QueryTexture(font->tex, NULL, NULL, &w, &h);
  te_Point size;
  tea_texture_size(font->tex, &size);


  r.x = pos.x;
  r.y = pos.y;
  r.w = size.x;
  r.h = size.y;
  SDL_SetTextureColorMod(tea->textures[font->tex], tea->_color.r, tea->_color.g, tea->_color.b);
  SDL_SetTextureAlphaMod(tea->textures[font->tex], tea->_color.a);
  // tea_render_texture(tea, font->tex, &r, NULL);
  while(*p) {
    tea_font_char_rect(font, *p, &r);
    te_Rect dest = tea_rect(pos.x, pos.y, r.w, r.h);
    int index = (int)*p;
    pos.x += font->c[index].ax;
    tea_render_texture(font->tex, &dest, &r);
    p++;
  }
}

static int _set_texture(Texture *tex) {
  int i = 0;
  while (i < MAX_TEXTURES) {
    if (tea()->textures[i] == NULL) {
      tea()->textures[i] = tex;
      return i;
    }
    i++;
  }

  tea_error("max textures reached");

  return -1;
}

static int _create_texture(int w, int h, unsigned int format, int access) {
  Tea *tea = tea();
  return _set_texture(SDL_CreateTexture(tea->render, format, access, w, h));
}


te_Texture tea_texture(int w, int h, unsigned int format) {
  return _create_texture(w, h, format, SDL_TEXTUREACCESS_STATIC);
}

te_Texture tea_texture_load(const char *str) {
  Tea *t = tea();
  int req_format = STBI_rgb_alpha;
  int w, h, format;
  unsigned char *data = stbi_load(str, &w, &h, &format, req_format);
  TE_ASSERT(data != NULL, "failed to load image");

  int depth, pitch;
  Uint32 pixel_format;


  if (req_format == STBI_rgb) {
    depth = 24;
    pitch = 3*w;
    pixel_format = SDL_PIXELFORMAT_RGB24;
  } else {
    depth = 32;
    pitch = 4*w;
    // printf("teste\n");
    pixel_format = SDL_PIXELFORMAT_RGBA32;
  }

  SDL_Surface *surf = SDL_CreateRGBSurfaceWithFormatFrom(data, w, h, depth, pitch, pixel_format);

  TE_ASSERT(surf != NULL, "Failed to create surface");

  Texture *tex = SDL_CreateTextureFromSurface(t->render, surf);

  TE_ASSERT(tex != NULL, "Failed to create texture");
  SDL_FreeSurface(surf);

  return _set_texture(tex);
}

te_Texture* tea_texture_memory(unsigned char *pixels, int w, int h, unsigned int format) {
  return NULL;
}


int tea_texture_width(te_Texture id) {
  int w;
  Texture *tex = tea()->textures[id];
  TE_ASSERT(tex != NULL, "invalid texture");

  SDL_QueryTexture(tex, NULL, NULL, &w, NULL);
  return w;
}
int tea_texture_height(te_Texture id) {
  int h;
  Texture *tex = tea()->textures[id];
  TE_ASSERT(tex != NULL, "invalid texture");

  SDL_QueryTexture(tex, NULL, NULL, NULL, &h);
  return h;
}
void tea_texture_size(te_Texture id, te_Point *size) {
  int w, h;
  Texture *tex = tea()->textures[id];
  TE_ASSERT(tex != NULL, "invalid texture");

  SDL_QueryTexture(tex, NULL, NULL, &w, &h);
  if (size) *size = tea_point(w, h);
}

/* Font */

te_Font* tea_font(const void *data, size_t buf_size, int font_size) {
  te_Font *f = malloc(sizeof(*f));
  TE_ASSERT(f != NULL, "Failed to alloc a block for font");

  tea_font_init(f, data, buf_size, font_size);

  return f;
}

te_Font* tea_font_load(const char *filename, int font_size) {
  size_t sz;
  FILE *fp;
  fp = fopen(filename, "rb");
  TE_ASSERT(fp != NULL, "Failed to load font");

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
  Tea *tea = tea();
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

  // printf("%d %d\n", tw, th);

  SDL_PixelFormat pixel_format;
  pixel_format.format = SDL_PIXELFORMAT_RGBA32;
  // font->tex = tea_texture(tea, tw, th, pixel_format.format);
  font->tex = _create_texture(tw, th, pixel_format.format, SDL_TEXTUREACCESS_STREAMING);

  SDL_SetTextureBlendMode(tea->textures[font->tex], SDL_BLENDMODE_BLEND);

  int x = 0;
  for (i = 0; i < MAX_FONT_CHAR; i++) {
    int ww = font->c[i].bw;
    int hh = font->c[i].bh;
    int ssize = ww * hh;
    int ox, oy;

    unsigned char *bitmap = stbtt_GetCodepointBitmap(&font->info, 0, font->scale, i, NULL, NULL, &ox, &oy);
    // printf("%p\n", bitmap);
    // Uint32 pixels[ssize];
    // unsigned char pixels[ssize];
    // for (int j = 0; j < ssize; j += 4) {
    //   int ii = j / 4;

    //   pixels[j] = 255;
    //   pixels[j+1] = 255;
    //   pixels[j+2] = 255;
    //   pixels[j+3] = bitmap[ii];
    //   // printf("%d\n", bitmap[ii]);
    // }
    Uint32 *pixels = NULL;
    int pitch;

    SDL_Rect r;
    r.x = x;
    r.y = 0;
    r.w = ww;
    r.h = hh;

    if (SDL_LockTexture(tea->textures[font->tex], &r, (void**)&pixels, &pitch) != 0) {
      fprintf(stderr, "[tea] error: %s\n", SDL_GetError());
      exit(1);
    }
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
    SDL_UnlockTexture(tea->textures[font->tex]);

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
  if (c == '\n' || c == '\t') return;
  if (c >= MAX_FONT_CHAR) return;

  if (r) *r = tea_rect(font->c[c].tx, 0, font->c[c].bw, font->c[c].bh);
}

int tea_font_get_text_width(te_Font *font, const char *text, int len);
int tea_font_get_text_height(te_Font *font, const char *text, int len);


/* Debug */

void tea_error(const char *msg) {
  fprintf(stderr, "[tea] error: %s\n", msg);
  exit(1);
}



/***************** Garbage ******************/

void tea_render_triangle_fill0(TRIANG_ARGS) {
  Tea *tea = tea();
  // tea_render_triangle_line(t, p0, p1, p2);
  int minX = tea_min(p0.x, tea_min(p1.x, p2.x));
  int maxX = tea_max(p0.x, tea_max(p1.x, p2.x));

  int minY = tea_min(p0.y, tea_min(p1.y, p2.y));
  int maxY = tea_max(p0.y, tea_max(p1.y, p2.y));

  int y;
  for (y = minY; y <= maxY; y++) {
    // for (x = minX; x <= maxX; x++) {

      // te_Point q = tea_point(p0.x, y - p0.y);

      te_Point sp0 = tea_point(minX, y);
      te_Point sp1 = tea_point(maxX, y);
      int intersect = 0;
      te_Point inter_points[2];

      // SDL_RenderDrawLine(tea->render, sp0.x, sp0.y, pp1.x, pp1.y);
      // te_Color c = tea->_color;
      // tea_render_draw_color(tea, tea_color(255, 0, 0));
      // tea_render_line(tea, sp1, sp0);
      // tea_render_draw_color(tea, c);


      // int cross = cross_prod(tea_point(p0.x-p1.x, pp0.y-pp1.y), tea_point(p0.y-p1.y, pp0.x-pp1.x));
      // int cross = get_intersection(pp0, pp1, p0, p1, &inter_points[intersect]);
      if (get_intersection(p0, p2, sp0, sp1, &inter_points[intersect])) intersect++;
      // printf("%d\n", intersect);
      if (get_intersection(p0, p1, sp0, sp1, &inter_points[intersect])) intersect++;
      // printf("%d\n", intersect);
      if (get_intersection(p2, p1, sp0, sp1, &inter_points[intersect])) intersect++;
      printf("%d\n", intersect);

      // printf("%d\n", intersect);


      for (int i = 0; i < intersect; i++) {
        // printf("p%d: %f %f\n", i, inter_points[i].x, inter_points[i].y);
      }
      // if (intersect > 0) 
        // SDL_RenderDrawLine(tea->render, inter_points[0].x, inter_points[0].y, inter_points[1].x, inter_points[1].y);
      switch (intersect) {
        case 0: break;
        case 1: SDL_RenderDrawPoint(tea->render, inter_points[0].x, inter_points[0].y); break;
        case 2: SDL_RenderDrawLine(tea->render, inter_points[0].x, inter_points[0].y, inter_points[1].x, inter_points[1].y); break;
      }

      // float s = (float)cross_prod(q, vs1) / c;
      // float tt = (float)cross_prod(vs0, q) / c;

      // if (s >= 0 && tt >= 0 && (s + tt <= 1)) SDL_RenderDrawPoint(t->render, x, y);

      // if ((p0.x - p1.x) * (y - p0.y) - (p0.y - p1.y) * (x - p0.x) > 0 &&
      //     (p1.x - p2.x) * (y - p1.y) - (p1.y - p2.y) * (x - p1.x) > 0 &&
      //     (p2.x - p0.x) * (y - p2.y) - dy * (x - p2.x) > 0){
      //       printf("porra eh, entra aqui n?\n");
      //       SDL_RenderDrawPoint(t->render, x, y);
      // }
    // }
  }
}


// Canvas

static int _push_canvas(te_Canvas canvas) {
  int i = 0;
  while (i < MAX_CANVAS) {
    if (tea()->canvas[i] == 0) {
      tea()->canvas[i] = canvas;
      return i;
    }
    i++;
  }

  tea_error("max canvas reached");

  return -1;
}

static int _create_canvas(int w, int h, unsigned int format) {
  te_Canvas c = _create_texture(w, h, format, SDL_TEXTUREACCESS_TARGET);
  _push_canvas(c);
  return c;
}

te_Canvas tea_canvas(int width, int height) {
  TE_ASSERT(width > 0, "Canvas width must be greater than zero");
  TE_ASSERT(height > 0, "Canvas height must be greater than zero");
  te_Canvas canvas = _create_canvas(width, height, SDL_PIXELFORMAT_RGBA32);
  // printf("%d\n", canvas);

  return canvas;
}

void tea_canvas_set(te_Canvas *canvas) {
  if (canvas) {
    SDL_SetRenderTarget(tea()->render, tea()->textures[*canvas]);
    tea_render_clear(BLACK);
  } else SDL_SetRenderTarget(tea()->render, NULL);

}

/*******************
 *      Input      *
 *******************/

int tea_key_is_down(int key) {
  return tea()->key_array[key];
}

int tea_key_is_up(int key) {
  return !tea_key_is_down(key);
}

int tea_key_was_pressed(int key);
int tea_key_was_released(int key);

int tea_mouse_is_down(int button);
int tea_mouse_is_up(int button);
int tea_mouse_was_pressed(int button);
int tea_mouse_was_released(int button);

int tea_joy_axis(int jid);
int tea_joy_button_is_down(int jid, int button);
int tea_joy_button_is_up(int jid, int button);
int tea_joy_button_was_pressed(int jid, int button);
int tea_joy_button_was_released(int jid, int button);