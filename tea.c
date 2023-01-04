#include "tea.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#if defined(__EMSCRIPTEN__)
    #include <emscripten.h>
#endif

#if defined(_WIN32)
    #include <windows.h>
    #ifndef WINDOWS_LEAN_AND_MEAN
        #define WINDOWS_LEAN_AND_MEAN 1
    #endif
    static HMODULE s_glsym;
#else
    #include <dlfcn.h>
    static void *s_glsym;
    #ifndef RTLD_LAZY
        #define RTLD_LAZY 0x00001
        #endif
    #ifndef RTLD_GLOBAL
        #define RTLD_GLOBAL 0x00100
    #endif
#endif

#define GET_GL(name)\
(GL()->procs[TEA_GL_##name])

#define CALL_GL(name)\
((GL##name##Proc)GET_GL(name))

#define TEA() (&tea_ctx)
#define GL() (&TEA()->gl)
#define STATE() (&TEA()->state)

#define MATRIX(T) (&TEA()->matrix[(T)])
#define MATRIX_TOP(T) (&TEA()->matrix[(T)].pool[TEA()->matrix[(T)].top])
#define BUFFER(i) (&TEA()->buffer.pool[(i)])

#define TEA_GL_VERSION 0x1F02
#define TEA_GL_SHADING_LANGUAGE_VERSION 0x8B8C

/* data types */
#define TEA_GL_BYTE           0x1400
#define TEA_GL_UNSIGNED_BYTE  0x1401
#define TEA_GL_SHORT          0x1402
#define TEA_GL_UNSIGNED_SHORT 0x1403
#define TEA_GL_INT            0x1404
#define TEA_GL_UNSIGNED_INT   0x1405
#define TEA_GL_FLOAT          0x1406
#define TEA_GL_2_BYTES        0x1407
#define TEA_GL_3_BYTES        0x1408
#define TEA_GL_4_BYTES        0x1409
#define TEA_GL_DOUBLE         0x140A

/* Primitives */
#define TEA_GL_POINTS         0x0000
#define TEA_GL_LINES          0x0001
#define TEA_GL_LINE_LOOP      0x0002
#define TEA_GL_LINE_STRIP     0x0003
#define TEA_GL_TRIANGLES      0x0004
#define TEA_GL_TRIANGLE_STRIP 0x0005
#define TEA_GL_TRIANGLE_FAN   0x0006
#define TEA_GL_QUADS          0x0007
#define TEA_GL_QUAD_STRIP     0x0008
#define TEA_GL_POLYGON        0x0009

/* Clear buffer bits */
#define TEA_GL_DEPTH_BUFFER_BIT   0x00000100
#define TEA_GL_ACCUM_BUFFER_BIT   0x00000200
#define TEA_GL_STENCIL_BUFFER_BIT 0x00000400
#define TEA_GL_COLOR_BUFFER_BIT   0x00004000

#define TEA_GL_RGB         0x1907
#define TEA_GL_RGBA        0x1908

/* bgra */
#define TEA_GL_BGR  0x80E0
#define TEA_GL_BGRA 0x80E1

#define TEA_GL_ARRAY_BUFFER 0x8892
#define TEA_GL_ELEMENT_ARRAY_BUFFER 0x8893

#define TEA_GL_STREAM_DRAW  0x88E0
#define TEA_GL_STREAM_READ  0x88E1
#define TEA_GL_STREAM_COPY  0x88E2
#define TEA_GL_STATIC_DRAW  0x88E4
#define TEA_GL_STATIC_READ  0x88E5
#define TEA_GL_STATIC_COPY  0x88E6
#define TEA_GL_DYNAMIC_DRAW 0x88E8
#define TEA_GL_DYNAMIC_READ 0x88E9
#define TEA_GL_DYNAMIC_COPY 0x88EA

#define TEA_GL_TEXTURE_2D 0x0DE1
#define TEA_GL_TEXTURE_MIN_FILTER 0x2800
#define TEA_GL_TEXTURE_MAG_FILTER 0x2801
#define TEA_GL_TEXTURE_WRAP_S 0x2802
#define TEA_GL_TEXTURE_WRAP_T 0x2803

#define TEA_GL_NEAREST 0x2600
#define TEA_GL_REPEAT 0x2901
#define TEA_GL_CLAMP 0x2900

#define TEA_GL_CLAMP_TO_EDGE 0x812F /* 1.2 */
#define TEA_CLAMP_TO_BORDER  0x812D /* 1.3 */

enum {
    TEA_PROC_OVERRIDE = (1 << 0),
    TEA_PROC_RET_ON_DUP = (1 << 1),
};

enum {
    /* Miscellaenous */
    TEA_GL_ClearColor = 0,
    TEA_GL_ClearDepth,
    TEA_GL_Clear,
    TEA_GL_BlendFunc,
    TEA_GL_LogicOp,
    TEA_GL_CullFace,
    TEA_GL_FrontFace,
    TEA_GL_PolygonMode,
    TEA_GL_Scissor,
    TEA_GL_DrawBuffer,
    TEA_GL_ReadBuffer,
    TEA_GL_Enable,
    TEA_GL_Disable,

    TEA_GL_EnableClientState,
    TEA_GL_DisableClientState,

    TEA_GL_GetBooleanv,
    TEA_GL_GetDoublev,
    TEA_GL_GetFloatv,
    TEA_GL_GetIntegerv,
    TEA_GL_GetError,
    TEA_GL_GetString,

    TEA_GL_GetStringi, /* 3.0 */

    /* Depth */
    TEA_GL_DepthFunc,
    TEA_GL_DepthMask,
    TEA_GL_DepthRange,

    /* Transformation */
    TEA_GL_Viewport,
    TEA_GL_MatrixMode,
    TEA_GL_PushMatrix,
    TEA_GL_PopMatrix,
    TEA_GL_LoadIdentity,

    TEA_GL_LoadMatrixd,
    TEA_GL_MultMatrixd,
    TEA_GL_Rotated,
    TEA_GL_Scaled,
    TEA_GL_Translated,

    TEA_GL_LoadMatrixf,
    TEA_GL_MultMatrixf,
    TEA_GL_Rotatef,
    TEA_GL_Scalef,
    TEA_GL_Translatef,

    TEA_GL_Ortho,
    TEA_GL_Frustum,

    TEA_GL_Orthof, /* GL ES */
    TEA_GL_Frustumf, /* GL ES */

    TEA_GL_LoadTransposeMatrixd,
    TEA_GL_MultTransposeMatrixd,
    TEA_GL_LoadTransposeMatrixf,
    TEA_GL_MultTransposeMatrixf,

    /* Vertex arrays */
    TEA_GL_VertexPointer,
    TEA_GL_NormalPointer,
    TEA_GL_ColorPointer,
    TEA_GL_TexCoordPointer,
    TEA_GL_IndexPointer,
    TEA_GL_EdgeFlagPointer,

    TEA_GL_DrawArrays,
    TEA_GL_DrawElements,

    /* Texture mapping */
    TEA_GL_TexParameterf,
    TEA_GL_TexParameteri,
    TEA_GL_TexParameterfv,
    TEA_GL_TexParameteriv,

    TEA_GL_GetTexParameteriv,
    TEA_GL_GetTexParameterfv,

    TEA_GL_GenTextures,
    TEA_GL_DeleteTextures,
    TEA_GL_BindTexture,
    TEA_GL_IsTexture,

    TEA_GL_TexImage1D,
    TEA_GL_TexImage2D,
    TEA_GL_TexSubImage1D,
    TEA_GL_TexSubImage2D,
    TEA_GL_CopyTexImage1D,
    TEA_GL_CopyTexImage2D,
    TEA_GL_CopyTexSubImage1D,
    TEA_GL_CopyTexSubImage2D,

    TEA_GL_TexImage3D,
    TEA_GL_TexSubImage3D,
    TEA_GL_CopyTexSubImage3D,

    /* GL_ARB_vertex_buffer_object */
    TEA_GL_BindBuffer,
    TEA_GL_DeleteBuffers,
    TEA_GL_GenBuffers,
    TEA_GL_BufferData,
    TEA_GL_BufferSubData,
    TEA_GL_MapBuffer,
    TEA_GL_UnmapBuffer,

    /* GL_ARB_vertex_program */
    TEA_GL_VertexAttribPointer,
    TEA_GL_EnableVertexAttribArray,
    TEA_GL_DisableVertexAttribArray,

    /* GL_ARB_vertex_shader */
    TEA_GL_BindAttribLocation,
    TEA_GL_GetAttribLocation,
    TEA_GL_GetActiveAttrib,

    /* GL_EXT_framebuffer_object */
    TEA_GL_GenFramebuffers,
    TEA_GL_DeleteFramebuffers,
    TEA_GL_BindFramebuffer,
    TEA_GL_CheckFramebufferStatus,
    TEA_GL_FramebufferTexture2D,
    TEA_GL_FramebufferRenderbuffer,
    TEA_GL_GenerateMipmap,
    TEA_GL_BlitFramebuffer,
    TEA_GL_IsFramebuffer,

    TEA_GL_GenRenderbuffers,
    TEA_GL_DeleteRenderbuffers,
    TEA_GL_BindRenderbuffer,
    TEA_GL_RenderbufferStorage,
    TEA_GL_RenderbufferStorageMultisample,
    TEA_GL_IsRenderbuffer,

    /* GL_ARB_shader_objects */
    TEA_GL_CreateProgram,
    TEA_GL_DeleteProgram,
    TEA_GL_UseProgram,
    TEA_GL_CreateShader,
    TEA_GL_DeleteShader,
    TEA_GL_ShaderSource,
    TEA_GL_CompileShader,
    TEA_GL_GetShaderiv,
    TEA_GL_GetShaderInfoLog,
    TEA_GL_AttachShader,
    TEA_GL_DetachShader,
    TEA_GL_LinkProgram,
    TEA_GL_GetProgramiv,
    TEA_GL_GetProgramInfoLog,
    TEA_GL_GetUniformLocation,
    TEA_GL_GetActiveUniform,
    TEA_GL_Uniform1f,
    TEA_GL_Uniform2f,
    TEA_GL_Uniform3f,
    TEA_GL_Uniform4f,
    TEA_GL_Uniform1i,
    TEA_GL_Uniform2i,
    TEA_GL_Uniform3i,
    TEA_GL_Uniform4i,
    TEA_GL_Uniform1fv,
    TEA_GL_Uniform2fv,
    TEA_GL_Uniform3fv,
    TEA_GL_Uniform4fv,
    TEA_GL_Uniform1iv,
    TEA_GL_Uniform2iv,
    TEA_GL_Uniform3iv,
    TEA_GL_Uniform4iv,
    TEA_GL_UniformMatrix2fv,
    TEA_GL_UniformMatrix3fv,
    TEA_GL_UniformMatrix4fv,

    /* GL_ARB_vertex_array_object */
    TEA_GL_BindVertexArray,
    TEA_GL_DeleteVertexArrays,
    TEA_GL_GenVertexArrays,

    TEA_GL_PROC_COUNT,
};

enum {
    TEA_HAS_VBO = (1 << 0),
    TEA_HAS_VAO = (1 << 1),
    TEA_HAS_SHADER = (1 << 2)
};

/* Miscellaneous */
typedef void(*GLClearColorProc)(f32, f32, f32, f32);
typedef void(*GLClearProc)(u32);
typedef void(*GLBlendFuncProc)(u32, u32);
typedef void(*GLLogicOpProc)(u32);
typedef void(*GLCullFaceProc)(u32);
typedef void(*GLFrontFaceProc)(u32);
typedef void(*GLPolygonModeProc)(u32, u32);
typedef void(*GLScissorProc)(i32, i32, i32, i32);
typedef void(*GLDrawBufferProc)(u32);
typedef void(*GLReadBufferProc)(u32);
typedef void(*GLEnableProc)(u32);
typedef void(*GLDisableProc)(u32);

typedef void(*GLEnableClientStateProc)(u32); /* 1.1 */
typedef void(*GLDisableClientStateProc)(u32); /* 1.1 */

typedef void(*GLGetBooleanvProc)(u32, te_bool*);
typedef void(*GLGetDoublevProc)(u32, f64*);
typedef void(*GLGetf32vProc)(u32, f32*);
typedef void(*GLGetIntegervProc)(u32, i32*);
typedef void(*GLGetErrorProc)(void);
typedef const u8*(*GLGetStringProc)(u32);

typedef const u8*(*GLGetStringiProc)(u32, u32); /* 3.0 */

/* Depth buffer */
typedef void(*GLClearDepthProc)(f32);
typedef void(*GLDepthFuncProc)(u32);
typedef void(*GLDepthMaskProc)(te_bool);
typedef void(*GLDepthRangeProc)(f64, f64);

/* Transformation */
typedef void(*GLViewportProc)(i32, i32, i32, i32);
typedef void(*GLMatrixModeProc)(u32);
typedef void(*GLPushMatrixProc)(void);
typedef void(*GLPopMatrixProc)(void);
typedef void(*GLLoadIdentityProc)(void);
typedef void(*GLLoadMatrixfProc)(const f32*);
typedef void(*GLLoadMatrixdProc)(const f64*);
typedef void(*GLMultMatrixfProc)(const f32*);
typedef void(*GLMultMatrixdProc)(const f64*);
typedef void(*GLOrthoProc)(f64, f64, f64, f64, f64, f64);
typedef void(*GLFrustumProc)(f64, f64, f64, f64, f64, f64);
typedef void(*GLTranslatefProc)(f32, f32, f32);
typedef void(*GLRotatefProc)(f32, f32, f32, f32);
typedef void(*GLScalefProc)(f32, f32, f32);
typedef void(*GLTranslatedProc)(f64, f64, f64);
typedef void(*GLRotatedProc)(f64, f64, f64, f64);
typedef void(*GLScaledProc)(f64, f64, f64);

typedef void(*GLLoadTransposeMatrixdProc)(const f64[16]); /* 1.3 */
typedef void(*GLLoadTransposeMatrixfProc)(const f32[16]); /* 1.3 */
typedef void(*GLMultTransposeMatrixdProc)(const f64[16]); /* 1.3 */
typedef void(*GLMultTransposeMatrixfProc)(const f32[16]); /* 1.3 */

/* Vertex Arrays */
typedef void(*GLVertexPointerProc)(i32, u32, i32, const void*);
typedef void(*GLColorPointerProc)(i32, u32, i32, const void*);
typedef void(*GLTexCoordPointerProc)(i32, u32, i32, const void*);
typedef void(*GLNormalPointerProc)(u32, i32, const void*);
typedef void(*GLIndexPointerProc)(u32, i32, const void*);
typedef void(*GLEdgeFlagPointerProc)(i32, i32, const void*);

typedef void(*GLDrawArraysProc)(u32, i32, i32);
typedef void(*GLDrawElementsProc)(u32, i32, u32, const void*);

/* Texture mapping */
typedef void(*GLTexParameterfProc)(u32, u32, f32);
typedef void(*GLTexParameteriProc)(u32, u32, i32);
typedef void(*GLTexParameterfvProc)(u32, u32, const f32*);
typedef void(*GLTexParameterivProc)(u32, u32, const i32*);

typedef void(*GLGetTexParameterfProc)(u32, u32, f32*);
typedef void(*GLGetTexParameteriProc)(u32, u32, i32*);
typedef void(*GLGetTexParameterfvProc)(u32, u32, f32*);
typedef void(*GLGetTexParameterivProc)(u32, u32, i32*);

typedef void(*GLGenTexturesProc)(u32, u32*);
typedef void(*GLDeleteTexturesProc)(u32, u32*);
typedef void(*GLBindTextureProc)(u32, u32);
typedef te_bool(*GLIsTextureProc)(u32);
typedef void(*GLTexImage1DProc)(u32, i32, i32, i32, i32, u32, u32, const void*);
typedef void(*GLTexImage2DProc)(u32, i32, i32, i32, i32, i32, u32, u32, const void*);
typedef void(*GLTexImage3DProc)(u32, i32, i32, i32, i32, i32, i32, u32, u32, const void*);
typedef void(*GLTexSubImage1DProc)(u32, i32, i32, i32, u32, u32, const void*);
typedef void(*GLTexSubImage2DProc)(u32, i32, i32, i32, i32, i32, u32, u32, const void*);
typedef void(*GLTexSubImage3DProc)(u32, i32, i32, i32, i32, i32, i32, u32, u32, const void*);
typedef void(*GLCopyTexImage1DProc)(u32, i32, u32, i32, i32, i32, i32);
typedef void(*GLCopyTexImage2DProc)(u32, i32, u32, i32, i32, i32, i32, i32);
typedef void(*GLCopyTexSubImage1DProc)(u32, i32, i32, i32, i32, i32);
typedef void(*GLCopyTexSubImage2DProc)(u32, i32, i32, i32, i32, i32, i32, i32);
typedef void(*GLCopyTexSubImage3DProc)(u32, i32, i32, i32, i32, i32, i32, i32, i32, i32);

/* GL_ARB_vertex_buffer_object */
typedef void(*GLBindBufferProc)(u32, u32);
typedef void(*GLBufferDataProc)(u32, u32, const void*, u32);
typedef void(*GLBufferSubDataProc)(u32, u32, u32, const void*);
typedef void(*GLGenBuffersProc)(u32, u32*);
typedef void(*GLDeleteBuffersProc)(u32, u32*);
typedef void*(*GLMapBufferProc)(u32, u32);
typedef u32(*GLUnmapBufferProc)(u32);

/* GL_ARB_vertex_array_object */
typedef void(*GLGenVertexArraysProc)(u32, u32*);
typedef void(*GLBindVertexArrayProc)(u32);
typedef void(*GLDeleteVertexArraysProc)(u32, u32*);

/* GL_ARB_vertex_array_program */
typedef void(*GLVertexAttribPointerProc)(u32, i32, u32, i32, i32, const void*);
typedef void(*GLEnableVertexAttribArrayProc)(u32);
typedef void(*GLDisableVertexAttribArrayProc)(u32);

/* GL_EXT_framebuffer_object */
typedef te_bool(*GLIsRenderbufferProc)(u32);
typedef void(*GLBindRenderbufferProc)(u32, u32);
typedef void(*GLDeleteRenderbuffersProc)(u32, u32*);
typedef void(*GLGenRenderbuffersProc)(u32, u32*);
typedef void(*GLRenderbufferStorageProc)(u32, u32, u32, u32);
typedef void(*GLGetRenderbufferParameterivProc)(u32, u32, i32*);

typedef te_bool(*GLIsFramebufferProc)(u32);
typedef void(*GLBindFramebufferProc)(u32, u32);
typedef void(*GLDeleteFramebuffersProc)(u32, u32*);
typedef void(*GLGenFramebuffersProc)(u32, u32*);
typedef void(*GLFramebufferRenderbufferProc)(u32, u32, u32, u32);
typedef void(*GLFramebufferTexture1DProc)(u32, u32, u32, u32, i32);
typedef void(*GLFramebufferTexture2DProc)(u32, u32, u32, u32, i32);
typedef void(*GLFramebufferTexture3DProc)(u32, u32, u32, u32, i32, i32);
typedef void(*GLFramebufferTextureLayerProc)(u32, u32, u32, i32, i32);
typedef u32(*GLCheckFramebufferStatusProc)(u32);
typedef void(*GLGetFramebufferAttachmentParameterivProc)(u32, u32, u32, i32*);
typedef void(*GLBlitFramebufferProc)(i32, i32, i32, i32, i32, i32, i32, i32, u32);
typedef void(*GLGenerateMipmapProc)(u32);

/* GL_ARB_shader_objects */
typedef void(*GLDeleteShaderProc)(u32);
typedef u32(*GLCreateShaderProc)(u32);
typedef void(*GLShaderSourceProc)(u32, i32, const i8**, const i32*);
typedef void(*GLCompileShaderProc)(u32);
typedef u32(*GLGetShaderivProc)(u32, u32, i32*);
typedef u32(*GLGetShaderInfoLogProc)(u32, i32, i32*, i8*);

typedef u32(*GLCreateProgramProc)(void);
typedef void(*GLDeleteProgramProc)(u32);
typedef void(*GLAttachShaderProc)(u32, u32);
typedef void(*GLDetachShaderProc)(u32, u32);
typedef void(*GLLinkProgramProc)(u32);
typedef void(*GLUseProgramProc)(u32);

typedef void(*GLGetProgramivProc)(u32, u32, i32*);
typedef void(*GLGetProgramInfoLogProc)(u32, i32, i32*, i8*);
typedef void(*GLGetActiveUniformProc)(u32, u32, i32, i32*, i32*, i32*, i8*);
typedef i32(*GLGetUniformLocationProc)(u32, const i8*);

#define GL_UNIFORM_X(X, T)\
typedef void(*GLUniform1##X##Proc)(i32, T);\
typedef void(*GLUniform2##X##Proc)(i32, T, T);\
typedef void(*GLUniform3##X##Proc)(i32, T, T, T);\
typedef void(*GLUniform4##X##Proc)(i32, T, T, T, T);\
typedef void(*GLUniform1##X##vProc)(i32, i32 value, const T*);\
typedef void(*GLUniform2##X##vProc)(i32, i32 value, const T*);\
typedef void(*GLUniform3##X##vProc)(i32, i32 value, const T*);\
typedef void(*GLUniform4##X##vProc)(i32, i32 value, const T*)

GL_UNIFORM_X(f, f32);
GL_UNIFORM_X(i, i32);

typedef void(*GLUniformMatrix2fvProc)(i32, i32, te_bool, const f32*);
typedef void(*GLUniformMatrix3fvProc)(i32, i32, te_bool, const f32*);
typedef void(*GLUniformMatrix4fvProc)(i32, i32, te_bool, const f32*);

/* GL_ARB_vertex_shader */
typedef i32(*GLGetAttribLocationProc)(u32 prog, const i8* name);
typedef void(*GLGetActiveAttribProc)(u32 prog, u32 index, i32 bufSize, i32* length, i32* size, u32* type, i8* name);
typedef void(*GLBindAttribLocationProc)(u32 prog, u32 index, const i8* name);


enum {
    COMMAND_NONE = 0,

    CLEAR_COMMAND,

    SET_DRAWMODE_COMMAND,
    SET_SHADER_COMMAND,
    SET_FBO_COMMAND,
    SET_TEXTURE_COMMAND,
    SET_CLIP_COMMAND,
    SET_SCISSOR_COMMAND,
    SET_VIEWPORT_COMMAND,
    SET_BLENDMODE_COMMAND,
    SET_MATRIX_COMMAND,

    VBO_DRAW_COMMAND,
    IBO_DRAW_COMMAND,

    MAX_COMMANDS
};

typedef struct te_proc_s te_proc_t;
struct te_proc_s {
    u8 tag;
    const i8 *names[3];
};

static struct {
    i8 mag, min;
} s_gl_max_ver;

struct te_gl_s {
    struct {
        u8 major, minor;
        u16 glsl;
        te_bool es;
    } version;
    u32 extensions;
    void* procs[TEA_GL_PROC_COUNT];
};

typedef struct {
    u32 target;
    u32 handle;
    u32 index, size;
    void *data;
    u8 used;
} buffer_t;

struct attrib_t {
	i8 size;
	i8 type;
	i8 location;
	u8 stride;
};

struct te_vertex_format_t {
	u16 stride;
	i8 top;
	struct attrib_t attribs[16];
};

struct te_state_s {
    te_program_t program;
    te_texture_t texture;
    te_framebuffer_t fbo;
    te_rect_t clip, scissor;
    te_buffer_t vbo, ibo;
	te_vertex_format_t *format;
    u32 vao;
    // f32 *projection, *modelview;
    f32* matrix;
    mat4 transform;
    u8 draw_mode;
};

#define MATRIX_STACK_SIZE 255

typedef struct {
    u8 index;
    mat4 stack[MATRIX_STACK_SIZE];
} matrix_stack_t;

#define POOL(type, size)\
struct {\
    u32 top;\
    type pool[(size)];\
}

struct Tea {
    u32 vao;
    buffer_t *vertex_buffer, *index_buffer;
    mat4 world, modelview;

    struct te_gl_s gl;
    struct te_state_s state;

    i32 matrix_mode;
    POOL(mat4, MATRIX_STACK_SIZE) matrix[2];
    POOL(buffer_t, TEA_BUFFER_POOL_SIZE) buffer;
    // POOL(batch_t, TEA_BUFFER_POOL_SIZE) batch;
    // POOL(command_t, TEA_COMMAND_POOL_SIZE) command;
    // POOL(texture_t, TEA_TEXTURE_POOL_SIZE) texture;
    // POOL(shader_t, TEA_SHADER_POOL_SIZE) shader;

    u32 current_command;
};

typedef struct {
    f32 x, y;
    f32 r, g, b, a;
    f32 u, v;
} vertex_t;

static Tea tea_ctx;

static const char *s_130_vert_header =
"#version 130\n"
"uniform mat4 u_World;\n"
"uniform mat4 u_ModelView;\n"
"in vec2 a_Position;\n"
"in vec4 a_Color;\n"
"in vec2 a_TexCoord;\n"
"out vec4 v_Color;\n"
"out vec2 v_TexCoord;\n";

static const char *s_130_frag_header =
"#version 130\n"
"in vec4 v_Color;\n"
"in vec2 v_TexCoord;\n"
"uniform sampler2D u_Texture;\n"
"out vec4 o_FragColor;\n";

static const char *s_120_vert_header =
"#version 120\n"
"uniform mat4 u_World;\n"
"uniform mat4 u_ModelView;\n"
"attribute vec2 a_Position;\n"
"attribute vec4 a_Color;\n"
"attribute vec2 a_TexCoord;\n"
"varying vec4 v_Color;\n"
"varying vec2 v_TexCoord;\n";

static const char *s_120_frag_header =
"#version 120\n"
"varying vec4 v_Color;\n"
"varying vec2 v_TexCoord;\n"
"uniform sampler2D u_Texture;\n"
"#define o_FragColor gl_FragColor\n"
"#define texture texture2D\n";

static const char *s_default_vert_function =
"vec4 position(mat4 model_view, mat4 world, vec2 pos) {\n"
"  return world * model_view * vec4(pos.x, pos.y, 0.0, 1.0);\n"
"}\n";

static const char *s_default_frag_function =
"vec4 pixel(vec4 color, vec2 tex_coord, sampler2D tex) {\n"
"  return color * texture(tex, tex_coord);\n"
"}\n";
// "  return color;\n"

static const char *s_vert_main =
"void main() {\n"
"  v_Color = a_Color;\n"
"  v_TexCoord = a_TexCoord;\n"
"  gl_Position = position(u_ModelView, u_World, a_Position);\n"
"}\n";

static const char *s_frag_main =
"void main() {\n"
"  o_FragColor = pixel(v_Color, v_TexCoord, u_Texture);\n"
"}\n";

static void* s_get_proc(const i8 *name);
static te_bool s_load_gl(void);
static void s_setup_gl(void);
static void s_close_gl(void);

static int buffer_init(buffer_t *buffer, i32 target, u32 size);
static int buffer_deinit(buffer_t *buffer);
static int buffer_destroy(buffer_t *buffer);

i32 gl_modes[] = {
    [TEA_LINE] = TEA_GL_LINES,
    [TEA_FILL] = TEA_GL_TRIANGLES,
    [TEA_FILL_QUAD] = TEA_GL_TRIANGLES
};

#define DEFAULT_BUFFER_SIZE 1000

int tea_init(te_config_t *config) {
    memset(&tea_ctx, 0, sizeof(Tea));
    if (s_load_gl()) {
        s_setup_gl();
        s_close_gl();
    } else
        TEA_ASSERT(0, "failed to initialize OpenGL");

    CALL_GL(GenVertexArrays)(1, &STATE()->vao);

    return 1;
}

void tea_quit(void) {
    for (i32 i = 0; i < TEA()->buffer.top; i++) {
        buffer_t *b = BUFFER(i);
        buffer_destroy(b);
    }
}

void tea_clear(f32 r, f32 g, f32 b, f32 a) {
    CALL_GL(ClearColor)(r, g, b, a);
    CALL_GL(Clear)(TEA_GL_COLOR_BUFFER_BIT);
}

void tea_draw(i32 mode) {
    i32 gl_mode = gl_modes[mode];
    if (STATE()->vbo == 0) return;
    buffer_t *buf = BUFFER(STATE()->vbo - 1);
    CALL_GL(BindVertexArray)(STATE()->vao);
    CALL_GL(DrawArrays)(gl_mode, 0, buf->index / sizeof(vertex_t));
    CALL_GL(BindVertexArray)(0);
}

void tea_draw_interval(i32 mode, i32 start, i32 count) {
    i32 gl_mode = gl_modes[mode];
    CALL_GL(BindVertexArray)(STATE()->vao);
    CALL_GL(DrawArrays)(gl_mode, start, count);
    CALL_GL(BindVertexArray)(0);
}

static int gl_type[] = {
	[TEA_ATTRIB_BOOL] = TEA_GL_BYTE,
	[TEA_ATTRIB_FLOAT] = TEA_GL_FLOAT,
	[TEA_ATTRIB_FLOAT2] = TEA_GL_FLOAT,
	[TEA_ATTRIB_FLOAT3] = TEA_GL_FLOAT,
	[TEA_ATTRIB_FLOAT4] = TEA_GL_FLOAT
};

static void s_enable_format(te_vertex_format_t* format) {
	u64 offset = 0;
	for (i32 i = 0; i < format->top; i++) {
		struct attrib_t *attr = &format->attribs[i];
		CALL_GL(EnableVertexAttribArray)(attr->location);
		CALL_GL(VertexAttribPointer)(attr->location, attr->size, gl_type[attr->type], TEA_FALSE, format->stride, (void*)offset);
		offset += attr->stride;
	}
}

static void s_disable_format(te_vertex_format_t* format) {
	if (!format) return;
	for (i32 i = 0; i < format->top; i++) {
		struct attrib_t *attr = &format->attribs[i];
		CALL_GL(DisableVertexAttribArray)(attr->location);
	}
}

void tea_setup_buffer(te_vertex_format_t* format, te_buffer_t b) {
	TEA_ASSERT(format != NULL, "Invalid vertex format");
    TEA_ASSERT(b > 0 && b <= TEA_BUFFER_POOL_SIZE, "Invalid buffer");
    CALL_GL(BindVertexArray)(STATE()->vao);
    tea_bind_buffer(b);
    STATE()->vbo = b;

	s_disable_format(STATE()->format);
	STATE()->format = format;
	s_enable_format(STATE()->format);
#if 0
    CALL_GL(EnableVertexAttribArray)(0);
    CALL_GL(EnableVertexAttribArray)(1);
    CALL_GL(EnableVertexAttribArray)(2);

    CALL_GL(VertexAttribPointer)(0, 2, TEA_GL_FLOAT, TEA_FALSE, sizeof(vertex_t), (void*)0);
    CALL_GL(VertexAttribPointer)(1, 4, TEA_GL_FLOAT, TEA_FALSE, sizeof(vertex_t), (void*)(2 * sizeof(f32)));
    CALL_GL(VertexAttribPointer)(2, 2, TEA_GL_FLOAT, TEA_FALSE, sizeof(vertex_t), (void*)(6 * sizeof(f32)));
#endif
    CALL_GL(BindVertexArray)(0);
    tea_unbind_buffer(b);
}

/*=================================*
 *          Vertex Format          *
 *=================================*/

te_vertex_format_t* tea_vertex_format(void) {
	te_vertex_format_t *format;
	format = (te_vertex_format_t*)TEA_MALLOC(sizeof(*format));
	memset(format, 0, sizeof(*format));
	return format;
}

void tea_free_vertex_format(te_vertex_format_t* format) {
	if (!format) return;
	TEA_FREE(format);
}

static int attrib_stride[] = {
	[TEA_ATTRIB_BOOL] = 1,
	[TEA_ATTRIB_FLOAT] = 4,
	[TEA_ATTRIB_FLOAT2] = 8,
	[TEA_ATTRIB_FLOAT3] = 12,
	[TEA_ATTRIB_FLOAT4] = 16
};

void tea_vertex_format_add(te_vertex_format_t* format, int attrib) {
	TEA_ASSERT(format != NULL, "Invalid vertex format");
	TEA_ASSERT(attrib >= TEA_ATTRIB_BOOL && attrib < TEA_ATTRIB_MAX, "Invalid attrib");
	struct attrib_t *attr = &format->attribs[format->top];
	attr->size = attrib > 0 ? attrib : 1; 
	attr->type = attrib;
	attr->location = format->top;
	attr->stride = attrib_stride[attrib];
	
	format->stride += attr->stride;
	format->top++;
}


/*=================================*
 *             Buffer              *
 *=================================*/

i32 gl_target[] = {
    TEA_GL_ARRAY_BUFFER,
    TEA_GL_ELEMENT_ARRAY_BUFFER
};

int buffer_init(buffer_t *b, i32 target, u32 size) {
    if (!size) return 0;
    CALL_GL(GenBuffers)(1, &b->handle);
    CALL_GL(BindBuffer)(gl_target[target], b->handle);
    if (b->data) {
        if (b->size > size) size = b->size;
        b->data = TEA_REALLOC(b->data, size);
    } else {
        b->data = TEA_MALLOC(size);
    }
    b->size = size;
    b->used = TEA_TRUE;
    b->index = 0;
    CALL_GL(BufferData)(gl_target[target], b->size, b->data, TEA_GL_DYNAMIC_DRAW);
    CALL_GL(BindBuffer)(gl_target[target], 0);

    return 1;
}

int buffer_deinit(buffer_t *b) {
    if (!b->size || !b->data) return 0;
    b->used = TEA_FALSE;
    return 1;
}

int buffer_destroy(buffer_t *b) {
    if (!b) return 0;
    buffer_deinit(b);
    if (b->data) TEA_FREE(b->data);

    return 1;
}

buffer_t *get_free_buffer(u32 *index) {
    for (u32 i = 0; i < TEA()->buffer.top; i++)
        if (!BUFFER(i)->used) return BUFFER(i);
    TEA_ASSERT(TEA()->buffer.top <= TEA_BUFFER_POOL_SIZE, "buffer limit reached (%d)", TEA_BUFFER_POOL_SIZE);
    buffer_t *buffer = &TEA()->buffer.pool[TEA()->buffer.top++];
    *index = TEA()->buffer.top;
    return buffer;
}

te_buffer_t tea_buffer(i32 target, u32 size) {
    TEA_ASSERT(size > 0, "Invalid buffer size");
    u32 index = 0;
    buffer_t *buf = get_free_buffer(&index);
    buffer_init(buf, target, size);
    return index;
}

void tea_buffer_free(te_buffer_t b) {
    TEA_ASSERT(b > 0 && b <= TEA_BUFFER_POOL_SIZE, "Invalid buffer");
    buffer_t *buf = BUFFER(b-1);
    buf->used = TEA_FALSE;
}

void tea_bind_buffer(te_buffer_t b) {
    TEA_ASSERT(b > 0 && b <= TEA_BUFFER_POOL_SIZE, "Invalid buffer");
    buffer_t *buf = BUFFER(b-1);
    CALL_GL(BindBuffer)(gl_target[buf->target], buf->handle);
}

void tea_unbind_buffer(te_buffer_t b) {
    TEA_ASSERT(b > 0 && b <= TEA_BUFFER_POOL_SIZE, "Invalid buffer");
    buffer_t *buf = BUFFER(b-1);
    CALL_GL(BindBuffer)(gl_target[buf->target], 0);
}

void tea_buffer_flush(te_buffer_t b) {
    TEA_ASSERT(b > 0 && b <= TEA_BUFFER_POOL_SIZE, "Invalid buffer");
    buffer_t *buf = BUFFER(b-1);
    i32 target = gl_target[buf->target];
    CALL_GL(BindBuffer)(target, buf->handle);
    /* vertex_t *v = buf->data;
    for (i32 i = 0; i < 8; i++) {
        TEA_LOG("%f %f %f %f %f %f %f %f", v->x, v->y, v->r, v->g, v->b, v->a, v->u, v->v);
        v++;
    } */
    CALL_GL(BufferSubData)(target, 0, buf->index, buf->data);
    CALL_GL(BindBuffer)(target, 0);
}

void tea_buffer_grow(te_buffer_t b) {
    TEA_ASSERT(b > 0 && b <= TEA_BUFFER_POOL_SIZE, "Invalid buffer");
    buffer_t *buf = BUFFER(b-1);
    buf->size *= 2;
    buf->data = TEA_REALLOC(buf->data, buf->size);
    i32 target = gl_target[buf->target];
    CALL_GL(BindBuffer)(target, buf->handle);
    CALL_GL(BufferData)(target, buf->size, buf->data, TEA_GL_DYNAMIC_DRAW);
    CALL_GL(BindBuffer)(target, 0);
}

void tea_buffer_seek(te_buffer_t b, u32 offset) {
    TEA_ASSERT(b > 0 && b <= TEA_BUFFER_POOL_SIZE, "Invalid buffer");
    buffer_t *buf = BUFFER(b-1);
    if (offset > buf->size) buf->index = buf->size;
    else buf->index = offset;
}

u32 tea_buffer_tell(te_buffer_t b) {
    TEA_ASSERT(b > 0 && b <= TEA_BUFFER_POOL_SIZE, "Invalid buffer");
    buffer_t *buf = BUFFER(b-1);
    return buf->index;
}

void tea_buffer_send_vertices(te_buffer_t b, u32 n, f32 *verts) {
    TEA_ASSERT(b > 0 && b <= TEA_BUFFER_POOL_SIZE, "Invalid buffer");
    buffer_t *buf = BUFFER(b-1);
    i32 size = n * STATE()->format->stride;
    while (buf->index + size > buf->size) tea_buffer_grow(b);
	u8* cdata = ((u8*)buf->data) + buf->index;
    memcpy(cdata, verts, size);
    buf->index += size;
	u8* data = ((u8*)buf->data) + buf->index;
    memcpy(data, cdata, STATE()->format->stride);
}

void tea_buffer_vertex2f(te_buffer_t b, f32 x, f32 y) {
    TEA_ASSERT(b > 0 && b <= TEA_BUFFER_POOL_SIZE, "Invalid buffer");
    buffer_t *buf = BUFFER(b-1);
    vertex_t *cdata = (vertex_t*)(((u8*)buf->data) + buf->index);
    vertex_t *data = cdata;
    data->x = x;
    data->y = y;
    data++;
    memcpy(data, cdata, sizeof(*data));
    buf->index += sizeof(vertex_t);
}

void tea_buffer_color3f(te_buffer_t v, f32 r, f32 g, f32 b) {
    TEA_ASSERT(v > 0 && v <= TEA_BUFFER_POOL_SIZE, "Invalid buffer");
    buffer_t *buf = BUFFER(v-1);
    vertex_t *cdata = (vertex_t*)(((u8*)buf->data) + buf->index);
    cdata->r = r;
    cdata->g = g;
    cdata->b = b;
}

void tea_buffer_color4f(te_buffer_t v, f32 r, f32 g, f32 b, f32 a) {
    TEA_ASSERT(b > 0 && b <= TEA_BUFFER_POOL_SIZE, "Invalid buffer");
    buffer_t *buf = BUFFER(v-1);
    vertex_t *cdata = (vertex_t*)(((u8*)buf->data) + buf->index);
    cdata->r = r;
    cdata->g = g;
    cdata->b = b;
    cdata->a = a;
}

void tea_buffer_texcoord(te_buffer_t b, f32 u, f32 v) {
    TEA_ASSERT(b > 0 && b <= TEA_BUFFER_POOL_SIZE, "Invalid buffer");
    buffer_t *buf = BUFFER(b-1);
    vertex_t *cdata = (vertex_t*)(((u8*)buf->data) + buf->index);
    cdata->u = u;
    cdata->v = v;
}

void tea_buffer_point(te_buffer_t buffer, f32 x, f32 y) { tea_buffer_vertex2f(buffer, x, y); }
void tea_buffer_line(te_buffer_t b, vec2 p0, vec2 p1) {
    tea_buffer_vertex2f(b, p0[0], p0[1]);
    tea_buffer_vertex2f(b, p1[0], p1[1]);
}
void tea_buffer_line_rectangle(te_buffer_t b, vec2 pos, vec2 size) {
    TEA_ASSERT(b > 0 && b <= TEA_BUFFER_POOL_SIZE, "Invalid buffer");
    buffer_t *buf = BUFFER(b-1);
    vertex_t *cdata = (vertex_t*)(((u8*)buf->data) + buf->index);
    f32 vertices[] = {
        pos[0],         pos[1],         cdata->r, cdata->g, cdata->b, cdata->a, cdata->u, cdata->v,
        pos[0]+size[0], pos[1],         cdata->r, cdata->g, cdata->b, cdata->a, cdata->u, cdata->v,

        pos[0]+size[0], pos[1],         cdata->r, cdata->g, cdata->b, cdata->a, cdata->u, cdata->v,
        pos[0]+size[0], pos[1]+size[1], cdata->r, cdata->g, cdata->b, cdata->a, cdata->u, cdata->v,
        
        pos[0]+size[0], pos[1]+size[1], cdata->r, cdata->g, cdata->b, cdata->a, cdata->u, cdata->v,
        pos[0],         pos[1]+size[1], cdata->r, cdata->g, cdata->b, cdata->a, cdata->u, cdata->v,
        
        pos[0],         pos[1]+size[1], cdata->r, cdata->g, cdata->b, cdata->a, cdata->u, cdata->v,
        pos[0],         pos[1],         cdata->r, cdata->g, cdata->b, cdata->a, cdata->u, cdata->v,
    };
    tea_buffer_send_vertices(b, 8, vertices);
}
void tea_buffer_fill_rectangle(te_buffer_t b, vec2 pos, vec2 size) {
    TEA_ASSERT(b > 0 && b <= TEA_BUFFER_POOL_SIZE, "Invalid buffer");
    buffer_t *buf = BUFFER(b-1);
    vertex_t *cdata = (vertex_t*)(((u8*)buf->data) + buf->index);
    f32 vertices[] = {
        pos[0],         pos[1],         cdata->r, cdata->g, cdata->b, cdata->a, cdata->u, cdata->v,
        pos[0]+size[0], pos[1],         cdata->r, cdata->g, cdata->b, cdata->a, cdata->u, cdata->v,
        pos[0]+size[0], pos[1]+size[1], cdata->r, cdata->g, cdata->b, cdata->a, cdata->u, cdata->v,

        pos[0],         pos[1],         cdata->r, cdata->g, cdata->b, cdata->a, cdata->u, cdata->v,
        pos[0],         pos[1]+size[1], cdata->r, cdata->g, cdata->b, cdata->a, cdata->u, cdata->v,
        pos[0]+size[0], pos[1]+size[1], cdata->r, cdata->g, cdata->b, cdata->a, cdata->u, cdata->v,
    };
    tea_buffer_send_vertices(b, 6, vertices);
}
void tea_buffer_line_triangle(te_buffer_t b, vec2 p0, vec2 p1, vec2 p2) {
    TEA_ASSERT(b > 0 && b <= TEA_BUFFER_POOL_SIZE, "Invalid buffer");
    buffer_t *buf = BUFFER(b-1);
    vertex_t *cdata = (vertex_t*)(((u8*)buf->data) + buf->index);
    f32 vertices[] = {
        p0[0], p0[1], cdata->r, cdata->g, cdata->b, cdata->a, cdata->u, cdata->v,
        p1[0], p1[1], cdata->r, cdata->g, cdata->b, cdata->a, cdata->u, cdata->v,
        
        p1[0], p1[1], cdata->r, cdata->g, cdata->b, cdata->a, cdata->u, cdata->v,
        p2[0], p2[1], cdata->r, cdata->g, cdata->b, cdata->a, cdata->u, cdata->v,

        p0[0], p0[1], cdata->r, cdata->g, cdata->b, cdata->a, cdata->u, cdata->v,
        p2[0], p2[1], cdata->r, cdata->g, cdata->b, cdata->a, cdata->u, cdata->v,
    };
    tea_buffer_send_vertices(b, 6, vertices);
}

void tea_buffer_fill_triangle(te_buffer_t b, vec2 p0, vec2 p1, vec2 p2) {
    TEA_ASSERT(b > 0 && b <= TEA_BUFFER_POOL_SIZE, "Invalid buffer");
    buffer_t *buf = BUFFER(b-1);
    vertex_t *cdata = (vertex_t*)(((u8*)buf->data) + buf->index);
    // TEA_LOG("color: %f %f %f %f", cdata->r, cdata->g, cdata->b, cdata->a);
    // TEA_LOG("texcoord: %f %f", cdata->u, cdata->v);
    f32 vertices[] = {
        p0[0], p0[1], cdata->r, cdata->g, cdata->b, cdata->a, cdata->u, cdata->v,
        p1[0], p1[1], cdata->r, cdata->g, cdata->b, cdata->a, cdata->u, cdata->v,
        p2[0], p2[1], cdata->r, cdata->g, cdata->b, cdata->a, cdata->u, cdata->v,
    };
    tea_buffer_send_vertices(b, 3, vertices);
}
static const double pi2 = 2.0 * TEA_PI;
void tea_buffer_line_circle(te_buffer_t b, vec2 pos, f32 radius, u32 s) {
    i32 sides = s < 4 ? 4 : s;
    i32 i;
    f32 x, y;
    x = pos[0];
    y = pos[1];
    for (i = 0; i < sides; i++) {
        f32 tetha = (i * pi2) / sides;
        tea_buffer_vertex2f(b, x + (cosf(tetha) * radius), y + (sinf(tetha) * radius));
        tetha = ((i+1) * pi2) / sides;
        tea_buffer_vertex2f(b, x + (cosf(tetha) * radius), y + (sinf(tetha) * radius));
    }
}

void tea_buffer_fill_circle(te_buffer_t b, vec2 pos, f32 radius, u32 s) {
    i32 sides = s < 4 ? 4 : s;
    i32 i;
    for (i = 0; i < sides; i++) {
        f32 tetha = (i * pi2) / sides;
        f32 tetha2 = ((i+1) * pi2) / sides;
        vec2 p1, p2;
        p1[0] = pos[0] + (cosf(tetha) * radius);
        p1[1] = pos[1] + (sinf(tetha) * radius);
        p2[0] = pos[0] + (cosf(tetha2) * radius);
        p2[1] = pos[1] + (sinf(tetha2) * radius);
        tea_buffer_fill_triangle(b, pos, p1, p2);
    }
}
#if 0
buffer_t *create_buffer(u32 target, u32 size) {
    if (!size) return NULL;
    buffer_t *buffer = TEA_MALLOC(sizeof *buffer);
    TEA_ASSERT(buffer != NULL, "Cannot allocate memory for buffer");
    buffer->data = TEA_MALLOC(size);
    TEA_ASSERT(buffer->data != NULL, "Cannot allocate memory for buffer data");
    buffer->index = 0;
    buffer->target = target;
    buffer->size = 0;
    CALL_GL(GenBuffers)(1, &buffer->handle);
    resize_buffer(buffer, size);

    return buffer;
}

void destroy_buffer(buffer_t *buffer) {
    if (!buffer) return;
    if (buffer->data) TEA_FREE(buffer->data);
    if (buffer->handle) CALL_GL(DeleteBuffers)(1, &buffer->handle);
}

void buffer_set_target(buffer_t *b, u32 target) {
    b->target = target;
}

void resize_buffer(buffer_t *b, u32 size) {
    if (!size) return;
    if (b->size > size) return;

    b->size = size;
    b->data = TEA_REALLOC(b->data, b->size);
    CALL_GL(BindBuffer)(b->target, b->handle);
    CALL_GL(BufferData)(b->target, b->size, b->data, TEA_GL_DYNAMIC_DRAW);
    CALL_GL(BindBuffer)(b->target, 0);
}

void buffer_add_data(buffer_t *b, u32 size, const void *data) {
    if (!size || !data) return;
    if (b->index + size > b->size)
        resize_buffer(b, (b->index + size) * 2);
    u8 *bdata = b->data;
    memcpy(bdata + b->index, data, size);
    b->index += size;
}

void buffer_add_vertices(buffer_t *b, u32 n, const f32 *vertices) {
    u32 data_size = n * 9 * sizeof(f32);
    buffer_add_data(b, data_size, vertices);
}

void buffer_add_indices(buffer_t *b, u32 n, const u32 *indices) {
    u32 data_size = n * sizeof(u32);
    buffer_add_data(b, data_size, indices);
}

void buffer_add_batch(buffer_t *b, batch_t *batch) {
    buffer_add_data(b, batch->index, batch->data);
}

void flush_buffer(buffer_t *b) {
    CALL_GL(BufferSubData)(b->target, 0, b->index, b->data);
}

/*=================================*
 *             Batch               *
 *=================================*/

/* Internal */

int batch_init(batch_t *batch, u32 size) {
    if (!size) return 0;
    if (batch->data) {
        if (batch->count > size) size = batch->count;
        batch->data = TEA_REALLOC(batch->data, size);
    } else {
        batch->data = TEA_MALLOC(size);
    }
    batch->count = size;
    batch->used = TEA_TRUE;

    return 1;
}

int batch_deinit(batch_t *batch) {
    if (!batch->count || !batch->data) return 0;
    batch->used = TEA_FALSE;

    return 1;
}

int batch_destroy(batch_t *batch) {
    if (!batch) return 0;
    if (batch->data) TEA_FREE(batch->data);

    return 1;
}

int batch_add_vertices(batch_t *batch, u32 n, f32 *vertices) {
    u32 data_size = n * (9 * sizeof(f32));
    if (batch->index + data_size > batch->count) {
        batch->count = (batch->index + data_size) * 2;
        batch->data = TEA_REALLOC(batch->data, batch->count);
    }
    u8 *data = batch->data;
    memcpy(data + batch->index, vertices, data_size);

    return 1;
}

int batch_add_indices(batch_t *b, u32 n, u32 *indices) {
    u32 data_size = n * sizeof(u32);
    if (b->index + data_size > b->count) {
        b->count = (b->index + data_size) * 2;
        b->data = TEA_REALLOC(b->data, b->count);
    }
    u8 *data = b->data;
    memcpy(data + b->index, indices, data_size);

    return 1;
}

int batch_add_batch(batch_t *b, batch_t *other) {
    if (b->index + other->index > b->count) {
        b->count = (b->index + other->index) * 2;
        b->data = TEA_REALLOC(b->data, b->count);
    }
    u8 *data = b->data;
    memcpy(data + b->index, other->data, other->index);

    return 1;
}

batch_t *get_free_batch(u32 *index) {
    for (u32 i = 0; i < TEA()->batch.top; i++)
        if (!BATCH(i)->used) return BATCH(i);
    TEA_ASSERT(TEA()->batch.top <= TEA_BUFFER_POOL_SIZE, "Batch limit reached (%d)", TEA_BUFFER_POOL_SIZE);
    batch_t *batch = &TEA()->batch.pool[TEA()->batch.top++];
    *index = TEA()->batch.top;
    return batch;
}

/* Tea API */

te_batch_t tea_batch(u32 size) {
    TEA_ASSERT(size > 0, "Invalid batch size");
    u32 index;
    batch_t *batch = get_free_batch(&index);
    batch_init(batch, size);

    return index;
}

void tea_batch_free(te_batch_t b) {
    TEA_ASSERT(b > 0 && b <= TEA_BUFFER_POOL_SIZE, "Invalid batch");
    batch_t *batch = BATCH(b-1);

    batch->used = TEA_FALSE;
}

void tea_batch_fill_triangle(te_batch_t b, vec2 p0, vec2 p1, vec2 p2) {
    batch_t *batch = BATCH(b-1);
    
    f32 vertices[27] = {
        p0[0], p0[1], 1.f, 1.f, 1.f, 1.f, 0.f, 0.f,
        p1[0], p1[1], 1.f, 1.f, 1.f, 1.f, 0.f, 0.f,
        p2[0], p2[1], 1.f, 1.f, 1.f, 1.f, 0.f, 0.f,
    };
    batch_add_vertices(batch, 3, vertices);
}

void tea_batch_flush(te_batch_t b) {
    TEA_ASSERT(b > 0 && b <= TEA_BUFFER_POOL_SIZE, "Invalid batch");
    batch_t *batch = BATCH(b-1);
    CALL_GL(BufferSubData)(TEA_GL_ARRAY_BUFFER, 0, batch->index, batch->data);
}

void tea_batch_draw(te_batch_t b, i32 mode) {
    TEA_ASSERT(b > 0 && b <= TEA_BUFFER_POOL_SIZE, "Invalid batch");
    // draw_command(VBO_DRAW_COMMAND);
    batch_t *batch = BATCH(b-1);
    tea_batch_flush(b);
    CALL_GL(DrawArrays)(gl_mode[mode], 0, batch->index / sizeof(vertex_t));
    // batch_add_batch(&default_batch, batch);
    // draw_command(VBO_DRAW_COMMAND);
}
#endif
/*=================================*
 *           Framebuffer           *
 *=================================*/

#define TEA_GL_FRAMEBUFFER 0x8D40
#define TEA_GL_COLOR_ATTACHMENT0 0x8CE0
#define TEA_GL_FRAMEBUFFER_COMPLETE 0x8CD5

te_framebuffer_t tea_framebuffer(te_texture_t tex) {
    if (!tex) {
        fprintf(stderr, "Texture cannot be NULL\n");
        return 0;
    }
    te_framebuffer_t fbo = 0;
    CALL_GL(GenFramebuffers)(1, &fbo);
    CALL_GL(BindFramebuffer)(TEA_GL_FRAMEBUFFER, fbo);
    CALL_GL(FramebufferTexture2D)(TEA_GL_FRAMEBUFFER, TEA_GL_COLOR_ATTACHMENT0, TEA_GL_TEXTURE_2D, tex, 0);
    u32 status = CALL_GL(CheckFramebufferStatus)(TEA_GL_FRAMEBUFFER);
    if (status != TEA_GL_FRAMEBUFFER_COMPLETE) {
        fprintf(stderr, "Failed to create framebuffer\n");
        return 0;
    }
    CALL_GL(BindFramebuffer)(TEA_GL_FRAMEBUFFER, 0);
    return fbo;
}

void tea_framebuffer_free(te_framebuffer_t fbo) {
    if (!fbo) return;
    CALL_GL(DeleteFramebuffers)(1, &fbo);
}

void tea_bind_framebuffer(te_framebuffer_t fbo) {
    CALL_GL(BindFramebuffer)(TEA_GL_FRAMEBUFFER, fbo);
}

/*=================================*
 *             Texture             *
 *=================================*/

i32 tex_format[] = {
    [TEA_ALPHA] = TEA_GL_RGBA,
    [TEA_RED] = TEA_GL_RGB,
    [TEA_RG] = TEA_GL_RGB,
    [TEA_RGB] = TEA_GL_RGB,
    [TEA_RGBA] = TEA_GL_RGBA
};

te_texture_t tea_texture(u8 format, i32 width, i32 height, const void *data, u8 type) {
    te_texture_t tex;
    i32 form = tex_format[format];
    CALL_GL(GenTextures)(1, &tex);
    CALL_GL(BindTexture)(TEA_GL_TEXTURE_2D, tex);

    CALL_GL(TexImage2D)(TEA_GL_TEXTURE_2D, 0, form, width, height, 0, form, TEA_GL_UNSIGNED_BYTE, data);

    CALL_GL(TexParameteri)(TEA_GL_TEXTURE_2D, TEA_GL_TEXTURE_MIN_FILTER, TEA_GL_NEAREST);
    CALL_GL(TexParameteri)(TEA_GL_TEXTURE_2D, TEA_GL_TEXTURE_MAG_FILTER, TEA_GL_NEAREST);
    
    CALL_GL(TexParameteri)(TEA_GL_TEXTURE_2D, TEA_GL_TEXTURE_WRAP_S, TEA_GL_CLAMP_TO_EDGE);
    CALL_GL(TexParameteri)(TEA_GL_TEXTURE_2D, TEA_GL_TEXTURE_WRAP_T, TEA_GL_CLAMP_TO_EDGE);

    CALL_GL(BindTexture)(TEA_GL_TEXTURE_2D, 0);
    return tex;
}

void tea_texture_free(te_texture_t tex) {
    CALL_GL(DeleteTextures)(1, &tex);
}

void tea_bind_texture(te_texture_t tex) {
    CALL_GL(BindTexture)(TEA_GL_TEXTURE_2D, tex);
}

/*=================================*
 *             Program             *
 *=================================*/

#define TEA_GL_FRAGMENT_SHADER 0x8B30
#define TEA_GL_VERTEX_SHADER 0x8B31

#define TEA_GL_COMPILE_STATUS 0x8B81
#define TEA_GL_LINK_STATUS 0x8B82

const i8 *shader_type_names[] = {
    "Fragment",
    "Vertex"
};

static u32 compile_gl_shader(u32 type, const i8 *src) {
    u32 shader = CALL_GL(CreateShader)(type);
    CALL_GL(ShaderSource)(shader, 1, &src, NULL);
    CALL_GL(CompileShader)(shader);
    i32 status;
    CALL_GL(GetShaderiv)(shader, TEA_GL_COMPILE_STATUS, &status);
    if (status == 0) {
        i8 log[1024];
        CALL_GL(GetShaderInfoLog)(shader, 1024, NULL, log);
        TEA_ASSERT(0, "%s shader compile error: %s", shader_type_names[type - TEA_GL_FRAGMENT_SHADER], log);
    }
    return shader;
}

int s_program_init_from_gl(te_program_t *prog, u32 n, u32 *shaders) {
    TEA_ASSERT(n > 0, "Invalid shader count");
    TEA_ASSERT(shaders != NULL, "GL shaders cannot be NULL");
    *prog = CALL_GL(CreateProgram)();
    for (i32 i = 0; i < n; i++)
        CALL_GL(AttachShader)(*prog, shaders[i]);
    
    CALL_GL(LinkProgram)(*prog);
    i32 status;
    CALL_GL(GetProgramiv)(*prog, TEA_GL_LINK_STATUS, &status);
    if (status == 0) {
        i8 log[1024];
        CALL_GL(GetProgramInfoLog)(*prog, 1024, NULL, log);
        TEA_ASSERT(0, "program link error: %s", log);
    }
    // shader->world_uniform = CALL_GL(GetUniformLocation)(shader->handle, "u_World");
    // shader->modelview_uniform = CALL_GL(GetUniformLocation)(shader->handle, "u_ModelView");
    // shader->texture_uniform = CALL_GL(GetUniformLocation)(shader->handle, "u_Texture");
    return 1;
}

te_program_t tea_program(const i8 *vert, const i8 *frag) {
	te_program_t prog;
	u32 shaders[2];
	shaders[0] = compile_gl_shader(TEA_GL_VERTEX_SHADER, vert);
	shaders[1] = compile_gl_shader(TEA_GL_FRAGMENT_SHADER, frag);
	s_program_init_from_gl(&prog, 2, shaders);
    CALL_GL(DeleteShader)(shaders[0]);
    CALL_GL(DeleteShader)(shaders[1]);
	return prog;
}

te_program_t tea_simple_program(const i8 *vert, const i8 *frag) {
    // shader_t *shader = &TEA()->shader.pool[TEA()->shader.top++];
    // u32 index = TEA()->shader.top;
    // shader_init(shader, vert, frag);
    te_program_t prog;
    vert = vert ? vert : s_default_vert_function;
    frag = frag ? frag : s_default_frag_function;
    const i8* vert_shader_strs[] = { s_130_vert_header, vert, s_vert_main };
    const i8* frag_shader_strs[] = { s_130_frag_header, frag, s_frag_main };

    i32 vert_len, frag_len;
    vert_len = frag_len = 0;
    for (u8 i = 0; i < 3; i++) {
        vert_len += strlen((const i8*)vert_shader_strs[i]);
        frag_len += strlen((const i8*)frag_shader_strs[i]);
    }

    i8 vert_source[vert_len + 1];
    i8 frag_source[frag_len + 1];
    vert_source[0] = frag_source[0] = '\0';
    for (u8 i = 0; i < 3; i++) {
        strcat(vert_source, (const i8*)vert_shader_strs[i]);
        strcat(frag_source, (const i8*)frag_shader_strs[i]);
    }
    vert_source[vert_len] = frag_source[frag_len] = '\0';

    u32 shaders[2];
    shaders[0] = compile_gl_shader(TEA_GL_VERTEX_SHADER, vert_source);
    shaders[1] = compile_gl_shader(TEA_GL_FRAGMENT_SHADER, frag_source);
    s_program_init_from_gl(&prog, 2, shaders);
    CALL_GL(DeleteShader)(shaders[0]);
    CALL_GL(DeleteShader)(shaders[1]);
    return prog;
}

void tea_program_free(te_program_t program) {
    if (!program) return;
    CALL_GL(DeleteProgram)(program);
}

void tea_use_program(te_program_t s) {
    CALL_GL(UseProgram)(s);
}

i32 tea_program_uniform_location(te_program_t s, const i8 *name) {
    // shader_t *shader = SHADER(s - 1);
    return CALL_GL(GetUniformLocation)(s, name);
}

#define TEA_UNIFORM_X_IMPL(X, T)\
void tea_program_set_uniform1##X(i32 l, T v) { CALL_GL(Uniform1##X)(l, v); }\
void tea_program_set_uniform2##X(i32 l, T v0, T v1) { CALL_GL(Uniform2##X)(l, v0, v1); }\
void tea_program_set_uniform3##X(i32 l, T v0, T v1, T v2) { CALL_GL(Uniform3##X)(l, v0, v1, v2); }\
void tea_program_set_uniform4##X(i32 l, T v0, T v1, T v2, T v3) { CALL_GL(Uniform4##X)(l, v0, v1, v2, v3); }\
void tea_program_set_uniform1##X##v(i32 l, i32 val, const T *v) { CALL_GL(Uniform1##X##v)(l, val, v); }\
void tea_program_set_uniform2##X##v(i32 l, i32 val, const T *v) { CALL_GL(Uniform2##X##v)(l, val, v); }\
void tea_program_set_uniform3##X##v(i32 l, i32 val, const T *v) { CALL_GL(Uniform3##X##v)(l, val, v); }\
void tea_program_set_uniform4##X##v(i32 l, i32 val, const T *v) { CALL_GL(Uniform4##X##v)(l, val, v); }

TEA_UNIFORM_X_IMPL(f, f32)
TEA_UNIFORM_X_IMPL(i, i32)

void tea_program_set_uniform_matrix2fv(i32 location, i32 count, te_bool transpose, const mat4 m) {
    CALL_GL(UniformMatrix2fv)(location, count, transpose, m);
}

void tea_program_set_uniform_matrix3fv(i32 location, i32 count, te_bool transpose, const mat4 m) {
    CALL_GL(UniformMatrix3fv)(location, count, transpose, m);
}

void tea_program_set_uniform_matrix4fv(i32 location, i32 count, te_bool transpose, const mat4 m) {
    CALL_GL(UniformMatrix4fv)(location, count, transpose, m);
}

/*=================================*
 *              Debug              *
 *=================================*/
#include <stdarg.h>

void tea_log(i32 line, const i8 *func, const i8 *file, const i8 *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "%s:%d - %s(...): ", file, line, func);
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
    va_end(args);
}

void tea_abort(const i8 *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
    va_end(args);
    exit(EXIT_FAILURE);
}

/*=================================*
 *         Transformation          *
 *=================================*/

void tea_viewport(f32 x, f32 y, f32 w, f32 h) { CALL_GL(Viewport)(x, y, w, h); }
void tea_matrix_mode(u8 mode) { CALL_GL(MatrixMode)(mode); }
const f32* tea_get_matrix(u8 mode) {
    // mode -= TEA_MODELVIEW;
    // struct matrix_stack_s *stack = &TEA()->matrix.stack[mode];
    // return stack->m[stack->top];
    return (f32*)MATRIX_TOP(mode);
}
void tea_push_matrix(void) { CALL_GL(PushMatrix)(); }
void tea_pop_matrix(void) { CALL_GL(PopMatrix)(); }
void tea_load_identity(void) { CALL_GL(LoadIdentity)(); }

void tea_load_matrixf(const f32 *m) { CALL_GL(LoadMatrixf)(m); }
void tea_load_transpose_matrixf(const f32 *m) { CALL_GL(LoadTransposeMatrixf)(m); }
void tea_mult_matrixf(const f32 *m) { CALL_GL(MultMatrixf)(m); }
void tea_mult_transpose_matrixf(const f32 *m) { CALL_GL(MultTransposeMatrixf)(m); }
void tea_translatef(f32 x, f32 y, f32 z) { CALL_GL(Translatef)(x, y, z); }
void tea_rotatef(f32 angle, f32 x, f32 y, f32 z) { CALL_GL(Rotatef)(angle, x, y, z); }
void tea_scalef(f32 x, f32 y, f32 z) { CALL_GL(Scalef)(x, y, z); }
void tea_orthof(f32 left, f32 right, f32 bottom, f32 top, f32 near, f32 far) {
    CALL_GL(Ortho)(left, right, bottom, top, near, far);
}
void tea_frustumf(f32 left, f32 right, f32 bottom, f32 top, f32 near, f32 far) {
    CALL_GL(Frustum)(left, right, bottom, top, near, far);
}
void tea_ortho(f32 left, f32 right, f32 bottom, f32 top, f32 near, f32 far) {
    CALL_GL(Ortho)(left, right, bottom, top, near, far);
}

#if 0 // don't using double right now
void tea_load_matrixd(const f64 *m) { CALL_GL(LoadMatrixd)(m); }
void tea_load_transpose_matrixd(const f64 *m) { CALL_GL(LoadTransposeMatrixd)(m); }
void tea_mult_matrixd(const f64 *m) { CALL_GL(MultMatrixd)(m); }
void tea_mult_transpose_matrixd(const f64 *m) { CALL_GL(MultTransposeMatrixd)(m); }
void tea_translated(f64 x, f64 y, f64 z) { CALL_GL(Translated)(x, y, z); }
void tea_rotated(f64 angle, f64 x, f64 y, f64 z) { CALL_GL(Rotated)(angle, x, y, z); }
void tea_scaled(f64 x, f64 y, f64 z) { CALL_GL(Scaled)(x, y, z); }
#endif
void tea_orthod(f64 left, f64 right, f64 bottom, f64 top, f64 near, f64 far) {
    CALL_GL(Ortho)(left, right, bottom, top, near, far);
}
void tea_frustumd(f64 left, f64 right, f64 bottom, f64 top, f64 near, f64 far) {
    CALL_GL(Frustum)(left, right, bottom, top, near, far);
}
void tea_perspective(f32 fovy, f32 aspect, f32 zNear, f32 zFar) {
    f32 ymax = zNear * tan(fovy * TEA_PI / 360.0);
    f32 ymin = -ymax;
    f32 xmin = ymin * aspect;
    f32 xmax = ymax * aspect;
    tea_frustumf(xmin, xmax, ymin, ymax, zNear, zFar);
}

/**************
 *  Internal  *
 **************/

#define CLONE_MATRIX(dest, src)\
for (i = 0; i < 16; i++) dest[i] = src[i]

#define TRANSPOSE_MATRIX(dest, src)\
for (i = 0; i < 4; i++)\
    for (j = 0; j < 4; j++)\
        dest[i * 4 + j] = src[j * 4 + i]

static void s_clone_matrixf(f32 *dest, const f32 *src) {
    i32 i;
    CLONE_MATRIX(dest, src);
}

static void s_transpose_matrixf(f32 *dest, const f32 *src) {
    i32 i, j;
    TRANSPOSE_MATRIX(dest, src);
}

static void s_mult_matrixf(f32 *dest, const f32 *a, const f32 *b) {
    i32 i, j, k;
    f32 tmp[16];
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            tmp[i * 4 + j] = 0.f;
            for (k = 0; k < 4; k++) {
                tmp[i * 4 + j] += a[i * 4 + k] * b[k * 4 + j];
            }
        }
    }
    s_clone_matrixf(dest, tmp);
}

#if 0 // don't using double right now
static void s_clone_matrixd(f64 *dest, const f64 *src) {
    i32 i;
    CLONE_MATRIX(dest, src);
}

static void s_transpose_matrixd(f64 *dest, const f64 *src) {
    i32 i, j;
    TRANSPOSE_MATRIX(dest, src);
}

static void s_mult_matrixd(f64 *dest, const f64 *a, const f64 *b) {
    i32 i, j, k;
    f64 tmp[16];
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            tmp[i * 4 + j] = 0.0f;
            for (k = 0; k < 4; k++) {
                tmp[i * 4 + j] += a[i * 4 + k] * b[k * 4 + j];
            }
        }
    }
    s_clone_matrixd(dest, tmp);
}
#endif

static void s_tea_matrix_mode(u32 mode) {
    TEA_ASSERT(mode < 3, "Invalid matrix mode");
    TEA()->matrix_mode = mode;
    STATE()->matrix = (f32*)MATRIX_TOP(mode);
    
    // struct matrix_stack_s *stack = &TEA()->matrix.stack[m];
    // TEA()->matrix.ptr = stack->m[stack->top];
}

static void s_tea_push_matrix(void) {
    // struct matrix_stack_s *stack = &TEA()->matrix.stack[TEA()->matrix.mode - TEA_MODELVIEW];

    i32 mode = TEA()->matrix_mode;
    TEA_ASSERT(TEA()->matrix[mode].top < MATRIX_STACK_SIZE - 1, "Matrix stack overflow");
    TEA()->matrix[mode].top++;
    s_clone_matrixf((f32*)MATRIX_TOP(mode), STATE()->matrix);
    STATE()->matrix = (f32*)MATRIX_TOP(mode);
}

static void s_tea_pop_matrix(void) {
    i32 mode = TEA()->matrix_mode;
    // struct matrix_stack_s *stack = &TEA()->matrix.stack[TEA()->matrix.mode - TEA_MODELVIEW];
    TEA_ASSERT(TEA()->matrix[mode].top > 0, "Matrix stack underflow");
    TEA()->matrix[mode].top--;
    STATE()->matrix = (f32*)MATRIX_TOP(mode);
}

static void s_tea_load_identity(void) {
    f32 *matrix = STATE()->matrix;
    for (u8 i = 0; i < 16; i++)
        matrix[i] = 0.f;
    matrix[0] = 1.f;
    matrix[5] = 1.f;
    matrix[10] = 1.f;
    matrix[15] = 1.f;
}

/* float */
static void s_tea_load_matrixf(const f32 *m) {
    TEA_ASSERT(m != NULL, "Invalid matrix");
    i32 i;
    CLONE_MATRIX(STATE()->matrix, m);
}

static void s_tea_load_transpose_matrixf(const f32 *m) {
    TEA_ASSERT(m != NULL, "Invalid matrix");
    i32 i, j;
    TRANSPOSE_MATRIX(STATE()->matrix, m);
}

static void s_tea_mult_matrixf(const f32 *m) {
    TEA_ASSERT(m != NULL, "Invalid matrix");
    f32 *ptr = STATE()->matrix;
    s_mult_matrixf(ptr, m, ptr);
}

static void s_tea_mult_transpose_matrixf(const f32 *m) {
    TEA_ASSERT(m != NULL, "Invalid matrix");
    f32 tmp[16];
    s_transpose_matrixf(tmp, m);
    s_mult_matrixf(STATE()->matrix, tmp, STATE()->matrix);
}

static void s_tea_translatef(f32 x, f32 y, f32 z) {
    f32 m[16] = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        x, y, z, 1.0f
    };
    s_tea_mult_matrixf(m);
}

static void s_tea_scalef(f32 x, f32 y, f32 z) {
    f32 m[16] = {
        x, 0.0f, 0.0f, 0.0f,
        0.0f, y, 0.0f, 0.0f,
        0.0f, 0.0f, z, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
    s_tea_mult_matrixf(m);
}

static void s_tea_rotatef(f32 angle, f32 x, f32 y, f32 z) {
    f32 c = cosf(TEA_DEG2RAD(angle));
    f32 s = sinf(TEA_DEG2RAD(angle));
    f32 nc = 1.f - c;
    f32 len = x*x + y*y + z*z;
    if (len > 0.f) {
        f32 rlen = 1.f / sqrtf(len);
        x *= rlen;
        y *= rlen;
        z *= rlen;
    }
    f32 m[16] = {
        x*x*nc + c,   y*x*nc + z*s, z*x*nc - y*s, 0.f,
        x*y*nc - z*s, y*y*nc + c,   z*y*nc + x*s, 0.f,
        x*z*nc + y*s, y*z*nc - x*s, z*z*nc + c,   0.f,
        0.f,         0.f,         0.f,         1.f
    };
    s_mult_matrixf(STATE()->matrix, m, STATE()->matrix);
}

static void s_tea_orthof(f32 left, f32 right, f32 bottom, f32 top, f32 near, f32 far) {
    f32 *ptr = STATE()->matrix;
    ptr[0] = 2.f / (right - left);
    ptr[1] = 0.f;
    ptr[2] = 0.f;
    ptr[3] = 0.f;

    ptr[4] = 0.f;
    ptr[5] = 2.f / (top - bottom);
    ptr[6] = 0.f;
    ptr[7] = 0.f;

    ptr[8] = 0.f;
    ptr[9] = 0.f;
    ptr[10] = -2.f / (far - near);
    ptr[11] = 0.f;

    ptr[12] = -(right + left) / (right - left);
    ptr[13] = -(top + bottom) / (top - bottom);
    ptr[14] = -(far + near) / (far - near);
    ptr[15] = 1.f;
}

static void s_tea_frustumf(f32 left, f32 right, f32 bottom, f32 top, f32 near, f32 far) {
    f32 m[16];
    f32 a = (right + left) / (right - left);
    f32 b = (top + bottom) / (top - bottom);
    f32 c = -(far + near) / (far - near);
    f32 d = -(2.f * far * near) / (far - near);
    m[0] = 2.f * near / (right - left);
    m[1] = 0.f;
    m[2] = 0.f;
    m[3] = 0.f;

    m[4] = 0.f;
    m[5] = 2.f * near / (top - bottom);
    m[6] = 0.f;
    m[7] = 0.f;

    m[8] = a;
    m[9] = b;
    m[10] = c;
    m[11] = -1.f;

    m[12] = 0.f;
    m[13] = 0.f;
    m[14] = d;
    m[15] = 0.f;
    s_tea_mult_matrixf(m);
}

#if 0 // don't using double right now
/* double */
static void s_tea_load_matrixd(const f64 *m) {
    TEA_ASSERT(m != NULL, "Invalid matrix");
    i32 i;
    CLONE_MATRIX(STATE()->matrix, (f32)m);
}

static void s_tea_load_transpose_matrixd(const f64 *m) {
    TEA_ASSERT(m != NULL, "Invalid matrix");
    i32 i, j;
    TRANSPOSE_MATRIX(STATE()->matrix, (f32)m);
}

static void s_tea_mult_matrixd(const f64 *m) {
    TEA_ASSERT(m != NULL, "Invalid matrix");
    f32 *ptr = STATE()->matrix;
    i32 i;
    s_mult_matrixf(ptr, m, ptr);
}

static void s_tea_mult_transpose_matrixd(const f64 *m) {
    TEA_ASSERT(m != NULL, "Invalid matrix");
    f32 tmp[16];
    i32 i, j;
    TRANSPOSE_MATRIX(tmp, m);
    s_mult_matrixf(STATE()->matrix, tmp, STATE()->matrix);
}

static void s_tea_translated(f64 x, f64 y, f64 z) {
    f32 m[16] = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        x, y, z, 1.0f
    };
    s_tea_mult_matrixf(m);
}

static void s_tea_scaled(f64 x, f64 y, f64 z) {
    f32 m[16] = {
        x, 0.f, 0.f, 0.f,
        0.f, y, 0.f, 0.f,
        0.f, 0.f, z, 0.f,
        0.f, 0.f, 0.f, 1.f
    };
    s_tea_mult_matrixf(m);
}

static void s_tea_rotated(f64 angle, f64 x, f64 y, f64 z) {
    f64 c = cos(TEA_DEG2RAD(angle));
    f64 s = sin(TEA_DEG2RAD(angle));
    f64 nc = 1.0 - c;
    f64 len = x * x + y * y + z * z;
    if (len > 0.0) {
        f64 rlen = 1.f / sqrt(len);
        x *= rlen;
        y *= rlen;
        z *= rlen;
    }
    f32 m[16] = {
        x*x*nc + c,   y*x*nc + z*s, z*x*nc - y*s, 0.0,
        x*y*nc - z*s, y*y*nc + c,   z*y*nc + x*s, 0.0,
        x*z*nc + y*s, y*z*nc - x*s, z*z*nc + c,   0.0,
        0.0,         0.0,         0.0,         1.0
    };
    s_mult_matrixf(STATE()->matrix, m, STATE()->matrix);
}

#endif
static void s_tea_orthod(f64 left, f64 right, f64 bottom, f64 top, f64 near, f64 far) {
    s_tea_orthof((f32)left, (f32)right, (f32)bottom, (f32)top, (f32)near, (f32)far);
}

static void s_tea_frustumd(f64 left, f64 right, f64 bottom, f64 top, f64 near, f64 far) {
    s_tea_frustumf((f32)left, (f32)right, (f32)bottom, (f32)top, (f32)near, (f32)far);
}

/*=================================*
 *             Loader              *
 *=================================*/

static te_bool s_tea_load_procs(te_proc_t *procs, u32 flags);

#if !defined(__APPLE__) && !defined(__HAIKU__)
void* (*s_proc_address)(const i8*);
#endif

te_bool s_load_gl(void) {
#if defined(_WIN32)
    s_glsym = LoadLibrary("opengl32.dll");
    TEA_ASSERT(s_glsym != NULL, "failed to load OpenGL32.dll");
#else
#if defined(__APPLE__)
    const char *names[] = {
        "../Frameworks/OpenGL.framework/OpenGL",
        "/Library/Frameworks/OpenGL.framework/Opengl",
        "/System/Library/Frameworks/OpenGL.framework/OpenGL",
        "/System/Library/Frameworks/OpenGL.framework/Versions/Current/OpenGL",
        NULL,
    };
#else
    const char *names[] = {
        "libGL.so.1",
        "libGL.so",
        NULL,
    };
#endif
    u32 index;
    for (index = 0; names[index] != NULL; ++index) {
        s_glsym = dlopen(names[index], RTLD_LAZY | RTLD_GLOBAL);
        if (s_glsym != NULL) {
#if defined(__APPLE__) || defined(__HAIKU__)
            return TEA_TRUE;
#else
            s_proc_address = (void*(*)(const i8*))dlsym(s_glsym, "glXGetProcAddress");
            return s_proc_address != NULL;
#endif
            break;
        }
    }
#endif
    return TEA_FALSE;
}

void s_close_gl(void) {
    if (s_glsym != NULL) {
#if defined(_WIN32)
        FreeLibrary(s_glsym);
#else
        dlclose(s_glsym);
#endif
        s_glsym = NULL;
    }
}

void s_setup_gl(void) {
    GET_GL(GetString) = s_get_proc("glGetString");
    GET_GL(GetStringi) = s_get_proc("glGetStringi");

    const i8 *version = (const i8*)CALL_GL(GetString)(TEA_GL_VERSION);
    const i8 *glsl = (const i8*)CALL_GL(GetString)(TEA_GL_SHADING_LANGUAGE_VERSION);
    TEA_ASSERT(version != NULL, "Failed to get OpenGL version");
    const i8 *prefixes[] = {
        "OpenGL ES-CM ",
        "OpenGL ES-CL ",
        "OpenGL ES ",
        NULL,
    };

    i8 *ver = (i8*)version;
    for (u32 i = 0; prefixes[i] != NULL; i++) {
        if (strncmp(ver, prefixes[i], strlen(prefixes[i])) == 0) {
            ver += strlen(prefixes[i]);
            GL()->version.es = TEA_TRUE;
            break;
        }
    }
    s_gl_max_ver.mag = ver[0] - '0';
    s_gl_max_ver.min = ver[2] - '0';
    if (GL()->version.major == 0) {
        GL()->version.major = s_gl_max_ver.mag;
        GL()->version.minor = s_gl_max_ver.min;
    }
    if (glsl == NULL) GL()->version.glsl = 100;
    else
        GL()->version.glsl = atof(glsl) * 100;

    fprintf(stderr, "OpenGL: %s\n", version);
    fprintf(stderr, "OpenGL shading language: %s\n", glsl);
    static const te_proc_t glBaseProcs[] = {
        /* Miscellaneous */
        { TEA_GL_ClearColor, { "glClearColor", NULL }},
        { TEA_GL_ClearDepth, { "glClearDepth", NULL }},
        { TEA_GL_Clear, { "glClear", NULL }},
        { TEA_GL_CullFace, { "glCullFace", NULL }},
        { TEA_GL_FrontFace, { "glFrontFace", NULL }},
        { TEA_GL_PolygonMode, { "glPolygonMode", NULL }},
        { TEA_GL_Scissor, { "glScissor", NULL }},
        { TEA_GL_ReadBuffer, { "glReadBuffer", NULL }},
        { TEA_GL_DrawBuffer, { "glDrawBuffer", NULL }},
        { TEA_GL_Enable, { "glEnable", NULL }},
        { TEA_GL_Disable, { "glDisable", NULL }},
        { TEA_GL_EnableClientState, { "glEnableClientState", NULL }},
        { TEA_GL_DisableClientState, { "glDisableClientState", NULL }},
        { TEA_GL_GetBooleanv, { "glGetBooleanv", NULL }},
        { TEA_GL_GetFloatv, { "glGetFloatv", NULL }},
        { TEA_GL_GetIntegerv, { "glGetIntegerv", NULL }},
        { TEA_GL_GetError, { "glGetError" }},
        /* Depth */
        { TEA_GL_DepthFunc, { "glDepthFunc", NULL }},
        { TEA_GL_DepthMask, { "glDepthMask", NULL }},
        { TEA_GL_DepthRange, { "glDepthRange", NULL }},
        /* Transformation */
        { TEA_GL_Viewport, { "glViewport", NULL }},
        { TEA_GL_MatrixMode, { "glMatrixMode", NULL }},
        { TEA_GL_LoadIdentity, { "glLoadIdentity", NULL }},
        { TEA_GL_LoadMatrixf, { "glLoadMatrixf", NULL }},
        { TEA_GL_LoadMatrixd, { "glLoadMatrixd", NULL }},
        { TEA_GL_MultMatrixf, { "glMultMatrixf", NULL }},
        { TEA_GL_MultMatrixd, { "glMultMatrixd", NULL }},
        { TEA_GL_Rotatef, { "glRotatef", NULL }},
        { TEA_GL_Rotated, { "glRotated", NULL }},
        { TEA_GL_Scalef, { "glScalef", NULL }},
        { TEA_GL_Scaled, { "glScaled", NULL }},
        { TEA_GL_Translatef, { "glTranslatef", NULL }},
        { TEA_GL_Translated, { "glTranslated", NULL }},
        { TEA_GL_Ortho, { "glOrtho", NULL }},
        { TEA_GL_Frustum, { "glFrustum", NULL }},
        { TEA_GL_Orthof, { "glOrthof", NULL }},
        { TEA_GL_Frustumf, { "glFrustumf", NULL }},
        { TEA_GL_PushMatrix, { "glPushMatrix", NULL }},
        { TEA_GL_PopMatrix, { "glPopMatrix", NULL }},
        { TEA_GL_LoadTransposeMatrixd, { "glLoadTransposeMatrixd", NULL }},
        { TEA_GL_LoadTransposeMatrixf, { "glLoadTransposeMatrixf", NULL }},
        { TEA_GL_MultTransposeMatrixd, { "glMultTransposeMatrixd", NULL }},
        { TEA_GL_MultTransposeMatrixf, { "glMultTransposeMatrixf", NULL }},
        /* Vertex arrays */
        { TEA_GL_VertexPointer, { "glVertexPointer", NULL }},
        { TEA_GL_NormalPointer, { "glNormalPointer", NULL }},
        { TEA_GL_ColorPointer, { "glColorPointer", NULL }},
        { TEA_GL_TexCoordPointer, { "glTexCoordPointer", NULL }},
        { TEA_GL_IndexPointer, { "glIndexPointer", NULL }},
        { TEA_GL_EdgeFlagPointer, { "glEdgeFlatPointer", NULL }},
        { TEA_GL_DrawArrays, { "glDrawArrays", NULL }},
        { TEA_GL_DrawElements, { "glDrawElements", NULL }},
    #if 0
        { TEA_GL_InterleavedArrays, { "glInterleavedArrays", NULL }},
        { TEA_GL_ClientActiveTexture, { "glClientActiveTexture", NULL }},
        { TEA_GL_ActiveTexture, { "glActiveTexture", NULL }},
        { TEA_GL_MultiTexCoord1d, { "glMultiTexCoord1d", NULL }},
        { TEA_GL_MultiTexCoord1dv, { "glMultiTexCoord1dv", NULL }},
    #endif
        /* Texture mapping */
        { TEA_GL_TexParameterf, { "glTexParameterf", NULL }},
        { TEA_GL_TexParameteri, { "glTexParameteri", NULL } },
        { TEA_GL_TexParameterfv, { "glTexParameterfv", NULL }},
        { TEA_GL_TexParameteriv, { "glTexParameteriv", NULL }},
        { TEA_GL_GetTexParameterfv, { "glGetTexParameterfv", NULL }},
        { TEA_GL_GetTexParameteriv, { "glGetTexParameteriv", NULL }},
        { TEA_GL_GenTextures, { "glGenTextures", NULL }},
        { TEA_GL_DeleteTextures, { "glDeleteTextures", NULL }},
        { TEA_GL_BindTexture, { "glBindTexture", NULL }},
        { TEA_GL_IsTexture, { "glIsTexture", NULL }},
        { TEA_GL_TexImage1D, { "glTexImage1D", NULL }},
        { TEA_GL_TexImage2D, { "glTexImage2D", NULL }},
        { TEA_GL_TexSubImage1D, { "glTexSubImage1D", NULL }},
        { TEA_GL_TexSubImage2D, { "glTexSubImage2D", NULL }},
        { TEA_GL_CopyTexImage1D, { "glCopyTexImage1D", NULL }},
        { TEA_GL_CopyTexImage2D, { "glCopyTexImage2D", NULL }},
        { TEA_GL_CopyTexSubImage1D, { "glCopyTexSubImage1D", NULL }},
        { TEA_GL_CopyTexSubImage2D, { "glCopyTexSubImage2D", NULL }},
        { TEA_GL_TexImage3D, { "glTexImage3D", NULL }},
        { TEA_GL_TexSubImage3D, { "glTexSubImage3D", NULL }},
        { TEA_GL_CopyTexSubImage3D, { "glCopyTexSubImage3D", NULL }},
        /* Vertex buffer object */
        { TEA_GL_GenBuffers, { "glGenBuffers", "glGenBuffersARB", NULL }},
        { TEA_GL_DeleteBuffers, { "glDeleteBuffers", "glDeleteBuffersARB", NULL }},
        { TEA_GL_BindBuffer, { "glBindBuffer", "glBindBufferARB", NULL }},
        { TEA_GL_BufferData, { "glBufferData", "glBufferDataARB", NULL }},
        { TEA_GL_BufferSubData, { "glBufferSubData", "glBufferSubDataARB", NULL }},
        { TEA_GL_MapBuffer, { "glMapBuffer", "glMapBufferARB", NULL }},
        { TEA_GL_UnmapBuffer, { "glUnmapBuffer", "glUnmapBufferARB", NULL }},
        /* Vertex program */
        { TEA_GL_VertexAttribPointer, { "glVertexAttribPointer", "glVertexAttribPointerARB", NULL }},
        { TEA_GL_EnableVertexAttribArray, { "glEnableVertexAttribArray", "glEnableVertexAttribArrayARB", NULL }},
        { TEA_GL_DisableVertexAttribArray, { "glDisableVertexAttribArray", "glDisableVertexAttribArrayARB", NULL }},
        /* Framebuffer */
        { TEA_GL_IsRenderbuffer, { "glIsRenderbuffer", "glIsRenderbufferEXT", NULL }},
        { TEA_GL_BindRenderbuffer, { "glBindRenderbuffer", "glBindRenderbufferEXT", NULL }},
        { TEA_GL_DeleteRenderbuffers, { "glDeleteRenderbuffers", "glDeleteRenderbuffersEXT", NULL }},
        { TEA_GL_GenRenderbuffers, { "glGenRenderbuffers", "glGenRenderbuffersEXT", NULL }},
        { TEA_GL_RenderbufferStorage, { "glRenderbufferStorage", "glRenderbufferStorageEXT", NULL }},
        { TEA_GL_IsFramebuffer, { "glIsFramebuffer", "glIsFramebufferEXT", NULL }},
        { TEA_GL_BindFramebuffer, { "glBindFramebuffer", "glBindFramebufferEXT", NULL }},
        { TEA_GL_DeleteFramebuffers, { "glDeleteFramebuffers", "glDeleteFramebuffersEXT", NULL }},
        { TEA_GL_GenFramebuffers, { "glGenFramebuffers", "glGenFramebuffersEXT", NULL }},
        { TEA_GL_CheckFramebufferStatus, { "glCheckFramebufferStatus", "glCheckFramebufferStatusEXT", NULL }},
        { TEA_GL_FramebufferTexture2D, { "glFramebufferTexture2D", "glFramebufferTexture2DEXT", NULL }},
        { TEA_GL_FramebufferRenderbuffer, { "glFramebufferRenderbuffer", "glFramebufferRenderbufferEXT", NULL }},
        { TEA_GL_GenerateMipmap, { "glGenerateMipmap", "glGenerateMipmapEXT", NULL }},
        { TEA_GL_BlitFramebuffer, { "glBlitFramebuffer", "glBlitFramebufferEXT", NULL }},
        /* Shader */
        { TEA_GL_CreateShader, { "glCreateShader", "glCreateShaderObjectARB", NULL }},
        { TEA_GL_DeleteShader, { "glDeleteShader", "glDeleteObjectARB", NULL }},
        { TEA_GL_ShaderSource, { "glShaderSource", "glShaderSourcerARB", NULL }},
        { TEA_GL_CompileShader, { "glCompileShader", "glCompileShaderARB", NULL }},
        { TEA_GL_GetShaderiv, { "glGetShaderiv", "glGetObjectParameterivARB", NULL }},
        { TEA_GL_GetShaderInfoLog, { "glGetShaderInfoLog", "glGetInfoLogARB", NULL }},
        { TEA_GL_CreateProgram, { "glCreateProgram", "glCreateProgramObjectARB", NULL }},
        { TEA_GL_DeleteProgram, { "glDeleteProgram", "glDeleteObjectARB", NULL }},
        { TEA_GL_AttachShader, { "glAttachShader", "glAttachObjectARB", NULL }},
        { TEA_GL_DetachShader, { "glDetachShader", "glDetachObjectARB", NULL }},
        { TEA_GL_LinkProgram, { "glLinkProgram", "glLinkProgramARB", NULL }},
        { TEA_GL_GetProgramiv, { "glGetProgramiv", "glGetObjectParameterivARB", NULL }},
        { TEA_GL_GetProgramInfoLog, { "glGetProgramInfoLog", "glGetInfoLogARB", NULL }},
        { TEA_GL_UseProgram, { "glUseProgram", "glUseProgramObjectARB", NULL }},
        { TEA_GL_GetUniformLocation, { "glGetUniformLocation", "glGetUniformLocationARB", NULL }},
        { TEA_GL_Uniform1f, { "glUniform1f", "glUniform1fARB", NULL }},
        { TEA_GL_Uniform2f, { "glUniform2f", "glUniform2fARB", NULL }},
        { TEA_GL_Uniform3f, { "glUniform3f", "glUniform3fARB", NULL }},
        { TEA_GL_Uniform4f, { "glUniform4f", "glUniform4fARB", NULL }},
        { TEA_GL_Uniform1i, { "glUniform1i", "glUniform1iARB", NULL }},
        { TEA_GL_Uniform2i, { "glUniform2i", "glUniform2iARB", NULL }},
        { TEA_GL_Uniform3i, { "glUniform3i", "glUniform3iARB", NULL }},
        { TEA_GL_Uniform4i, { "glUniform4i", "glUniform4iARB", NULL }},
        { TEA_GL_Uniform1fv, { "glUniform1fv", "glUniform1fvARB", NULL }},
        { TEA_GL_Uniform2fv, { "glUniform2fv", "glUniform2fvARB", NULL }},
        { TEA_GL_Uniform3fv, { "glUniform3fv", "glUniform3fvARB", NULL }},
        { TEA_GL_Uniform4fv, { "glUniform4fv", "glUniform4fvARB", NULL }},
        { TEA_GL_Uniform1iv, { "glUniform1iv", "glUniform1ivARB", NULL }},
        { TEA_GL_Uniform2iv, { "glUniform2iv", "glUniform2ivARB", NULL }},
        { TEA_GL_Uniform3iv, { "glUniform3iv", "glUniform3ivARB", NULL }},
        { TEA_GL_Uniform4iv, { "glUniform4iv", "glUniform4ivARB", NULL }},
        { TEA_GL_UniformMatrix2fv, { "glUniformMatrix2fv", "glUniformMatrix2fvARB", NULL }},
        { TEA_GL_UniformMatrix3fv, { "glUniformMatrix3fv", "glUniformMatrix3fvARB", NULL }},
        { TEA_GL_UniformMatrix4fv, { "glUniformMatrix4fv", "glUniformMatrix4fvARB", NULL }},
        { TEA_GL_GetAttribLocation, { "glGetAttribLocation", "glGetAttribLocationARB", NULL }},
        { TEA_GL_BindAttribLocation, { "glBindAttribLocation", "glBindAttribLocationARB", NULL }},
        { TEA_GL_GetActiveUniform, { "glGetActiveUniform", "glGetActiveUniformARB", NULL }},
        { TEA_GL_GetActiveAttrib, { "glGetActiveAttrib", "glGetActiveAttribARB", NULL }},
        /* Vertex array object */
        { TEA_GL_GenVertexArrays, { "glGenVertexArrays", NULL }},
        { TEA_GL_DeleteVertexArrays, { "glDeleteVertexArrays", NULL }},
        { TEA_GL_BindVertexArray, { "glBindVertexArray", NULL }},
        { 0, { NULL }}
    };

    s_tea_load_procs((te_proc_t*)glBaseProcs, TEA_PROC_OVERRIDE);

    GL()->extensions |= TEA_HAS_VAO * (GET_GL(GenVertexArrays) != NULL);
    GL()->extensions |= TEA_HAS_VBO * (GET_GL(GenBuffers) != NULL);
    GL()->extensions |= TEA_HAS_SHADER * (GET_GL(CreateShader) != NULL);
#if 0
    if (GL()->extensions & TEA_HAS_VAO) {
        s_tea_bind_vao = s_tea_bind_vao3;
    } else if (GL()->extensions & TEA_HAS_VBO) {
        s_tea_bind_vao = s_tea_bind_vao2;
    } else {
        s_tea_bind_vao = s_tea_bind_vao1;
    }
#endif

    if (GL()->extensions & TEA_HAS_SHADER) {
        GET_GL(MatrixMode) = s_tea_matrix_mode;
        GET_GL(PushMatrix) = s_tea_push_matrix;
        GET_GL(PopMatrix) = s_tea_pop_matrix;
        GET_GL(LoadIdentity) = s_tea_load_identity;

        GET_GL(LoadMatrixf) = s_tea_load_matrixf;
        GET_GL(LoadTransposeMatrixf) = s_tea_load_transpose_matrixf;
        GET_GL(MultMatrixf) = s_tea_mult_matrixf;
        GET_GL(MultTransposeMatrixf) = s_tea_mult_transpose_matrixf;
        GET_GL(Translatef) = s_tea_translatef;
        GET_GL(Scalef) = s_tea_scalef;
        GET_GL(Rotatef) = s_tea_rotatef;
        GET_GL(Orthof) = s_tea_orthof;
        GET_GL(Frustumf) = s_tea_frustumf;

#if 0 // don't using double right now
        GET_GL(LoadMatrixd) = s_tea_load_matrixd;
        GET_GL(LoadTransposeMatrixd) = s_tea_load_transpose_matrixd;
        GET_GL(MultMatrixd) = s_tea_mult_matrixd;
        GET_GL(MultTransposeMatrixd) = s_tea_mult_transpose_matrixd;
        GET_GL(Translated) = s_tea_translated;
        GET_GL(Scaled) = s_tea_scaled;
        GET_GL(Rotated) = s_tea_rotated;
#endif

        GET_GL(Ortho) = s_tea_orthod;
        GET_GL(Frustum) = s_tea_frustumd;
    }
}

void* s_get_proc(const i8 *name) {
    void *sym = NULL;
    if (s_glsym == NULL) return sym;
#if !defined(__APPLE__) && !defined(__HAIKU__)
    if (s_proc_address != NULL) {
        sym = s_proc_address(name);
    }
#endif
    if (sym == NULL) {
#if defined(_WIN32) || defined(__CYGWIN__)
        sym = (void*)GetProcAddress(s_glsym, name);
#else
        sym = (void*)dlsym(s_glsym, name);
#endif
    }
    return sym;
}

te_bool s_tea_load_procs(te_proc_t *procs, u32 flags) {
    te_proc_t *proc = procs;
    while (proc->names[0]) {
        if (!GL()->procs[proc->tag] || (flags & TEA_PROC_OVERRIDE)) {
            u32 i = 0;
            i8 **names = (i8**)proc->names;
            while (names[i] &&  i < 3) {
                if ((GL()->procs[proc->tag] = s_get_proc(names[i])))
                    break;
                i++;
            }
        }
        else if (flags & TEA_PROC_RET_ON_DUP)
            return TEA_TRUE;

        proc++;
    }
    return TEA_TRUE;
}
