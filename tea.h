#ifndef _TEA_H
#define _TEA_H

#define TEAPI extern

#ifndef TEA_MALLOC
    #define TEA_MALLOC malloc
#endif

#ifndef TEA_REALLOC
    #define TEA_REALLOC realloc
#endif

#ifndef TEA_FREE
    #define TEA_FREE free
#endif

#define TEA_TRUE  1
#define TEA_FALSE 0

#define TEA_ERR -1
#define TEA_OK   0

#define TEA_LOG(...)\
    tea_log(__LINE__, __PRETTY_FUNCTION__, __FILE__, __VA_ARGS__)

#define TEA_ASSERT(expr, ...)\
    if (!(expr)) {\
        fprintf(stderr, "Assertion failed at '%s':%d in %s: ", __PRETTY_FUNCTION__, __LINE__, __FILE__);\
        tea_abort(__VA_ARGS__);\
    }

#define TEA_PI 3.14159265
#define TEA_DEG2RAD(a) ((a) * TEA_PI / 180.0)
#define TEA_RAD2DEG(a) ((a) * 180.0 / TEA_PI)
#define TEA_MAX(a, b) ((a) > (b) ? (a) : (b))
#define TEA_MIN(a, b) ((a) < (b) ? (a) : (b))

#define TEA_RGB(r, g, b) ((te_color_t){(r), (g), (b), 255})
#define TEA_RGBA(r, g, b, a) ((te_color_t){(r), (g), (b), (a)})

#define TEA_BUFFER_POOL_SIZE 32

enum {
    TEA_TEXTURE_STATIC = 0,
    TEA_TEXTURE_STREAM,
    TEA_TEXTURE_TARGET
};

enum {
    TEA_ALPHA = 0,
    TEA_RED,
    TEA_RG,
    TEA_RGB,
    TEA_RGBA
};

enum {
    TEA_PERSPECTIVE = 0,
    TEA_MODELVIEW
};

enum {
    TEA_LINE = 0,
    TEA_FILL,
    TEA_FILL_QUAD,
};

enum {
    TEA_VERTEX_BUFFER = 0,
    TEA_INDEX_BUFFER
};

typedef unsigned char te_bool;
typedef char i8;
typedef unsigned char u8;
typedef short i16;
typedef unsigned short u16;
typedef int i32;
typedef unsigned int u32;
typedef long int i64;
typedef unsigned long int u64;
typedef float f32;
typedef double f64;

typedef struct Tea Tea;
typedef struct {
    struct {
        u16 glsl_ver;
        u8 min, mag;
        u8 is_es;
    } gl;
} te_config_t;

typedef f32 vec2[2];
typedef f32 vec3[3];
typedef f32 vec4[4];
typedef f32 mat4[16];

typedef u32 te_buffer_t;
typedef u32 te_texture_t;
typedef u32 te_program_t;
typedef u32 te_framebuffer_t;

typedef struct { f32 x, y, w, h; } te_rect_t;

#if defined(__cplusplus)
extern "C" {
#endif

TEAPI int tea_init(te_config_t *config);
TEAPI void tea_quit(void);

TEAPI void tea_begin(void);
TEAPI void tea_end(void);

TEAPI void tea_clear(f32 r, f32 g, f32 b, f32 a);

TEAPI void tea_viewport(f32 x, f32 y, f32 width, f32 height);
TEAPI void tea_scissor(f32 x, f32 y, f32 width, f32 height);

TEAPI void tea_draw(i32 mode);
TEAPI void tea_draw_interval(i32 mode, i32 start, i32 count);

TEAPI void tea_setup_buffer(te_buffer_t buffer);

/*=================================*
 *             Buffer              *
 *=================================*/

TEAPI te_buffer_t tea_buffer(i32 target, u32 size);
TEAPI void tea_buffer_free(te_buffer_t buffer);

TEAPI void tea_bind_buffer(te_buffer_t buffer);
TEAPI void tea_unbind_buffer(te_buffer_t buffer);

TEAPI void tea_buffer_flush(te_buffer_t buffer);
TEAPI void tea_buffer_grow(te_buffer_t buffer);

TEAPI void tea_buffer_seek(te_buffer_t buffer, u32 offset);
TEAPI u32 tea_buffer_tell(te_buffer_t buffer);

TEAPI void tea_buffer_send_vertices(te_buffer_t buffer, u32 n, f32 *vertices);

TEAPI void tea_buffer_vertex2f(te_buffer_t buffer, f32 x, f32 y);
TEAPI void tea_buffer_color3f(te_buffer_t buffer, f32 r, f32 g, f32 b);
TEAPI void tea_buffer_color4f(te_buffer_t buffer, f32 r, f32 g, f32 b, f32 a);
TEAPI void tea_buffer_texcoord(te_buffer_t buffer, f32 u, f32 v);

TEAPI void tea_buffer_point(te_buffer_t buffer, f32 x, f32 y);
TEAPI void tea_buffer_line(te_buffer_t buffer, vec2 p0, vec2 p1);
TEAPI void tea_buffer_line_rectangle(te_buffer_t buffer, vec2 pos, vec2 size);
TEAPI void tea_buffer_fill_rectangle(te_buffer_t buffer, vec2 pos, vec2 size);
TEAPI void tea_buffer_line_circle(te_buffer_t buffer, vec2 pos, f32 radius, u32 segments);
TEAPI void tea_buffer_fill_circle(te_buffer_t buffer, vec2 pos, f32 radius, u32 segments);
TEAPI void tea_buffer_line_triangle(te_buffer_t buffer, vec2 p0, vec2 p1, vec2 p2);
TEAPI void tea_buffer_fill_triangle(te_buffer_t buffer, vec2 p0, vec2 p1, vec2 p2);
TEAPI void tea_buffer_line_quad(te_buffer_t buffer, vec2 p0, vec2 p1, vec2 p2, vec2 p3);
TEAPI void tea_buffer_fill_quad(te_buffer_t buffer, vec2 p0, vec2 p1, vec2 p2, vec2 p3);
TEAPI void tea_buffer_line_polygon(te_buffer_t b, i32 n, vec2 *points);
// TEAPI void tea_buffer_fill_polygon(te_buffer_t *buffer, i32 n, vec2 *points);

#if 0
/*=================================*
 *             Batch               *
 *=================================*/

TEAPI te_batch_t tea_batch(u32 size);
TEAPI void tea_batch_free(te_batch_t batch);

TEAPI void tea_batch_flush(te_batch_t batch);
TEAPI void tea_batch_draw(te_batch_t batch, i32 mode);
TEAPI void tea_batch_draw_interval(te_batch_t batch, i32 mode, i32 start, i32 count);
TEAPI void tea_batch_grow(te_batch_t batch);

TEAPI void tea_batch_seek(te_batch_t batch, u32 offset);
TEAPI u32 tea_batch_tell(te_batch_t batch);
TEAPI void tea_batch_send_vertices(te_batch_t batch, u32 n, f32 *vertices);

TEAPI void tea_batch_vertex2f(te_batch_t batch, f32 x, f32 y);
TEAPI void tea_batch_color3f(te_batch_t batch, f32 r, f32 g, f32 b);
TEAPI void tea_batch_color4f(te_batch_t batch, f32 r, f32 g, f32 b, f32 a);
TEAPI void tea_batch_texcoord(te_batch_t batch, f32 u, f32 v);

TEAPI void tea_batch_point(te_batch_t batch, f32 x, f32 y);
TEAPI void tea_batch_line(te_batch_t batch, vec2 p0, vec2 p1);
TEAPI void tea_batch_line_rectangle(te_batch_t batch, vec2 pos, vec2 size);
TEAPI void tea_batch_fill_rectangle(te_batch_t batch, vec2 pos, vec2 size);
TEAPI void tea_batch_line_circle(te_batch_t batch, vec2 pos, f32 radius, u32 segments);
TEAPI void tea_batch_fill_circle(te_batch_t batch, vec2 pos, f32 radius, u32 segments);
TEAPI void tea_batch_line_triangle(te_batch_t batch, vec2 p0, vec2 p1, vec2 p2);
TEAPI void tea_batch_fill_triangle(te_batch_t batch, vec2 p0, vec2 p1, vec2 p2);
TEAPI void tea_batch_line_quad(te_batch_t batch, vec2 p0, vec2 p1, vec2 p2, vec2 p3);
TEAPI void tea_batch_fill_quad(te_batch_t batch, vec2 p0, vec2 p1, vec2 p2, vec2 p3);
TEAPI void tea_batch_line_polygon(te_batch_t b, i32 n, vec2 *points);
// TEAPI void tea_batch_fill_polygon(te_batch_t *batch, i32 n, vec2 *points);
#endif
/*=================================*
 *             Texture             *
 *=================================*/

TEAPI te_texture_t tea_texture(u8 format, i32 width, i32 height, const void *data, u8 type);
TEAPI void tea_texture_free(te_texture_t tex);

TEAPI void tea_bind_texture(te_texture_t tex);

TEAPI void tea_texture_get_size(te_texture_t tex, vec2 out);

TEAPI void tea_texture_set_filter(te_texture_t tex, i8 filter_min, i8 filter_mag);
TEAPI void tea_texture_set_wrap(te_texture_t tex, i8 wrap_s, i8 wrap_t);
TEAPI void tea_texture_get_filter(te_texture_t tex, vec2 out);
TEAPI void tea_texture_get_wrap(te_texture_t tex, vec2 out);

/*=================================*
 *           Framebuffer           *
 *=================================*/

TEAPI te_framebuffer_t tea_framebuffer(te_texture_t tex);
TEAPI void tea_framebuffer_free(te_framebuffer_t fbo);

TEAPI void tea_bind_framebuffer(te_framebuffer_t fbo);

/*=================================*
 *             Matrix              *
 *=================================*/

TEAPI void tea_matrix_mode(u8 mode);
TEAPI const f32* tea_get_matrix(u8 mode);
TEAPI void tea_load_identity(void);
TEAPI void tea_push_matrix(void);
TEAPI void tea_push_matrix(void);
TEAPI void tea_pop_matrix(void);

TEAPI void tea_load_matrix(const mat4 matrix);
TEAPI void tea_load_transpose_matrix(mat4 matrix);
TEAPI void tea_mult_matrix(mat4 matrix);
TEAPI void tea_mult_transpose_matrix(mat4 matrix);
TEAPI void tea_ortho(f32 left, f32 right, f32 bottom, f32 top, f32 near, f32 far);
TEAPI void tea_frustum(f32 left, f32 right, f32 bottom, f32 top, f32 near, f32 far);
TEAPI void tea_perspective(f32 fovy, f32 aspect, f32 near, f32 far);

TEAPI void tea_translatef(f32 x, f32 y, f32 z);
TEAPI void tea_scalef(f32 x, f32 y, f32 z);
TEAPI void tea_rotatef(f32 angle, f32 x, f32 y, f32 z);
TEAPI void tea_rotatef_z(f32 angle);

/*=================================*
 *             Program             *
 *=================================*/

TEAPI te_program_t tea_program(const i8 *vert, const i8 *frag);
TEAPI te_program_t tea_simple_program(const i8 *position, const i8 *pixel);
TEAPI te_program_t tea_program_from_gl(i32 count, u32 *programs);
TEAPI void tea_program_free(te_program_t program);

TEAPI void tea_use_program(te_program_t program);

TEAPI i32 tea_program_uniform_location(te_program_t program, const i8 *name);

#define TEA_UNIFORM_X(X, T)\
TEAPI void tea_program_set_uniform1##X(i32 location, T value);\
TEAPI void tea_program_set_uniform2##X(i32 location, T v0, T v1);\
TEAPI void tea_program_set_uniform3##X(i32 location, T v0, T v1, T v2);\
TEAPI void tea_program_set_uniform4##X(i32 location, T v0, T v1, T v2, T v3);\
TEAPI void tea_program_set_uniform1##X##v(i32 location, i32 val, const T *v);\
TEAPI void tea_program_set_uniform2##X##v(i32 location, i32 val, const T *v);\
TEAPI void tea_program_set_uniform3##X##v(i32 location, i32 val, const T *v);\
TEAPI void tea_program_set_uniform4##X##v(i32 location, i32 val, const T *v)

TEA_UNIFORM_X(f, f32);
TEA_UNIFORM_X(i, i32);

TEAPI void tea_program_set_uniform_matrix2fv(i32 location, i32 count, te_bool transpose, const mat4 m);
TEAPI void tea_program_set_uniform_matrix3fv(i32 location, i32 count, te_bool transpose, const mat4 m);
TEAPI void tea_program_set_uniform_matrix4fv(i32 location, i32 count, te_bool transpose, const mat4 m);

/*=================================*
 *              Debug              *
 *=================================*/

TEAPI void tea_log(i32 line, const i8* func, const i8* file, const i8 *fmt, ...);
TEAPI void tea_abort(const i8 *fmt, ...);

#if defined(__cplusplus)
}
#endif

#endif /* _TEA_H */