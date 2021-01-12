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

typedef struct te_Texture te_Texture;
typedef struct te_Canvas te_Canvas;
typedef struct te_Transform te_Transform;
typedef struct te_Shader te_Shader;

typedef TEA_VALUE te_Vec2[2];
typedef TEA_VALUE te_Vec3[3];
typedef TEA_VALUE te_Vec4[4];
typedef TEA_VALUE te_Matrix[4][4];
typedef struct { TEA_VALUE x, y; } te_Point;
typedef struct { TEA_VALUE x, y, w, h; } te_Rect;
typedef struct { unsigned char r, g, b, a; } te_Color;
typedef enum { TEA_FLIP_NONE = 0, TEA_FLIP_H = (1 << 0), TEA_FLIP_V = (1 << 1) } te_RenderFlip;


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

typedef void(*DrawPointFn)(Tea*, te_Point);
typedef void(*DrawLineFn)(Tea*, te_Point, te_Point);
typedef void(*DrawRectFn)(Tea*, te_Rect);
typedef void(*DrawCircleFn)(Tea*, te_Point, TEA_VALUE);
typedef void(*DrawTriangleFn)(Tea*, te_Point, te_Point, te_Point);
typedef void(*DrawTextureFn)(Tea*, te_Rect, te_Rect);
typedef void(*DrawTextureExFn)(Tea*, te_Rect, te_Rect, TEA_VALUE, te_Point, te_RenderFlip flip);

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
      te_Texture *tex;
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

struct te_Config {
  unsigned char title[100];
  int width, height;

  int flags;
  int window_flags;
  int render_flags;
};

struct Tea {
  te_Render *render;
  te_Window *window;
  te_Event event;

  te_Color _color;
  TEA_DRAW_MODE _mode;

  struct {
    DrawPointFn point;
    DrawLineFn line;
    DrawRectFn rect[DRAW_MODE_COUNT];
    DrawCircleFn circle[DRAW_MODE_COUNT];
    DrawTriangleFn triangle[DRAW_MODE_COUNT];
    DrawTextureFn texture;
    DrawTextureExFn texture_ex;
  } draw;

  struct {
    unsigned char cp; // canvas pointer
    unsigned char tp; // transform pointer
    unsigned char sp; // shader pointer

    te_Canvas *canvas[STACK_MAX];
    te_Transform *transform[STACK_MAX];
    te_Shader *shader[STACK_MAX];
  } stack;

  unsigned int cp; // command pointer
  te_Command commands[COMMAND_MAX];
};


// Config
TE_API int tea_config_init(te_Config *conf, const char *title, int width, int height);

// Core
TE_API Tea* tea_context();
TE_API Tea* tea_init(struct te_Config *c);
TE_API void tea_terminate(Tea *ctx);

TE_API void tea_push(Tea *ctx, te_Command cmd);
TE_API te_Command* tea_pop(Tea *ctx);
TE_API te_Command* tea_top(Tea *ctx);
TE_API void tea_repeat(Tea *ctx, int index);

// TE_API te_Command tea_command_draw(Tea *ctx, TEA_DRAW_COMMAND_ type);
// TE_API te_Command tea_command_stack(Tea *ctx, TEA_STACK_COMMAND_ type);

TE_API int tea_should_close(Tea *ctx);

TE_API void tea_begin_render(Tea *ctx);
TE_API void tea_end_render(Tea *ctx);

TE_API void tea_draw_color(Tea *ctx, te_Color color);
TE_API void tea_draw_mode(Tea *ctx, TEA_DRAW_MODE mode);

TE_API void tea_draw_point(Tea *ctx, te_Point p);
TE_API void tea_draw_line(Tea *ctx, te_Point p0, te_Point p1);

TE_API void tea_draw_rect(Tea *ctx, TEA_VALUE x, TEA_VALUE y, TEA_VALUE w, TEA_VALUE h);
TE_API void tea_draw_circle(Tea *ctx, te_Point p, TEA_VALUE radius);
TE_API void tea_draw_triangle(Tea *ctx, te_Point p0, te_Point p1, te_Point p2);

TE_API void tea_draw_texture(Tea *ctx, te_Texture *tex, te_Rect *r, te_Point p);
TE_API void tea_draw_texture_ex(Tea *ctx, te_Texture *tex, te_Rect *r, te_Point p, TEA_VALUE angle, te_Point scale, te_Point origin);

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

#define RECT_ARGS Tea *t, te_Rect rect
#define CIRC_ARGS Tea *t, te_Point p, TEA_VALUE radius
#define TRIANG_ARGS Tea *t, te_Point p0, te_Point p1, te_Point p2

TE_API void tea_render_point(Tea *t, te_Point p);
TE_API void tea_render_line(Tea *t, te_Point p0, te_Point p1);
TE_API void tea_render_rect_fill(RECT_ARGS);
TE_API void tea_render_rect_line(RECT_ARGS);
TE_API void tea_render_circle_fill(CIRC_ARGS);
TE_API void tea_render_circle_line(CIRC_ARGS);
TE_API void tea_render_triangle_fill(TRIANG_ARGS);
TE_API void tea_render_triangle_line(TRIANG_ARGS);
TE_API void tea_render_texture(Tea *render);
TE_API void tea_render_texture_ex(Tea *render);

/* Debug */

TE_API void tea_error(const char *msg);

#endif /* TEA_H */