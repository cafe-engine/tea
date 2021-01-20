#ifndef TEA_H
#define TEA_H

#define TE_API extern
#include <SDL2/SDL.h>

#if defined(TEA_GL_RENDER)
#include <SDL2/SDL_opengl.h>
#endif

#define TEA_VERSION "0.1.0"
#define CAT(a, b) a b

#define TEA_VALUE float
#define COMMAND_MAX 2048
#define STACK_MAX 255

#define MAX_FONT_CHAR 256
#define MAX_TEXTURES 64

#define STR(expr) #expr
#define TE_ASSERT(expr, msg) if (!(expr)) tea_error(msg);

#define tea_max(a, b) ((a) > (b) ? (a) : (b))
#define tea_min(a, b) ((a) > (b) ? (b) : (a))

#define tea_color(r, g, b) ((te_Color){(r), (g), (b), 255})
#define WHITE tea_color(255, 255, 255)
#define BLACK tea_color(0, 0, 0)

#define tea_vec2(x, y) (te_Vec2){(x), (y)}
#define tea_rect(x, y, w, h) ((te_Rect){(x), (y), (w), (h)})
#define tea_point(x, y) ((te_Point){(x), (y)})

typedef struct Tea Tea;
typedef struct te_Config te_Config;

typedef SDL_Window te_Window;
typedef SDL_Renderer te_Render;
typedef SDL_Event te_Event;

typedef struct te_Command te_Command;
typedef struct te_DrawCommand te_DrawCommand;
typedef struct te_StackCommand te_StackCommand;

typedef unsigned int te_Texture;
typedef struct te_Font te_Font;
typedef struct te_Canvas te_Canvas;
typedef struct te_Transform te_Transform;
typedef struct te_Shader te_Shader;

// typedef TEA_VALUE te_Vec2[2];
// typedef TEA_VALUE te_Vec3[3];
// typedef TEA_VALUE te_Vec4[4];
// typedef TEA_VALUE te_Matrix[4][4];
typedef struct { TEA_VALUE x, y; } te_Point;
typedef struct { TEA_VALUE x, y, w, h; } te_Rect;
typedef struct { unsigned char r, g, b, a; } te_Color;
typedef enum { TEA_FLIP_NONE = 0, TEA_FLIP_H = (1 << 0), TEA_FLIP_V = (1 << 1) } te_RenderFlip;


typedef enum {
  TEA_RED,
  TEA_GREEN,
  TEA_BLUE,
  TEA_ALPHA,
  TEA_RGB,
  TEA_RGBA,

  TEA_PIXELFORMAT_COUNT
} TEA_PIXELFORMAT;

typedef enum {
  TEA_DEFAULT = 0,
  TEA_SOFTWARE_RENDER = (1 << 0),
  TEA_HARDWARE_RENDER = (1 << 1),
  TEA_BATCH_RENDER = (1 << 2)
} TEA_RENDER_FLAGS;

typedef enum {
  TEA_LINE = 0,
  TEA_FILL,

  DRAW_MODE_COUNT
} TEA_DRAW_MODE;

typedef void(*RenderPointFn)(Tea*, te_Point);
typedef void(*RenderLineFn)(Tea*, te_Point, te_Point);
typedef void(*RenderRectFn)(Tea*, te_Rect);
typedef void(*RenderCircleFn)(Tea*, te_Point, TEA_VALUE);
typedef void(*RenderTriangleFn)(Tea*, te_Point, te_Point, te_Point);
typedef void(*RenderTextureFn)(Tea*, te_Texture, te_Rect*, te_Rect*);
typedef void(*RenderTextureExFn)(Tea*, te_Texture, te_Rect*, te_Rect*, TEA_VALUE, te_Point, te_RenderFlip flip);

// typedef enum {
//   DRAW_NONE = 0,
//   DRAW_POINT,
//   DRAW_LINE,
//   DRAW_RECT,
//   DRAW_CIRCLE,
//   DRAW_TRIANGLE,
//   DRAW_TEXTURE
// } TEA_DRAW_COMMAND_;

// typedef enum {
//   STACK_NONE = 0,
//   PUSH_CANVAS,
//   PUSH_TRANSFORM,
//   PUSH_SHADER,
//   POP_CANVAS,
//   POP_TRANSFORM,
//   POP_SHADER
// } TEA_STACK_COMMAND_;

typedef enum {
  TEA_COMMAND_NONE = 0,
  // TEA_COMMAND_DRAW,
  // TEA_COMMAND_STACK
  TEA_DRAW_POINT,
  TEA_DRAW_LINE,
  TEA_DRAW_RECT,
  TEA_DRAW_CIRCLE,
  TEA_DRAW_TRIANGLE,
  TEA_DRAW_TEXTURE,
  TEA_DRAW_TEXTURE_EX,

  TEA_PUSH_CANVAS,
  TEA_PUSH_TRANSFORM,
  TEA_PUSH_SHADER,
  TEA_POP_CANVAS,
  TEA_POP_TRANSFORM,
  TEA_POP_SHADER,

  TEA_COMMAND_COUNT
} TEA_COMMAND_;

struct te_DrawCommand {
  // TEA_DRAW_COMMAND_ type;
  int fill;
  union {
    te_Point point;
    struct { te_Point p0, p1; } line;
    te_Rect rect;
    struct { te_Point p; TEA_VALUE radius; } circle;
    struct { te_Point p0, p1, p2; } triang;
    struct {
      te_Texture tex;
      te_Rect dest;
      te_Rect src;
      TEA_VALUE angle;
      te_Point origin;
      te_RenderFlip flip;
    } texture;
  };

  te_Color color;
};

struct te_StackCommand {
  // TEA_STACK_COMMAND_ type;
  union {
    te_Canvas *canvas;
    te_Transform *transform;
    te_Shader *shader;
  };
};

struct te_Command {
  TEA_COMMAND_ type;
  union {
    te_DrawCommand draw;
    te_StackCommand stack;
  };
};


// struct te_Texture {
//   SDL_Texture *tex;
//   int width, height;
// };

struct te_Config {
  unsigned char title[100];
  int width, height;

  int flags;
  int window_flags;
  int render_flags;
};

// Config
TE_API int tea_config_init(te_Config *conf, const char *title, int width, int height);

// Core
TE_API Tea* tea_context();
TE_API Tea* tea_init(struct te_Config *c);
TE_API void tea_terminate(Tea *tea);

TE_API void tea_push(Tea *tea, te_Command cmd);
TE_API te_Command* tea_pop(Tea *tea);
TE_API te_Command* tea_top(Tea *tea);
TE_API void tea_repeat(Tea *tea, int index);

// TE_API te_Command tea_command_draw(Tea *tea, TEA_DRAW_COMMAND_ type);
// TE_API te_Command tea_command_stack(Tea *tea, TEA_STACK_COMMAND_ type);

TE_API int tea_should_close(Tea *tea);

TE_API void tea_begin_render(Tea *tea);
TE_API void tea_end_render(Tea *tea);

TE_API void tea_draw_color(Tea *tea, te_Color color);
TE_API void tea_draw_mode(Tea *tea, TEA_DRAW_MODE mode);

TE_API void tea_draw_point(Tea *tea, te_Point p);
TE_API void tea_draw_line(Tea *tea, te_Point p0, te_Point p1);

TE_API void tea_draw_rect(Tea *tea, TEA_VALUE x, TEA_VALUE y, TEA_VALUE w, TEA_VALUE h);
TE_API void tea_draw_circle(Tea *tea, te_Point p, TEA_VALUE radius);
TE_API void tea_draw_triangle(Tea *tea, te_Point p0, te_Point p1, te_Point p2);

TE_API void tea_draw_texture(Tea *tea, te_Texture tex, te_Rect *r, te_Point p);
TE_API void tea_draw_texture_scale(Tea *tea, te_Texture tex, te_Rect *r, te_Point p, te_Point scale);
TE_API void tea_draw_texture_ex(Tea *tea, te_Texture tex, te_Rect *r, te_Point p, TEA_VALUE angle, te_Point scale, te_Point origin);

TE_API void tea_draw_text(Tea *tea, te_Font *font, const char *text, te_Point pos);

// Window

TE_API te_Window* tea_window_create(const char *title, int width, int height, int flags);
TE_API void tea_window_destroy(te_Window *window);

TE_API int tea_window_should_close(te_Window *window);

// Render

TE_API te_Render* tea_render_create(te_Window *window, TEA_RENDER_FLAGS flags);
TE_API void tea_render_destroy(te_Render *render);

TE_API void tea_render_draw_color(Tea *t, te_Color color);
TE_API void tea_render_clear(Tea *t, te_Color color);
TE_API void tea_render_swap(Tea *t);

#define RECT_ARGS Tea *tea, te_Rect rect
#define CIRC_ARGS Tea *tea, te_Point p, TEA_VALUE radius
#define TRIANG_ARGS Tea *tea, te_Point p0, te_Point p1, te_Point p2

TE_API void tea_render_point(Tea *tea, te_Point p);
TE_API void tea_render_line(Tea *tea, te_Point p0, te_Point p1);
TE_API void tea_render_rect_fill(RECT_ARGS);
TE_API void tea_render_rect_line(RECT_ARGS);
TE_API void tea_render_circle_fill(CIRC_ARGS);
TE_API void tea_render_circle_line(CIRC_ARGS);
TE_API void tea_render_triangle_fill(TRIANG_ARGS);
TE_API void tea_render_triangle_line(TRIANG_ARGS);
TE_API void tea_render_texture(Tea *tea, te_Texture tex, te_Rect *dest, te_Rect *src);
TE_API void tea_render_texture_ex(Tea *tea, te_Texture tex, te_Rect *dest, te_Rect *src, TEA_VALUE angle, te_Point origin, te_RenderFlip flip);
TE_API void tea_render_text(Tea *tea, te_Font *font, const char *c, te_Point p);

// Texture

TE_API te_Texture tea_texture(Tea *tea, int w, int h, unsigned int format);
TE_API te_Texture tea_texture_load(Tea *tea, const char *filename);
// TE_API int tea_texture_init(Tea *tea, te_Texture *t, int w, int h, unsigned int format);

TE_API int tea_texture_width(Tea *tea, te_Texture tex);
TE_API int tea_texture_height(Tea *tea, te_Texture tex);
TE_API void tea_texture_size(Tea *tea, te_Texture tex, te_Point *size);

TE_API void tea_texture_destroy(Tea *tea, te_Texture *tex);

// Font

TE_API te_Font* tea_font(Tea *tea, const void *data, size_t buf_size, int font_size);
TE_API te_Font* tea_font_load(Tea *tea, const char *filename, int font_size);
TE_API int tea_font_init(Tea *tea, te_Font *font, const void *data, size_t buf_size, int font_size);

TE_API void tea_font_destroy(Tea *tea, te_Font *font);

TE_API void tea_font_get_rect(Tea *tea, te_Font* font, const int c, TEA_VALUE *x, TEA_VALUE *y, te_Point *out_pos, te_Rect *r, TEA_VALUE width);
TE_API void tea_font_char_rect(Tea *tea, te_Font* font, const unsigned int c, te_Rect *r);

TE_API int tea_font_get_text_width(Tea *tea, te_Font *font, const char *text, int len);
TE_API int tea_font_get_text_height(Tea *tea, te_Font *font, const char *text, int len);

/* Debug */

TE_API void tea_error(const char *msg);

#endif /* TEA_H */