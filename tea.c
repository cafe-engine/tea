#include "tea.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define GL_SILENCE_DEPRECATION 1
#define LOAD_PROC(proc)\
gl()->proc = (GL##proc##Proc)getProcGL("gl"#proc)

#define LOAD_PROC_EXT(proc, ext)\
gl()->proc = (GL##proc##Proc)getProcGL("gl"#proc#ext)

#define TEA_PROC(proc)\
tea##proc = gl()->procs[TEA_GL_##proc]

#define GET_GL(name)\
(gl()->procs[TEA_GL_##name])

#define CALL_GL(name)\
((GL##name##Proc)GET_GL(name))

#if defined(__EMSCRIPTEN__)
    #include <emscripten.h>
#endif

#if defined(_WIN32)
    #include <windows.h>
    #ifndef WINDOWS_LEAN_AND_MEAN
        #define WINDOWS_LEAN_AND_MEAN 1
    #endif
    static HMODULE openglSym;
#else
    #include <dlfcn.h>
    static void *openglSym;
    #ifndef RTLD_LAZY
        #define RTLD_LAZY 0x00001
        #endif
    #ifndef RTLD_GLOBAL
        #define RTLD_GLOBAL 0x00100
    #endif
#endif

#define tea() (&s_teaCtx)
#define gl() (&tea()->gl)
#define state() (&tea()->state)
#define base() (&tea()->base)

static te_bool loadLibGL(void);
static void setupGLProcs(void);
static void closeLibGL(void);
static void* getProcGL(const char* name);

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
    TEA_GL_ARB_vertex_buffer_object = 1 << 0,
    TEA_GL_ARB_vertex_array_object = 1 << 1,
    TEA_GL_EXT_framebuffer_object = 1 << 2,
    TEA_GL_ARB_shader_objects = 1 << 3,
    TEA_GL_ARB_shading_language_100 = 1 << 4,
    TEA_GL_ARB_vertex_shader = 1 << 5,
    TEA_GL_ARB_fragment_shader = 1 << 6,
    TEA_GL_ARB_vertex_program = 1 << 7,
};

struct te_vao_s {
    te_uint handle;
    te_buffer_t* buffers[2];
    te_vao_format_t format;
};

struct te_buffer_s {
    te_uint handle;
    te_uint target, usage;
    te_uint offset, size;
    te_ubyte *data;
};

/* Miscellaneous */
typedef void(*GLClearColorProc)(te_float, te_float, te_float, te_float);
typedef void(*GLClearProc)(te_uint);
typedef void(*GLBlendFuncProc)(te_uint, te_uint);
typedef void(*GLLogicOpProc)(te_uint);
typedef void(*GLCullFaceProc)(te_uint);
typedef void(*GLFrontFaceProc)(te_uint);
typedef void(*GLPolygonModeProc)(te_uint, te_uint);
typedef void(*GLScissorProc)(te_int, te_int, te_int, te_int);
typedef void(*GLDrawBufferProc)(te_uint);
typedef void(*GLReadBufferProc)(te_uint);
typedef void(*GLEnableProc)(te_uint);
typedef void(*GLDisableProc)(te_uint);

typedef void(*GLEnableClientStateProc)(te_uint); /* 1.1 */
typedef void(*GLDisableClientStateProc)(te_uint); /* 1.1 */

typedef void(*GLGetBooleanvProc)(te_uint, te_bool*);
typedef void(*GLGetDoublevProc)(te_uint, te_double*);
typedef void(*GLGetFloatvProc)(te_uint, te_float*);
typedef void(*GLGetIntegervProc)(te_uint, te_int*);
typedef void(*GLGetErrorProc)(void);
typedef const te_ubyte*(*GLGetStringProc)(te_uint);

typedef const te_ubyte*(*GLGetStringiProc)(te_uint, te_uint); /* 3.0 */

/* Depth buffer */
typedef void(*GLClearDepthProc)(te_float);
typedef void(*GLDepthFuncProc)(te_uint);
typedef void(*GLDepthMaskProc)(te_bool);
typedef void(*GLDepthRangeProc)(te_double, te_double);

/* Transformation */
typedef void(*GLViewportProc)(te_int, te_int, te_int, te_int);
typedef void(*GLMatrixModeProc)(te_uint);
typedef void(*GLPushMatrixProc)(void);
typedef void(*GLPopMatrixProc)(void);
typedef void(*GLLoadIdentityProc)(void);
typedef void(*GLLoadMatrixfProc)(const te_float*);
typedef void(*GLLoadMatrixdProc)(const te_double*);
typedef void(*GLMultMatrixfProc)(const te_float*);
typedef void(*GLMultMatrixdProc)(const te_double*);
typedef void(*GLOrthoProc)(te_double, te_double, te_double, te_double, te_double, te_double);
typedef void(*GLFrustumProc)(te_double, te_double, te_double, te_double, te_double, te_double);
typedef void(*GLTranslatefProc)(te_float, te_float, te_float);
typedef void(*GLRotatefProc)(te_float, te_float, te_float, te_float);
typedef void(*GLScalefProc)(te_float, te_float, te_float);
typedef void(*GLTranslatedProc)(te_double, te_double, te_double);
typedef void(*GLRotatedProc)(te_double, te_double, te_double, te_double);
typedef void(*GLScaledProc)(te_double, te_double, te_double);

typedef void(*GLLoadTransposeMatrixdProc)(const te_double[16]); /* 1.3 */
typedef void(*GLLoadTransposeMatrixfProc)(const te_float[16]); /* 1.3 */
typedef void(*GLMultTransposeMatrixdProc)(const te_double[16]); /* 1.3 */
typedef void(*GLMultTransposeMatrixfProc)(const te_float[16]); /* 1.3 */

/* Vertex Arrays */
typedef void(*GLVertexPointerProc)(te_int, te_uint, te_int, const te_void*);
typedef void(*GLColorPointerProc)(te_int, te_uint, te_int, const te_void*);
typedef void(*GLTexCoordPointerProc)(te_int, te_uint, te_int, const te_void*);
typedef void(*GLNormalPointerProc)(te_uint, te_int, const te_void*);
typedef void(*GLIndexPointerProc)(te_uint, te_int, const te_void*);
typedef void(*GLEdgeFlagPointerProc)(te_int, te_int, const te_void*);

typedef void(*GLDrawArraysProc)(te_uint, te_int, te_int);
typedef void(*GLDrawElementsProc)(te_uint, te_int, te_uint, const te_void*);

/* Texture mapping */
typedef void(*GLTexParameterfProc)(te_uint, te_uint, te_float);
typedef void(*GLTexParameteriProc)(te_uint, te_uint, te_int);
typedef void(*GLTexParameterfvProc)(te_uint, te_uint, const te_float*);
typedef void(*GLTexParameterivProc)(te_uint, te_uint, const te_int*);

typedef void(*GLGetTexParameterfProc)(te_uint, te_uint, te_float*);
typedef void(*GLGetTexParameteriProc)(te_uint, te_uint, te_int*);
typedef void(*GLGetTexParameterfvProc)(te_uint, te_uint, te_float*);
typedef void(*GLGetTexParameterivProc)(te_uint, te_uint, te_int*);

typedef void(*GLGenTexturesProc)(te_uint, te_uint*);
typedef void(*GLDeleteTexturesProc)(te_uint, te_uint*);
typedef void(*GLBindTextureProc)(te_uint, te_uint);
typedef void(*GLTexImage1DProc)(te_uint, te_int, te_int, te_int, te_int, te_uint, te_uint, const te_void*);
typedef void(*GLTexImage2DProc)(te_uint, te_int, te_int, te_int, te_int, te_int, te_uint, te_uint, const te_void*);
typedef void(*GLTexImage3DProc)(te_uint, te_int, te_int, te_int, te_int, te_int, te_int, te_uint, te_uint, const te_void*);
typedef void(*GLTexSubImage1DProc)(te_uint, te_int, te_int, te_int, te_uint, te_uint, const te_void*);
typedef void(*GLTexSubImage2DProc)(te_uint, te_int, te_int, te_int, te_int, te_int, te_uint, te_uint, const te_void*);
typedef void(*GLTexSubImage3DProc)(te_uint, te_int, te_int, te_int, te_int, te_int, te_int, te_uint, te_uint, const te_void*);
typedef void(*GLCopyTexImage1DProc)(te_uint, te_int, te_uint, te_int, te_int, te_int, te_int);
typedef void(*GLCopyTexImage2DProc)(te_uint, te_int, te_uint, te_int, te_int, te_int, te_int, te_int);
typedef void(*GLCopyTexSubImage1DProc)(te_uint, te_int, te_int, te_int, te_int, te_int);
typedef void(*GLCopyTexSubImage2DProc)(te_uint, te_int, te_int, te_int, te_int, te_int, te_int, te_int);
typedef void(*GLCopyTexSubImage3DProc)(te_uint, te_int, te_int, te_int, te_int, te_int, te_int, te_int, te_int, te_int);

/* GL_ARB_vertex_buffer_object */
typedef void(*GLBindBufferProc)(te_uint, te_uint);
typedef void(*GLBufferDataProc)(te_uint, te_uint, const te_void*, te_uint);
typedef void(*GLBufferSubDataProc)(te_uint, te_uint, te_uint, const te_void*);
typedef void(*GLGenBuffersProc)(te_uint, te_uint*);
typedef void(*GLDeleteBuffersProc)(te_uint, te_uint*);

/* GL_ARB_vertex_array_object */
typedef void(*GLGenVertexArraysProc)(te_uint, te_uint*);
typedef void(*GLBindVertexArrayProc)(te_uint);
typedef void(*GLDeleteVertexArraysProc)(te_uint, te_uint*);

/* GL_ARB_vertex_array_program */
typedef void(*GLVertexAttribPointerProc)(te_uint, te_int, te_uint, te_int, te_int, const te_void*);
typedef void(*GLEnableVertexAttribArrayProc)(te_uint);
typedef void(*GLDisableVertexAttribArrayProc)(te_uint);

/* GL_EXT_framebuffer_object */
typedef te_bool(*GLIsRenderbufferProc)(te_uint);
typedef void(*GLBindRenderbufferProc)(te_uint, te_uint);
typedef void(*GLDeleteRenderbuffersProc)(te_uint, te_uint*);
typedef void(*GLGenRenderbuffersProc)(te_uint, te_uint*);
typedef void(*GLRenderbufferStorageProc)(te_uint, te_uint, te_uint, te_uint);
typedef void(*GLGetRenderbufferParameterivProc)(te_uint, te_uint, te_int*);

typedef te_bool(*GLIsFramebufferProc)(te_uint);
typedef void(*GLBindFramebufferProc)(te_uint, te_uint);
typedef void(*GLDeleteFramebuffersProc)(te_uint, te_uint*);
typedef void(*GLGenFramebuffersProc)(te_uint, te_uint*);
typedef void(*GLFramebufferRenderbufferProc)(te_uint, te_uint, te_uint, te_uint);
typedef void(*GLFramebufferTexture1DProc)(te_uint, te_uint, te_uint, te_uint, te_int);
typedef void(*GLFramebufferTexture2DProc)(te_uint, te_uint, te_uint, te_uint, te_int);
typedef void(*GLFramebufferTexture3DProc)(te_uint, te_uint, te_uint, te_uint, te_int, te_int);
typedef void(*GLFramebufferTextureLayerProc)(te_uint, te_uint, te_uint, te_int, te_int);
typedef te_uint(*GLCheckFramebufferStatusProc)(te_uint);
typedef void(*GLGetFramebufferAttachmentParameterivProc)(te_uint, te_uint, te_uint, te_int*);
typedef void(*GLBlitFramebufferProc)(te_int, te_int, te_int, te_int, te_int, te_int, te_int, te_int, te_uint);
typedef void(*GLGenerateMipmapProc)(te_uint);

/* GL_ARB_shader_objects */
typedef void(*GLDeleteShaderProc)(te_uint);
typedef te_uint(*GLCreateShaderProc)(te_uint);
typedef void(*GLShaderSourceProc)(te_uint, te_int, const te_byte**, const te_int*);
typedef void(*GLCompileShaderProc)(te_uint);
typedef te_uint(*GLGetShaderivProc)(te_uint, te_uint, te_int*);
typedef te_uint(*GLGetShaderInfoLogProc)(te_uint, te_int, te_int*, te_byte*);

typedef te_uint(*GLCreateProgramProc)(void);
typedef void(*GLDeleteProgramProc)(te_uint);
typedef void(*GLAttachShaderProc)(te_uint, te_uint);
typedef void(*GLDetachShaderProc)(te_uint, te_uint);
typedef void(*GLLinkProgramProc)(te_uint);
typedef void(*GLUseProgramProc)(te_uint);

typedef void(*GLGetProgramivProc)(te_uint, te_uint, te_int*);
typedef void(*GLGetProgramInfoLogProc)(te_uint, te_int, te_int*, te_byte*);
typedef void(*GLGetActiveUniformProc)(te_uint, te_uint, te_int, te_int*, te_int*, te_int*, te_byte*);
typedef void(*GLGetUniformLocationProc)(te_uint, const te_byte*, te_int*);

/* GL_ARB_vertex_shader */
typedef te_int(*GLGetAttribLocationProc)(te_uint prog, const te_byte* name);
typedef void(*GLGetActiveAttribProc)(te_uint prog, te_uint index, te_int bufSize, te_int* length, te_int* size, te_uint* type, te_byte* name);
typedef void(*GLBindAttribLocationProc)(te_uint prog, te_uint index, const te_byte* name);

static te_ubyte glMaxMajor, glMaxMinor;
struct te_gl_s {
    struct {
        te_ubyte major, minor;
        te_ushort glsl;
        te_bool es;
    } version;
    te_uint extensions;

    void* procs[TEA_GL_PROC_COUNT];
};

#define MAX_MATRIX_STACK 32
struct matrix_stack_s {
    te_double m[MAX_MATRIX_STACK][16];
    te_ubyte top;
};

struct te_base_s {
    te_vao_t *vao;
    te_buffer_t *vbo, *ibo;
    te_shader_t shader2D, shader3D;
    te_vao_format_t format2D, format3D;
};

struct te_state_s {
    te_uint clearFlags;
    te_texture_t *texture;
    te_shader_t baseShader, currentShader;
    te_vao_t *vao;
    struct {
        te_uint mode;
        te_float pos[3];
        te_float color[4];
        te_float texcoord[2];
        te_float normal[3];
    } vertex;
    struct {
        struct matrix_stack_s stack[3];
        te_uint mode;
        te_double *ptr;
    } matrix;
};

struct Tea {
    struct te_gl_s gl;
    struct te_state_s state;
    struct te_base_s base;
};

static Tea s_teaCtx;
static te_vao_t s_defaultVao;

te_config_t teaConfig(const char *glslVersion) {
    te_config_t config;
    memset(&config, 0, sizeof(te_config_t));
    if (glslVersion) memcpy(config.glslVersion, glslVersion, sizeof(config.glslVersion));
    config.vboMode = TEA_DYNAMIC_DRAW;
    config.vboSize = 4000;
    config.iboMode = TEA_DYNAMIC_DRAW;
    config.iboSize = 6000;

    return config;
}

#define TEA_GL_VERSION 0x1F02
#define TEA_GL_SHADING_LANGUAGE_VERSION 0x8B8C

te_int teaInit(te_config_t *config) {
    TEA_ASSERT(config != NULL, "config is NULL");
    memset(tea(), 0, sizeof(Tea));
    memset(&s_defaultVao, 0, sizeof(te_vao_t));
    gl()->version.major = config->glMag;
    gl()->version.minor = config->glMin;
    gl()->version.es = config->glES;

    if (loadLibGL()) {
        setupGLProcs();
        closeLibGL();
    } else
        TEA_ASSERT(0, "failed to initialize OpenGL");

    state()->vertex.color[3] = 1.0f;
    teaMatrixMode(TEA_MODELVIEW);
    teaLoadIdentity();
    teaMatrixMode(TEA_PROJECTION);
    teaLoadIdentity();

    state()->clearFlags = TEA_COLOR_BUFFER_BIT | TEA_DEPTH_BUFFER_BIT;
    base()->vbo = teaBuffer(TEA_ARRAY_BUFFER, config->vboSize, config->vboMode);
    base()->ibo = teaBuffer(TEA_ELEMENT_ARRAY_BUFFER, config->iboSize, config->iboMode);

    teaVAOFormat(&base()->format2D);
    teaVAOFormatAdd(&base()->format2D, TEA_ATTRIB_POSITION_3D);
    teaVAOFormatAdd(&base()->format2D, TEA_ATTRIB_COLOR);
    //teaVAOFormatAdd(&base()->format2D, TEA_ATTRIB_TEXCOORD);
    base()->vao = teaVAO();
    state()->vao = &s_defaultVao;
#if 1
    teaBindVAO(base()->vao);
    teaBindBuffer(TEA_ARRAY_BUFFER, base()->vbo);
    teaBindBuffer(TEA_ELEMENT_ARRAY_BUFFER, base()->ibo);
    teaBindVAOFormat(&base()->format2D);
    teaBindVAO(NULL);
#endif
    return TEA_OK;
}

void teaQuit(void) {

}

/* Imediate mode */
void teaBegin(te_uint mode) {
    TEA_ASSERT(mode >= TEA_POINTS && mode <= TEA_TRIANGLE_FAN, "invalid mode");
    state()->vertex.mode = mode;
    te_uint projLocation = teaGetUniformLocation(1, "u_Projection");
    te_uint viewLocation = teaGetUniformLocation(1, "u_ModelView");

    te_uint locations[] = { viewLocation, projLocation };
    for (te_uint i = 0; i < 2; i++) {
        struct matrix_stack_s *stack = &state()->matrix.stack[i];
        te_float m[16];
        for (te_uint j = 0; j < 16; j++)
            m[j] = stack->m[stack->top][j];
        if (locations[i] != -1)
            teaUniformMatrix4fv(locations[i], 1, TEA_FALSE, m);
    }
    te_vao_t *vao = state()->vao->handle ? state()->vao : base()->vao;
    teaBindVAO(base()->vao);
    teaBindBuffer(TEA_ARRAY_BUFFER, base()->vbo);
    teaSeekBuffer(base()->vbo, 0);
}

void teaEnd(void) {
    teaFlushBuffer(TEA_ARRAY_BUFFER);
    te_int size = 0;
    te_vao_format_t *format = &state()->vao->format;
    if (format->stride != 0)
        size = base()->vbo->offset / format->stride;
    CALL_GL(DrawArrays)(state()->vertex.mode, 0, size);
    teaBindVAO(NULL);
}

void teaVertex2f(te_float x, te_float y) {
    te_float p[] = {x, y};
    teaVertex2fv(p);
}

void teaVertex3f(te_float x, te_float y, te_float z) {
    te_float p[] = {x, y, z};
    teaVertex3fv(p);
}

void teaVertex4f(te_float x, te_float y, te_float z, te_float w) {
    te_float p[] = {x, y, z, w};
    teaVertex4fv(p);
}

void teaVertex2fv(te_float *v) {
    te_float vv[] = {v[0], v[1], 0.0f, 1.0f};
    teaVertex4fv(vv);
}

void teaVertex3fv(te_float *v) {
    te_float vv[] = {v[0], v[1], v[2], 1.0f};
    teaVertex4fv(vv);
}

void teaVertex4fv(te_float *v) {
    te_float *ptr = (te_float*)(base()->vbo->data + base()->vbo->offset);
    te_vao_format_t *format = &state()->vao->format;
    ptr[0] = v[0];
    ptr[1] = v[1];
    ptr[2] = v[2];
    ptr[3] = v[3];
    te_uint i = format->attribs[0].size;
    ptr[i] = state()->vertex.color[0];
    ptr[i+1] = state()->vertex.color[1];
    ptr[i+2] = state()->vertex.color[2];
    ptr[i+3] = state()->vertex.color[3];
    ptr[i+4] = state()->vertex.texcoord[0];
    ptr[i+5] = state()->vertex.texcoord[1];
    ptr[i+6] = state()->vertex.normal[0];
    ptr[i+7] = state()->vertex.normal[1];
    ptr[i+8] = state()->vertex.normal[2];
    base()->vbo->offset += format->stride;
}

void teaColor3f(te_float r, te_float g, te_float b) {
    te_float c[] = {r, g, b};
    teaColor3fv(c);
}

void teaColor3ub(te_ubyte r, te_ubyte g, te_ubyte b) {
    te_float c[] = {r / 255.0f, g / 255.0f, b / 255.0f};
    teaColor3fv(c);
}

void teaColor4f(te_float r, te_float g, te_float b, te_float a) {
    te_float c[] = {r, g, b, a};
    teaColor4fv(c);
}

void teaColor4ub(te_ubyte r, te_ubyte g, te_ubyte b, te_ubyte a) {
    te_float c[] = {r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f};
    teaColor4fv(c);
}

void teaColor3fv(te_float *v) {
    state()->vertex.color[0] = v[0];
    state()->vertex.color[1] = v[1];
    state()->vertex.color[2] = v[2];
}

void teaColor4fv(te_float *v) {
    state()->vertex.color[0] = v[0];
    state()->vertex.color[1] = v[1];
    state()->vertex.color[2] = v[2];
    state()->vertex.color[3] = v[3];
}

void teaTexCoord2f(te_float x, te_float y) {
    te_float c[] = {x, y};
    teaTexCoord2fv(c);
}

void teaTexCoord2fv(te_float *v) {
    state()->vertex.texcoord[0] = v[0];
    state()->vertex.texcoord[1] = v[1];
}

void teaNormal3f(te_float x, te_float y, te_float z) {
    te_float n[] = {x, y, z};
    teaNormal3fv(n);
}

void teaNormal3fv(te_float *v) {
    state()->vertex.normal[0] = v[0];
    state()->vertex.normal[1] = v[1];
    state()->vertex.normal[2] = v[2];
}

/* Transforming */

static void s_cloneMatrixd(te_double *dst, const te_double *src) {
    for (te_ubyte i = 0; i < 16; i++)
        dst[i] = src[i];
}

static void s_multMatrixd(te_double *m, const te_double *a, const te_double *b) {
    te_double r[16];
    for (te_ubyte i = 0; i < 4; i++) {
        for (te_ubyte j = 0; j < 4; j++) {
            r[i*4 + j] = 0.0f;
            for (te_ubyte k = 0; k < 4; k++)
                r[i*4 + j] += a[i*4 + k] * b[k*4 + j];
        }
    }
    s_cloneMatrixd(m, r);
}

static void s_cloneMatrixf(te_float *dst, const te_float *src) {
    for (te_ubyte i = 0; i < 16; i++)
        dst[i] = src[i];
}

static void s_multMatrixf(te_float *m, const te_float *a, const te_float *b) {
    te_float r[16];
    for (te_ubyte i = 0; i < 4; i++) {
        for (te_ubyte j = 0; j < 4; j++) {
            r[i*4 + j] = 0.0f;
            for (te_ubyte k = 0; k < 4; k++)
                r[i*4 + j] += a[i*4 + k] * b[k*4 + j];
        }
    }
    s_cloneMatrixf(m, r);
}

void _teaOrthodTof(te_double left, te_double right, te_double bottom, te_double top, te_double zNear, te_double zFar) {
    teaOrthof((te_float)left, (te_float)right, (te_float)bottom, (te_float)top, (te_float)zNear, (te_float)zFar);
}

void _teaOrthofTod(te_float left, te_float right, te_float bottom, te_float top, te_float zNear, te_float zFar) {
    teaOrthod(left, right, bottom, top, zNear, zFar);
}

static void s_teaMatrixMode_core(te_uint mode) {
    te_ubyte m = mode - TEA_MODELVIEW;
    TEA_ASSERT(m < 3, "invalid matrix mode");
    state()->matrix.mode = mode;
    struct matrix_stack_s *stack = &state()->matrix.stack[m];
    state()->matrix.ptr = stack->m[stack->top];
}

static void s_teaPushMatrix_core(void) {
    struct matrix_stack_s *stack = &state()->matrix.stack[state()->matrix.mode - TEA_MODELVIEW];
    TEA_ASSERT(stack->top < MAX_MATRIX_STACK - 1, "matrix stack overflow");
    stack->top++;
    s_cloneMatrixd(stack->m[stack->top], state()->matrix.ptr);
    state()->matrix.ptr = stack->m[stack->top];
}

static void s_teaPopMatrix_core(void) {
    struct matrix_stack_s *stack = &state()->matrix.stack[state()->matrix.mode - TEA_MODELVIEW];
    TEA_ASSERT(stack->top > 0, "matrix stack underflow");
    stack->top--;
    state()->matrix.ptr = stack->m[stack->top];
}

static void s_teaLoadIdentity_core(void) {
    for (te_ubyte i = 0; i < 16; i++)
        state()->matrix.ptr[i] = 0.0f;

    state()->matrix.ptr[0] = 1.0f;
    state()->matrix.ptr[5] = 1.0f;
    state()->matrix.ptr[10] = 1.0f;
    state()->matrix.ptr[15] = 1.0f;
}

static void s_teaLoadMatrixd_core(const te_double *m) {
    TEA_ASSERT(m != NULL, "matrix is NULL");
    for (te_uint i = 0; i < 16; i++)
        state()->matrix.ptr[i] = m[i];
}

static void s_teaMultMatrixd_core(const te_double *m) {
    TEA_ASSERT(m != NULL, "matrix is NULL");
    s_multMatrixd(state()->matrix.ptr, m, state()->matrix.ptr);
}

static void s_teaTranslated_core(te_double x, te_double y, te_double z) {
    te_double m[16] = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        x, y, z, 1.0f
    };
    teaMultMatrixd(m);
}

static void s_teaScaled_core(te_double x, te_double y, te_double z) {
    te_double m[16] = {
        x, 0.0f, 0.0f, 0.0f,
        0.0f, y, 0.0f, 0.0f,
        0.0f, 0.0f, z, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
    teaMultMatrixd(m);
}

static void s_teaRotated_core(te_double angle, te_double x, te_double y, te_double z) {
    te_double c = cos(angle);
    te_double s = sin(angle);
    te_double nc = 1.0f - c;
    te_double len = x*x + y*y + z*z;
    if (len > 0.0) {
        te_double rlen = 1.0f / sqrt(len);
        x *= rlen;
        y *= rlen;
        z *= rlen;
    }
    te_double m[16] = {
        x*x*nc + c, x*y*nc - z*s, x*z*nc + y*s, 0.0f,
        x*y*nc + z*s, y*y*nc + c, y*z*nc - x*s, 0.0f,
        x*z*nc - y*s, y*z*nc + x*s, z*z*nc + c, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
    teaMultMatrixd(m);
}

static void s_teaLoadMatrixf_core(const te_float *m) {
    TEA_ASSERT(m != NULL, "matrix is NULL");
    for (te_uint i = 0; i < 16; i++)
        state()->matrix.ptr[i] = m[i];
}

static void s_teaMultMatrixf_core(const te_float *m) {
    TEA_ASSERT(m != NULL, "matrix is NULL");
    te_float n[16];
    for (te_uint i = 0; i < 16; i++)
        n[i] = state()->matrix.ptr[i];
    s_multMatrixf(n, m, n);
    for (te_uint i = 0; i < 16; i++)
        state()->matrix.ptr[i] = n[i];
}

static void s_teaTranslatef_core(te_float x, te_float y, te_float z) {
    te_float m[16] = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        x, y, z, 1.0f
    };
    teaMultMatrixf(m);
}

static void s_teaScalef_core(te_float x, te_float y, te_float z) {
    te_float m[16] = {
        x, 0.0f, 0.0f, 0.0f,
        0.0f, y, 0.0f, 0.0f,
        0.0f, 0.0f, z, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
    teaMultMatrixf(m);
}

static void s_teaRotatef_core(te_float angle, te_float x, te_float y, te_float z) {
    te_float c = cosf(angle);
    te_float s = sinf(angle);
    te_float nc = 1.0f - c;
    te_float len = x*x + y*y + z*z;
    if (len > 0.0f) {
        te_float rlen = 1.0f / sqrtf(len);
        x *= rlen;
        y *= rlen;
        z *= rlen;
    }
    te_float m[16] = {
        x*x*nc + c, x*y*nc - z*s, x*z*nc + y*s, 0.0f,
        x*y*nc + z*s, y*y*nc + c, y*z*nc - x*s, 0.0f,
        x*z*nc - y*s, y*z*nc + x*s, z*z*nc + c, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
    teaMultMatrixf(m);
}

static void s_teaOrtho_core(te_double left, te_double right, te_double bottom, te_double top, te_double zNear, te_double zFar) {
    te_double tx = -(right + left) / (right - left);
    te_double ty = -(top + bottom) / (top - bottom);
    te_double tz = -(zFar + zNear) / (zFar - zNear);
    te_double m[16] = {
        2.0f / (right - left), 0.0f, 0.0f, 0.0f,
        0.0f, 2.0f / (top - bottom), 0.0f, 0.0f,
        0.0f, 0.0f, -2.0f / (zFar - zNear), 0.0f,
        tx, ty, tz, 1.0f
    };
    teaMultMatrixd(m);
}

static void s_teaFrustum_core(te_double left, te_double right, te_double bottom, te_double top, te_double zNear, te_double zFar) {
    te_double tx = -(right + left) / (right - left);
    te_double ty = -(top + bottom) / (top - bottom);
    te_double m[16] = {
        2.0f * zNear / (right - left), 0.0f, 0.0f, 0.0f,
        0.0f, 2.0f * zNear / (top - bottom), 0.0f, 0.0f,
        tx, ty, -(zFar + zNear) / (zFar - zNear), -1.0f,
        0.0f, 0.0f, -2.0f * zFar * zNear / (zFar - zNear), 0.0f
    };
    teaMultMatrixd(m);
}

static void s_teaOrthof_core(te_float left, te_float right, te_float bottom, te_float top, te_float zNear, te_float zFar) {
    te_float tx = -(right + left) / (right - left);
    te_float ty = -(top + bottom) / (top - bottom);
    te_float tz = -(zFar + zNear) / (zFar - zNear);
    te_float m[16] = {
        2.0f / (right - left), 0.0f, 0.0f, 0.0f,
        0.0f, 2.0f / (top - bottom), 0.0f, 0.0f,
        0.0f, 0.0f, -2.0f / (zFar - zNear), 0.0f,
        tx, ty, tz, 1.0f
    };
    teaMultMatrixf(m);
}

static void s_teaFrustumf_core(te_float left, te_float right, te_float bottom, te_float top, te_float zNear, te_float zFar) {
    te_float tx = -(right + left) / (right - left);
    te_float ty = -(top + bottom) / (top - bottom);
    te_float m[16] = {
        2.0f * zNear / (right - left), 0.0f, 0.0f, 0.0f,
        0.0f, 2.0f * zNear / (top - bottom), 0.0f, 0.0f,
        tx, ty, -(zFar + zNear) / (zFar - zNear), -1.0f,
        0.0f, 0.0f, -2.0f * zFar * zNear / (zFar - zNear), 0.0f
    };
    teaMultMatrixf(m);
}

void teaPerspective(te_double fovy, te_double aspect, te_double zNear, te_double zFar) {
    te_double ymax = zNear * tan(fovy * TEA_PI / 360.0);
    te_double ymin = -ymax;
    te_double xmin = ymin * aspect;
    te_double xmax = ymax * aspect;
    te_double m[16] = {
        0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f
    };
    m[0] = (2.0f * zNear) / (xmax - xmin);
    m[5] = (2.0f * zNear) / (ymax - ymin);
    m[8] = (xmax + xmin) / (xmax - xmin);
    m[9] = (ymax + ymin) / (ymax - ymin);
    m[10] = -(zFar + zNear) / (zFar - zNear);
    m[11] = -1.0f;
    m[14] = -(2.0f * zFar * zNear) / (zFar - zNear);
    teaMultMatrixd(m);
}

void teaClear(void) {
    CALL_GL(Clear)(state()->clearFlags);
}

/* Texture functions */
te_texture_t teaTexture2D(const char *data, te_uint width, te_uint height, te_uint format) {
    te_texture_t texture = 0;
    CALL_GL(GenTextures)(1, &texture);
    CALL_GL(BindTexture)(TEA_TEXTURE_2D, texture);
    CALL_GL(TexImage2D)(TEA_TEXTURE_2D, 0, format, width, height, 0, format, TEA_UNSIGNED_BYTE, data);
    CALL_GL(TexParameteri)(TEA_TEXTURE_2D, TEA_TEXTURE_MAG_FILTER, TEA_NEAREST);
    CALL_GL(TexParameteri)(TEA_TEXTURE_2D, TEA_TEXTURE_MIN_FILTER, TEA_NEAREST);
    CALL_GL(TexParameteri)(TEA_TEXTURE_2D, TEA_TEXTURE_WRAP_S, TEA_CLAMP_TO_EDGE);
    CALL_GL(TexParameteri)(TEA_TEXTURE_2D, TEA_TEXTURE_WRAP_T, TEA_CLAMP_TO_EDGE);
    CALL_GL(BindTexture)(TEA_TEXTURE_2D, 0);
    return texture;
}

/* Buffer functions */
static void VertexPointer(te_uint size, te_uint type, te_uint stride, const te_void *pointer) {
    CALL_GL(VertexPointer)(size, type, stride, pointer);
}
static void ColorPointer(te_uint size, te_uint type, te_uint stride, const te_void *pointer) {
    CALL_GL(ColorPointer)(size, type, stride, pointer);
}
static void TexCoordPointer(te_uint size, te_uint type, te_uint stride, const te_void *pointer) {
    CALL_GL(TexCoordPointer)(size, type, stride, pointer);
}
static void NormalPointer(te_uint size, te_uint type, te_uint stride, const te_void *pointer) {
    CALL_GL(NormalPointer)(type, stride, pointer);
}

struct {
    te_ubyte size, stride;
    te_ushort type;
    te_uint client;
    void(*func)(te_uint, te_uint, te_uint, const te_void*);
} _attribInternal[] = {
    {2, 8, TEA_FLOAT, TEA_VERTEX_ARRAY, VertexPointer},
    {3, 12, TEA_FLOAT, TEA_VERTEX_ARRAY, VertexPointer},
    {4, 16, TEA_FLOAT, TEA_COLOR_ARRAY, ColorPointer},
    {2, 8, TEA_FLOAT, TEA_TEXTURE_COORD_ARRAY, TexCoordPointer},
    {3, 12, TEA_FLOAT, TEA_NORMAL_ARRAY, NormalPointer},
};

static te_buffer_t* s_getBuffer(te_uint target) {
    te_uint tg = target - TEA_ARRAY_BUFFER;
    TEA_ASSERT(tg < 2, "invalid target");
    return state()->vao->buffers[tg];
}

static te_buffer_t* s_getBufferSafe(te_uint target) {
    te_buffer_t *buffer = s_getBuffer(target);
    TEA_ASSERT(buffer != NULL, "invalid target");
    return buffer;
}

#if 0
static void _flushVBO(void);
static void _flushIBO(void);
typedef void(*BuffFlush)(void);
static BuffFlush buffFlushFuncs[] = {
    _flushVBO,
    _flushIBO,
};
#endif

typedef void(*SetBufferProc)(te_uint, te_buffer_t*);
static SetBufferProc s_setBuffer;

static void s_setBuffer_1(te_uint target, te_buffer_t *buffer) {
    if (!buffer)
        return;
    if (target == 0) {
        te_uint i;
        te_vao_format_t *format = &state()->vao->format;
        for (i = 0; i < format->count; i++) {
            te_vao_attrib_t *attrib = &format->attribs[i];
            _attribInternal[attrib->tag].func(attrib->size, attrib->type, format->stride, (te_void*)attrib->offset);
        }
    }
}
static void s_setBuffer_2(te_uint target, te_buffer_t *buffer) {
    if (!buffer)
        return;
    if (target == 0) {
        te_uint i;
        te_vao_format_t *format = &state()->vao->format;
        for (i = 0; i < format->count; i++) {
            te_vao_attrib_t *attrib = &format->attribs[i];
            CALL_GL(VertexAttribPointer)(i, attrib->size, attrib->type, TEA_FALSE, format->stride, (te_void*)attrib->offset);
        }
    }
}

static void s_setBuffer_3(te_uint target, te_buffer_t *buffer) {
}

te_buffer_t* teaBuffer(te_uint target, te_uint size, te_uint usage) {
    te_buffer_t *buffer = calloc(1, sizeof(te_buffer_t));
    TEA_ASSERT(buffer != NULL, "failed to allocate buffer");
    buffer->target = target;
    buffer->size = size;
    buffer->usage = usage;
    buffer->data = malloc(size);
    TEA_ASSERT(buffer->data != NULL, "failed to allocate buffer data");
    CALL_GL(GenBuffers)(1, &buffer->handle);
    CALL_GL(BindBuffer)(target, buffer->handle);
    CALL_GL(BufferData)(buffer->target, buffer->size, NULL, buffer->usage);
    CALL_GL(BindBuffer)(buffer->target, 0);
    return buffer;
}

void teaFreeBuffer(te_buffer_t *buffer) {
    if (!buffer) return;
    if (buffer->handle) {
        CALL_GL(DeleteBuffers)(1, &buffer->handle);
    }
    free(buffer->data);
    free(buffer);
}

void teaSeekBuffer(te_buffer_t *buffer, te_uint offset) {
    TEA_ASSERT(buffer != NULL, "invalid buffer");
    TEA_ASSERT(offset < buffer->size, "invalid offset");
    buffer->offset = offset;
}

void teaWriteBuffer(te_buffer_t *buffer, const void *data, te_uint size) {
    TEA_ASSERT(buffer != NULL, "invalid buffer");
    TEA_ASSERT(buffer->offset + size <= buffer->size, "full buffer");
    memcpy(buffer->data + buffer->offset, data, size);
    buffer->offset += size;
}

void teaReadBuffer(te_buffer_t *buffer, void *data, te_uint size) {
    TEA_ASSERT(buffer != NULL, "invalid buffer");
    size = size + buffer->offset > buffer->size ? buffer->size - buffer->offset : size;
    memcpy(data, buffer->data + buffer->offset, size);
}

void teaBindBuffer(te_uint target, te_buffer_t *buffer) {
    TEA_ASSERT(target == TEA_ARRAY_BUFFER || target == TEA_ELEMENT_ARRAY_BUFFER, "invalid buffer target");
    te_uint glbuf = buffer ? buffer->handle : 0;
    CALL_GL(BindBuffer)(target, glbuf);
    s_setBuffer(target-TEA_ARRAY_BUFFER, buffer);
    state()->vao->buffers[target - TEA_ARRAY_BUFFER] = buffer;
}

void teaResizeBuffer(te_uint target, te_uint size, te_uint usage) {
    te_buffer_t *buffer = s_getBufferSafe(target);
    TEA_ASSERT(buffer->handle, "buffer not initialized");
    if (buffer->size == size) return;
    TEA_ASSERT(buffer->size < size, "buffer size cannot be decreased");
    CALL_GL(BufferData)(target, size, NULL, usage);
    CALL_GL(BufferSubData)(target, 0, buffer->size, buffer->data);
    buffer->data = realloc(buffer->data, size);
    buffer->size = size;
}

void teaGrowBuffer(te_uint target) {
    te_buffer_t *buffer = s_getBufferSafe(target);
    TEA_ASSERT(buffer->handle, "buffer not initialized");
    te_uint nsize = (buffer->size)*2;
    CALL_GL(BufferData)(target, nsize, NULL, buffer->usage);
    CALL_GL(BufferSubData)(target, 0, buffer->size, buffer->data);
    buffer->data = realloc(buffer->data, nsize);
    buffer->size = nsize;
}

void teaFlushBuffer(te_uint target) {
    te_buffer_t *buffer = s_getBufferSafe(target);
    TEA_ASSERT(buffer->handle, "buffer not initialized");
    CALL_GL(BufferSubData)(target, 0, buffer->size, buffer->data);
}

void teaSendBuffer(te_uint target, te_uint size) {
    te_buffer_t *buffer = s_getBufferSafe(target);
    TEA_ASSERT(buffer->handle, "buffer not initialized");
    CALL_GL(BufferSubData)(target, 0, size, buffer->data);
}

void teaSendBufferRange(te_uint target, te_uint offset, te_uint size) {
    te_buffer_t *buffer = s_getBufferSafe(target);
    TEA_ASSERT(buffer->handle, "buffer not initialized");
    TEA_ASSERT(offset + size <= buffer->offset, "buffer range out of bounds");
    CALL_GL(BufferSubData)(target, 0, size, buffer->data + offset);
}

/* VAO functions */

static void _enableVAOFormat(te_vao_format_t *format);

void teaVAOFormat(te_vao_format_t *format) {
    memset(format, 0, sizeof(*format));
}

void teaVAOFormatAdd(te_vao_format_t *format, te_uint attr) {
    TEA_ASSERT(format->count < TEA_MAX_VERTEX_ATTRIBS, "too many attributes");
    te_vao_attrib_t *attrib = &format->attribs[format->count];
    attrib->tag = attr;
    attrib->type = _attribInternal[attr].type;
    attrib->size = _attribInternal[attr].size;
    attrib->stride = _attribInternal[attr].stride;
    attrib->offset = format->stride;

    format->count++;
    format->stride += attrib->stride;
}

static void _enableVAOFormat(te_vao_format_t *format) {
    te_uint i;
    if (gl()->extensions & TEA_GL_ARB_vertex_array_object ||
        gl()->extensions & TEA_GL_ARB_vertex_program) {
        for (i = 0; i < format->count; i++) {
            te_vao_attrib_t *attrib = &format->attribs[i];
            CALL_GL(EnableVertexAttribArray)(i);
            CALL_GL(VertexAttribPointer)(i, attrib->size, attrib->type, TEA_FALSE, format->stride, (void*)attrib->offset);
        }
    } else {
        for (i = 0; i < format->count; i++) {
            te_vao_attrib_t *attrib = &format->attribs[i];
            CALL_GL(EnableClientState)(_attribInternal[attrib->tag].client);
            _attribInternal[attrib->tag].func(attrib->size, attrib->type, format->stride, (te_void*)attrib->offset);
        }
    }
}

static void _disableVAOFormat(te_vao_format_t *format) {
    te_uint i;
    if (gl()->extensions & TEA_GL_ARB_vertex_array_object ||
        gl()->extensions & TEA_GL_ARB_vertex_program) {
        for (i = 0; i < format->count; i++)
            CALL_GL(DisableVertexAttribArray)(i);
    } else {
        for (i = 0; i < format->count; i++) {
            te_vao_attrib_t *attrib = &format->attribs[i];
            CALL_GL(DisableClientState)(_attribInternal[attrib->tag].client);
        }
    }
}

void teaBindVAOFormat(te_vao_format_t *format) {
    TEA_ASSERT(format != NULL, "invalid format");
    _disableVAOFormat(&state()->vao->format);
    _enableVAOFormat(format);
    memcpy(&state()->vao->format, format, sizeof(te_vao_format_t));
}

te_vao_t* teaVAO(void) {
    te_vao_t *vao = calloc(1, sizeof(te_vao_t));
    TEA_ASSERT(vao != NULL, "failed to allocate VAO");
    if (gl()->extensions & TEA_GL_ARB_vertex_array_object)
        CALL_GL(GenVertexArrays)(1, &vao->handle);
    return vao;
}

void teaFreeVAO(te_vao_t *vao) {
    if (!vao) return;
    if (vao->handle && gl()->extensions & TEA_GL_ARB_vertex_array_object)
        CALL_GL(DeleteVertexArrays)(1, &vao->handle);
    free(vao);
}



static void s_teaBindVAO_1(te_vao_t *vao) {
    vao = vao ? vao : &s_defaultVao;
    te_vao_format_t *format = &state()->vao->format;
    for (te_uint i = 0; i < format->count; i++) {
        te_vao_attrib_t *attrib = &format->attribs[i];
        CALL_GL(DisableClientState)(_attribInternal[attrib->tag].client);
    }
    format = &vao->format;
    for (te_uint i = 0; i < format->count; i++) {
        te_vao_attrib_t *attrib = &format->attribs[i];
        CALL_GL(EnableClientState)(_attribInternal[attrib->tag].client);
    }
    state()->vao = vao;
}

static void s_teaBindVAO_2(te_vao_t *vao) {
    vao = vao ? vao : &s_defaultVao;
    for (te_uint i = 0; i < state()->vao->format.count; i++) {
        CALL_GL(DisableVertexAttribArray)(i);
    }

    for (te_uint i = 0; i < vao->format.count; i++)
        CALL_GL(EnableVertexAttribArray)(i);

    for (te_uint i = 0; i < 2; i++) {
        if (vao->buffers[i])
            CALL_GL(BindBuffer)(TEA_ARRAY_BUFFER, vao->buffers[i]->handle);
        else
            CALL_GL(BindBuffer)(TEA_ARRAY_BUFFER, 0);
    }
    if (vao->buffers[0]) {
        te_vao_format_t *format = &vao->format;
        for (te_uint i = 0; i < format->count; i++) {
            te_vao_attrib_t *attrib = &format->attribs[i];
            CALL_GL(VertexAttribPointer)(i, attrib->size, attrib->type, TEA_FALSE, format->stride, (void*)attrib->offset);
        }
    }
    state()->vao = vao;
}

static void s_teaBindVAO_3(te_vao_t *vao) {
    vao = vao ? vao : &s_defaultVao;
    CALL_GL(BindVertexArray)(vao->handle);
    state()->vao = vao;
}

void teaVertexAttribPointer(te_uint attr, te_uint size, te_uint type, te_uint stride, te_uint offset) {
    TEA_ASSERT(attr < TEA_MAX_VERTEX_ATTRIBS, "invalid attribute");
    TEA_ASSERT(size < 5, "invalid size");
    CALL_GL(VertexAttribPointer)(attr, size, type, TEA_FALSE, stride, (void*)(offset));
}

/* Shader functions */
//#include <GL/gl.h>
#define TEA_COMPILE_STATUS 0x8B81
#define TEA_LINK_STATUS 0x8B82
static te_uint s_shaderCompile(te_uint type, const char *source) {
    te_uint shader = CALL_GL(CreateShader)(type);
    CALL_GL(ShaderSource)(shader, 1, &source, NULL);
    CALL_GL(CompileShader)(shader);
    te_uint status;
    CALL_GL(GetShaderiv)(shader, TEA_COMPILE_STATUS, &status);
    if (status == 0) {
        char log[1024];
        CALL_GL(GetShaderInfoLog)(shader, 1024, NULL, log);
        TEA_ASSERT(0, "shader compile error: %s", log);
    }
    return shader;
}
te_shader_t teaShader(const char *fragSrc, const char *vertSrc) {
    TEA_ASSERT(fragSrc != NULL, "fragment shader source is null");
    TEA_ASSERT(vertSrc != NULL, "vertex shader source is null");
    te_shader_t shader = CALL_GL(CreateProgram)();
    te_uint fragShader = s_shaderCompile(TEA_FRAGMENT_SHADER, fragSrc);
    te_uint vertShader = s_shaderCompile(TEA_VERTEX_SHADER, vertSrc);
    CALL_GL(AttachShader)(shader, fragShader);
    CALL_GL(AttachShader)(shader, vertShader);
    CALL_GL(LinkProgram)(shader);
    te_uint status;
    CALL_GL(GetProgramiv)(shader, TEA_LINK_STATUS, &status);
    if (status == 0) {
        char log[1024];
        CALL_GL(GetProgramInfoLog)(shader, 1024, NULL, log);
        TEA_ASSERT(0, "shader link error: %s", log);
    }
    CALL_GL(DeleteShader)(fragShader);
    CALL_GL(DeleteShader)(vertShader);
    return shader;
}

void teaFreeShader(te_shader_t shader) {
    if (!shader) return;
    CALL_GL(DeleteProgram)(shader);
}

void teaUseShader(te_shader_t shader) {
    CALL_GL(UseProgram)(shader);
}

/* Debug */
#include <stdarg.h>
void teaAbort(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
    va_end(args);
    exit(EXIT_FAILURE);
}

/* Loader */
static te_bool loadTeaFunctions(void);

TeaClearColorProc teaClearColor = NULL;
TeaClearDepthProc teaClearDepth = NULL;
TeaScissorProc teaScissor = NULL;
TeaEnableProc teaEnable = NULL;
TeaDisableProc teaDisable = NULL;

TeaBlendFuncProc teaBlendFunc = NULL;
TeaBlendFuncSeparateProc teaBlendFuncSeparate = NULL;
TeaBlendEquationProc teaBlendEquation = NULL;
TeaBlendEquationSeparateProc teaBlendEquationSeparate = NULL;
TeaBlendColorProc teaBlendColor = NULL;

TeaDepthFuncProc teaDepthFunc = NULL;
TeaDepthMaskProc teaDepthMask = NULL;
TeaDepthRangeProc teaDepthRange = NULL;

TeaStencilFuncProc teaStencilFunc = NULL;
TeaStencilMaskProc teaStencilMask = NULL;
TeaStencilOpProc teaStencilOp = NULL;

/* Texture */
TeaBindTextureProc teaBindTexture = NULL;

/* Framebuffer */
TeaBindFBOProc teaBindFBO = NULL;

/* Transformations */
TeaMatrixModeProc teaMatrixMode = NULL;
TeaPushMatrixProc teaPushMatrix = NULL;
TeaPopMatrixProc teaPopMatrix = NULL;
TeaLoadIdentityProc teaLoadIdentity = NULL;

#define TEA_MATRIX_IMPL_X(X)\
TeaLoadMatrix##X##Proc teaLoadMatrix##X = NULL;\
TeaMultMatrix##X##Proc teaMultMatrix##X = NULL;\
TeaTranslate##X##Proc teaTranslate##X = NULL;\
TeaScale##X##Proc teaScale##X = NULL;\
TeaRotate##X##Proc teaRotate##X = NULL;\
TeaLoadTransposeMatrix##X##Proc teaLoadTransposeMatrix##X = NULL;\
TeaMultTransposeMatrix##X##Proc teaMultTransposeMatrix##X = NULL;\
TeaOrtho##X##Proc teaOrtho##X = NULL;\
TeaFrustum##X##Proc teaFrustum##X = NULL

TEA_MATRIX_IMPL_X(f);
TEA_MATRIX_IMPL_X(d);
TeaViewportProc teaViewport = NULL;

/* Vertex array object */
TeaBindVAOProc teaBindVAO = NULL;

/* Shaders */
TeaGetUniformLocationProc teaGetUniformLocation = NULL;

#define TEA_UNIFORM_IMPL_X(X)\
TeaUniform1##X##Proc teaUniform1##X = NULL;\
TeaUniform2##X##Proc teaUniform2##X = NULL;\
TeaUniform3##X##Proc teaUniform3##X = NULL;\
TeaUniform4##X##Proc teaUniform4##X = NULL;\
TeaUniform##X##vProc teaUniform1##X##v = NULL;\
TeaUniform##X##vProc teaUniform2##X##v = NULL;\
TeaUniform##X##vProc teaUniform3##X##v = NULL;\
TeaUniform##X##vProc teaUniform4##X##v = NULL

TEA_UNIFORM_IMPL_X(f);
TEA_UNIFORM_IMPL_X(i);
TeaUniformMatrixfvProc teaUniformMatrix2fv = NULL;
TeaUniformMatrixfvProc teaUniformMatrix3fv = NULL;
TeaUniformMatrixfvProc teaUniformMatrix4fv = NULL;
//TEA_UNIFORM_IMPL_X(ui);

#if !defined(__APPLE__) && !defined(__HAIKU__)
typedef void*(*TeaGetProcAddressPtr)(const char*);
TeaGetProcAddressPtr teaGetProcAddress = NULL;
#endif

te_bool loadLibGL(void) {
#if _WIN32
    openglSym = LoadLibrary("opengl32.dll");
    TEA_ASSERT(openglSym != NULL, "Failed to load OpenGL32.dll");
#else
#ifdef __APPLE__
    const char *names[] = {
        "../Frameworks/OpenGL.framework/OpenGL",
        "/Library/Frameworks/OpenGL.framework/OpenGL",
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
    te_uint index;
    for (index = 0; names[index] != NULL; index++) {
        openglSym = dlopen(names[index], RTLD_LAZY | RTLD_GLOBAL);
        if (openglSym != NULL) {
#if defined(__APPLE__) || defined(__HAIKU__)
            return TEA_TRUE;
#else
        teaGetProcAddress = (TeaGetProcAddressPtr)dlsym(openglSym, "glXGetProcAddress");
        return teaGetProcAddress != NULL;
#endif
        }
    }
#endif
    return TEA_FALSE;
}

void closeLibGL(void) {
    if (openglSym != NULL) {
#if _WIN32
        FreeLibrary(openglSym);
#else
        dlclose(openglSym);
#endif
        openglSym = NULL;
    }
}

void* getProcGL(const char *proc) {
    void *sym = NULL;
    if (openglSym == NULL) return sym;
#if !defined(__APPLE__) && !defined(__HAIKU__)
    if (teaGetProcAddress != NULL) {
        sym = teaGetProcAddress(proc);
    }
#endif
    if (sym == NULL) {
#if defined(_WIN32) || defined(__CYGWIN__)
        sym = (void*)GetProcAddress(openglSym, proc);
#else
        sym = dlsym(openglSym, proc);
#endif
    }
    return sym;
}

void setupGLProcs(void) {
    GET_GL(GetString) = getProcGL("glGetString");
    GET_GL(GetStringi) = getProcGL("glGetStringi");

    const char *version = (const char*)CALL_GL(GetString)(TEA_GL_VERSION);
    const char *glsl = (const char*)CALL_GL(GetString)(TEA_GL_SHADING_LANGUAGE_VERSION);
    TEA_ASSERT(version != NULL, "Failed to get OpenGL version");
    const char *prefixes[] = {
        "OpenGL ES-CM ",
        "OpenGL ES-CL ",
        "OpenGL ES ",
        NULL,
    };

    char *ver = version;
    for (te_uint i = 0; prefixes[i] != NULL; i++) {
        if (strncmp(ver, prefixes[i], strlen(prefixes[i])) == 0) {
            ver += strlen(prefixes[i]);
            gl()->version.es = TEA_TRUE;
            break;
        }
    }
    glMaxMajor = ver[0] - '0';
    glMaxMinor = ver[2] - '0';
    if (gl()->version.major == 0) {
        gl()->version.major = glMaxMajor;
        gl()->version.minor = glMaxMinor;
    }

    fprintf(stderr, "OpenGL version: %s\n", version);
    fprintf(stderr, "OpenGL shading language version: %s\n", glsl);
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
        { TEA_GL_BindFramebuffer, { "glBindFTEA_GL_BindFramebuffer", "glBindFTEA_GL_BindFramebufferEXT", NULL }},
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

    teaLoadProcs((te_proc_t*)glBaseProcs, TEA_PROC_OVERRIDE);

    if (GET_GL(BindBuffer)) gl()->extensions |= TEA_GL_ARB_vertex_buffer_object;
    if (GET_GL(CreateShader)) gl()->extensions |= TEA_GL_ARB_shader_objects;
    if (GET_GL(VertexAttribPointer)) gl()->extensions |= TEA_GL_ARB_vertex_program;
    if (GET_GL(GenVertexArrays)) gl()->extensions |= TEA_GL_ARB_vertex_array_object;
    loadTeaFunctions();
}

te_bool loadTeaFunctions(void) {
    TEA_PROC(ClearColor);
    TEA_PROC(ClearDepth);

    TEA_PROC(Scissor);
    TEA_PROC(Enable);
    TEA_PROC(Disable);

    TEA_PROC(BlendFunc);

    TEA_PROC(DepthFunc);
    TEA_PROC(DepthMask);
    TEA_PROC(DepthRange);

    /* Transformation */
    TEA_PROC(MatrixMode);
    TEA_PROC(PushMatrix);
    TEA_PROC(PopMatrix);
    TEA_PROC(LoadIdentity);

    TEA_PROC(LoadMatrixf);
    TEA_PROC(LoadMatrixd);
    TEA_PROC(MultMatrixf);
    TEA_PROC(MultMatrixd);

    TEA_PROC(Translatef);
    TEA_PROC(Translated);
    TEA_PROC(Scalef);
    TEA_PROC(Scaled);
    TEA_PROC(Rotatef);
    TEA_PROC(Rotated);

    TEA_PROC(LoadTransposeMatrixf);
    TEA_PROC(LoadTransposeMatrixd);
    TEA_PROC(MultTransposeMatrixf);
    TEA_PROC(MultTransposeMatrixd);

    if (gl()->extensions & TEA_GL_ARB_vertex_array_object) {
        fprintf(stderr, "Using vertex array object\n");
        teaBindVAO = (TeaBindVAOProc)s_teaBindVAO_3;
        s_setBuffer = s_setBuffer_3;
    } else if (gl()->extensions & TEA_GL_ARB_vertex_program) {
        fprintf(stderr, "Using vertex program\n");
        teaBindVAO = (TeaBindVAOProc)s_teaBindVAO_2;
        s_setBuffer = s_setBuffer_2;
    } else {
        fprintf(stderr, "Using legacy vertex array object\n");
        teaBindVAO = (TeaBindVAOProc)s_teaBindVAO_1;
        s_setBuffer = s_setBuffer_1;
    }

    TEA_PROC(Ortho);
    TEA_PROC(Frustum);
    teaOrthod = GET_GL(Ortho);
    teaOrthof = GET_GL(Orthof);

    if (!gl()->version.es)
        teaOrthof = _teaOrthofTod;
    else
        teaOrthod = _teaOrthodTof;

    teaFrustumd = GET_GL(Frustum);
    teaFrustumf = GET_GL(Frustumf);

    TEA_PROC(Viewport);

    /* Stencil */

    /* Texture mapping */
    TEA_PROC(BindTexture);

    /* Framebuffers */
    teaBindFBO = GET_GL(BindFramebuffer);

    /* Shaders */
    if (gl()->extensions & TEA_GL_ARB_shader_objects) {
        fprintf(stderr, "Using shader objects\n");
        TEA_PROC(GetUniformLocation);
        TEA_PROC(Uniform1f);
        TEA_PROC(Uniform2f);
        TEA_PROC(Uniform3f);
        TEA_PROC(Uniform4f);
        TEA_PROC(Uniform1i);
        TEA_PROC(Uniform2i);
        TEA_PROC(Uniform3i);
        TEA_PROC(Uniform4i);
        TEA_PROC(Uniform1fv);
        TEA_PROC(Uniform2fv);
        TEA_PROC(Uniform3fv);
        TEA_PROC(Uniform4fv);
        TEA_PROC(Uniform1iv);
        TEA_PROC(Uniform2iv);
        TEA_PROC(Uniform3iv);
        TEA_PROC(Uniform4iv);
        TEA_PROC(UniformMatrix2fv);
        TEA_PROC(UniformMatrix3fv);
        TEA_PROC(UniformMatrix4fv);

        teaMatrixMode = s_teaMatrixMode_core;
        teaPushMatrix = s_teaPushMatrix_core;
        teaPopMatrix = s_teaPopMatrix_core;
        teaLoadIdentity = s_teaLoadIdentity_core;
        teaLoadMatrixf = s_teaLoadMatrixf_core;
        teaLoadMatrixd = s_teaLoadMatrixd_core;
        teaMultMatrixf = s_teaMultMatrixf_core;
        teaMultMatrixd = s_teaMultMatrixd_core;
        teaTranslatef = s_teaTranslatef_core;
        teaTranslated = s_teaTranslated_core;
        teaScalef = s_teaScalef_core;
        teaScaled = s_teaScaled_core;
        teaRotatef = s_teaRotatef_core;
        teaRotated = s_teaRotated_core;
        teaOrtho = s_teaOrtho_core;
        teaFrustum = s_teaFrustum_core;
        teaOrthof = s_teaOrthof_core;
        teaFrustumf = s_teaFrustumf_core;
        //teaLoadTransposeMatrixf = s_teaLoadTransposeMatrixf_core;
        //teaLoadTransposeMatrixd = s_teaLoadTransposeMatrixd_core;
        //teaMultTransposeMatrixf = s_teaMultTransposeMatrixf_core;
        //teaMultTransposeMatrixd = s_teaMultTransposeMatrixd_core;
    }

    return TEA_TRUE;
}

te_bool teaLoadProcs(te_proc_t *procs, te_uint flags) {
    te_proc_t *proc = procs;
    while (proc->names[0]) {
        if (!gl()->procs[proc->tag] || (flags & TEA_PROC_OVERRIDE)) {
            te_uint i = 0;
            te_byte **names = proc->names;
            while (names[i] &&  i < 3) {
                if ((gl()->procs[proc->tag] = getProcGL(names[i])))
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

void* teaGetProc(te_uint tag) {
    return gl()->procs[tag % TEA_GL_PROC_COUNT];
}