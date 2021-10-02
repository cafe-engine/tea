#ifndef _TEA_H
#define _TEA_H

#define TEAPI extern
#define TEA_VER "0.1.0"

#define TEA_OK 0
#define TEA_ERR -1

#define TEA_ASSERT(expr, ...)\
    if (!(expr)) {\
	fprintf(stderr, "Assertion failed at '%s':%d in %s: ", __PRETTY_FUNCTION__, __LINE__, __FILE__);\
	teaAbort(__VA_ARGS__);\
    }

#define TEA_PI 3.14159265
#define TEA_DEG2RAD(a) ((a) * TEA_PI / 180.0)
#define TEA_RAD2DEG(a) ((a) * 180.0 / TEA_PI)
#define TEA_MAX(a, b) ((a) > (b) ? (a) : (b))
#define TEA_MIN(a, b) ((a) < (b) ? (a) : (b))

/* Boolean */
#define TEA_TRUE 1
#define TEA_FALSE 0

/* Data types */
#define TEA_BYTE           0x1400
#define TEA_UNSIGNED_BYTE  0x1401
#define TEA_SHORT          0x1402
#define TEA_UNSIGNED_SHORT 0x1403
#define TEA_INT            0x1404
#define TEA_UNSIGNED_INT   0x1405
#define TEA_FLOAT          0x1406
#define TEA_2_BYTES        0x1407
#define TEA_3_BYTES        0x1408
#define TEA_4_BYTES        0x1409
#define TEA_DOUBLE         0x140A

/* Primitives */
#define TEA_POINTS         0x0000
#define TEA_LINES          0x0001
#define TEA_LINE_LOOP      0x0002
#define TEA_LINE_STRIP     0x0003
#define TEA_TRIANGLES      0x0004
#define TEA_TRIANGLE_STRIP 0x0005
#define TEA_TRIANGLE_FAN   0x0006
#define TEA_QUADS          0x0007
#define TEA_QUAD_STRIP     0x0008
#define TEA_POLYGON        0x0009

/* Matrix mode */
#define TEA_MATRIX_MODE 0x0BA0
#define TEA_MODELVIEW   0x1700
#define TEA_PROJECTION  0x1701
#define TEA_TEXTURE     0x1702

/* Polygon */
#define TEA_POINT 0x1B00
#define TEA_LINE  0x1B01
#define TEA_FILL  0x1B02
#define TEA_CW    0x0900
#define TEA_CCW   0x0901
#define TEA_FRONT 0x0404
#define TEA_BACK  0x0405
#define TEA_EDGE_FLAG      0x0B43
#define TEA_CULL_FACE      0x0B44
#define TEA_CULL_FACE_MODE 0x0B45
#define TEA_FRONT_FACE     0x0B46

/* Depth buffer */
#define TEA_NEVER         0x0200
#define TEA_LESS          0x0201
#define TEA_EQUAL         0x0202
#define TEA_LEQUAL        0x0203
#define TEA_GREATER       0x0204
#define TEA_NOTEQUAL      0x0205
#define TEA_GEQUAL        0x0206
#define TEA_ALWAYS        0x0207
#define TEA_DEPTH_TEST    0x0B71

/* Blending */
#define TEA_BLEND         0x0BE2
#define TEA_BLEND_SRC     0x0BE1
#define TEA_BLEND_DST     0x0BE0
#define TEA_ZERO          0x0
#define TEA_ONE           0x1
#define TEA_SRC_COLOR     0x0300
#define TEA_ONE_MINUS_SRC_COLOR 0x0301
#define TEA_SRC_ALPHA     0x0302
#define TEA_ONE_MINUS_SRC_ALPHA 0x0303
#define TEA_DST_ALPHA     0x0304
#define TEA_ONE_MINUS_DST_ALPHA 0x0305
#define TEA_DST_COLOR     0x0306
#define TEA_ONE_MINUS_DST_COLOR 0x0307
#define TEA_SRC_ALPHA_SATURATE 0x0308

/* Buffers, Pixels Drawing/Reading */
#define TEA_NONE          0x0
#define TEA_LEFT          0x0406
#define TEA_RIGHT         0x0407
#define TEA_FRONT_LEFT    0x0400
#define TEA_FRONT_RIGHT   0x0401
#define TEA_BACK_LEFT     0x0402
#define TEA_BACK_RIGHT    0x0403
#define TEA_RED          0x1903
#define TEA_GREEN        0x1904
#define TEA_BLUE         0x1905
#define TEA_ALPHA        0x1906
#define TEA_LUMINANCE    0x1909
#define TEA_LUMINANCE_ALPHA 0x190A

#define TEA_READ_BUFFER  0x0C02
#define TEA_DRAW_BUFFER  0x0C01
#define TEA_DOUBLEBUFFER 0x0C32

#define TEA_STEREO      0x0C33
#define TEA_BITMAP      0x1A00
#define TEA_COLOR       0x1800
#define TEA_DEPTH       0x1801
#define TEA_STENCIL     0x1802
#define TEA_DITHER      0x0BD0
#define TEA_RGB         0x1907
#define TEA_RGBA        0x1908

/* bgra */
#define TEA_BGR  0x80E0
#define TEA_BGRA 0x80E1

/* Clear buffer bits */
#define TEA_DEPTH_BUFFER_BIT   0x00000100
#define TEA_ACCUM_BUFFER_BIT   0x00000200
#define TEA_STENCIL_BUFFER_BIT 0x00000400
#define TEA_COLOR_BUFFER_BIT   0x00004000

/* Texture mapping */
#define TEA_TEXTURE_1D				0x0DE0
#define TEA_TEXTURE_2D				0x0DE1
#define TEA_TEXTURE_WRAP_S			0x2802
#define TEA_TEXTURE_WRAP_T			0x2803
#define TEA_TEXTURE_MAG_FILTER			0x2800
#define TEA_TEXTURE_MIN_FILTER			0x2801
#define TEA_TEXTURE_ENV_COLOR			0x2201
#define TEA_TEXTURE_GEN_S			0x0C60
#define TEA_TEXTURE_GEN_T			0x0C61
#define TEA_TEXTURE_GEN_R			0x0C62
#define TEA_TEXTURE_GEN_Q			0x0C63
#define TEA_TEXTURE_GEN_MODE			0x2500
#define TEA_TEXTURE_BORDER_COLOR			0x1004
#define TEA_TEXTURE_WIDTH			0x1000
#define TEA_TEXTURE_HEIGHT			0x1001
#define TEA_TEXTURE_BORDER			0x1005
#define TEA_TEXTURE_COMPONENTS			0x1003
#define TEA_TEXTURE_RED_SIZE			0x805C
#define TEA_TEXTURE_GREEN_SIZE			0x805D
#define TEA_TEXTURE_BLUE_SIZE			0x805E
#define TEA_TEXTURE_ALPHA_SIZE			0x805F
#define TEA_TEXTURE_LUMINANCE_SIZE		0x8060
#define TEA_TEXTURE_INTENSITY_SIZE		0x8061
#define TEA_NEAREST_MIPMAP_NEAREST		0x2700
#define TEA_NEAREST_MIPMAP_LINEAR		0x2702
#define TEA_LINEAR_MIPMAP_NEAREST		0x2701
#define TEA_LINEAR_MIPMAP_LINEAR			0x2703
#define TEA_OBJECT_LINEAR			0x2401
#define TEA_OBJECT_PLANE				0x2501
#define TEA_EYE_LINEAR				0x2400
#define TEA_EYE_PLANE				0x2502
#define TEA_SPHERE_MAP				0x2402
#define TEA_DECAL				0x2101
#define TEA_MODULATE				0x2100
#define TEA_NEAREST				0x2600
#define TEA_REPEAT				0x2901
#define TEA_CLAMP				0x2900
#define TEA_S					0x2000
#define TEA_T					0x2001
#define TEA_R					0x2002
#define TEA_Q					0x2003

#define TEA_CLAMP_TO_EDGE			0x812F /* 1.2 */
#define TEA_CLAMP_TO_BORDER			0x812D /* 1.3 */

/* Texture 3D */
#define TEA_TEXUTRE_3D     0x806F
#define TEA_TEXTURE_DEPTH  0x8071
#define TEA_TEXTURE_WRAP_R 0x8072

/* Cube map texture */
#define TEA_TEXTURE_CUBE_MAP            0x8513
#define TEA_TEXTURE_CUBE_MAP_POSITIVE_X 0x8515
#define TEA_TEXTURE_CUBE_MAP_NEGATIVE_X 0x8516
#define TEA_TEXTURE_CUBE_MAP_POSITIVE_Y 0x8517
#define TEA_TEXTURE_CUBE_MAP_NEGATIVE_Y 0x8518
#define TEA_TEXTURE_CUBE_MAP_POSITIVE_Z 0x8519
#define TEA_TEXTURE_CUBE_MAP_NEGATIVE_Z 0x851A
#define TEA_MAX_CUBE_MAP_TEXTURE_SIZE   0x851C

/* Texture array */
#define TEA_TEXTURE_1D_ARRAY 0x8C18
#define TEA_TEXTURE_2D_ARRAY 0x8C1A

/* Multitexture */
#define TEA_TEXTURE0              0x84C0 /* use "TEA_TEXTURE0 + i" for more */
#define TEA_ACTIVE_TEXTURE        0x84E0
#define TEA_MAX_TEXTURE_UNITS     0x84E1

/* Vertex buffer */
#define TEA_ARRAY_BUFFER                 0x8892
#define TEA_ARRAY_BUFFER_BINDING         0x8894
#define TEA_ELEMENT_ARRAY_BUFFER         0x8893
#define TEA_ELEMENT_ARRAY_BUFFER_BINDING 0x8895
#define TEA_VERTEX_ARRAY_BUFFER_BINDING  0x8896
#define TEA_NORMAL_ARRAY_BUFFER_BINDING  0x8897
#define TEA_COLOR_ARRAY_BUFFER_BINDING   0x8898
#define TEA_TEXTURE_COORD_ARRAY_BUFFER_BINDING 0x889A
#define TEA_STATIC_DRAW                  0x88E4
#define TEA_DYNAMIC_DRAW                 0x88E8
#define TEA_BUFFER_SIZE                  0x8764
#define TEA_BUFFER_USAGE                 0x8765
#define TEA_READ_ONLY                    0x88B8
#define TEA_WRITE_ONLY                   0x88B9
#define TEA_READ_WRITE                   0x88BA

#define TEA_BUFFER_ACCESS               0x88BB
#define TEA_BUFFER_MAPPED               0x88BC
#define TEA_BUFFER_MAP_POINTER          0x88BD
#define TEA_STREAM_DRAW                 0x88E0
#define TEA_STREAM_READ                 0x88E1
#define TEA_STREAM_COPY                 0x88E2
#define TEA_STATIC_DRAW                 0x88E4
#define TEA_STATIC_READ                 0x88E5
#define TEA_STATIC_COPY                 0x88E6
#define TEA_DYNAMIC_DRAW                0x88E8
#define TEA_DYNAMIC_READ                0x88E9
#define TEA_DYNAMIC_COPY                0x88EA

/* Shader */
#define TEA_FRAGMENT_SHADER   0x8B30
#define TEA_VERTEX_SHADER     0x8B31

/* Vertex array */
#define TEA_VERTEX_ARRAY 0x8074
#define TEA_NORMAL_ARRAY 0x8075
#define TEA_COLOR_ARRAY  0x8076
#define TEA_INDEX_ARRAY  0x8077
#define TEA_TEXTURE_COORD_ARRAY 0x8078

typedef unsigned char te_bool;

typedef char te_byte;
typedef unsigned char te_ubyte;
typedef short te_short;
typedef unsigned short te_ushort;
typedef int te_int;
typedef unsigned int te_uint;
typedef float te_float;
typedef double te_double;
typedef void te_void;

typedef struct Tea Tea;
typedef struct {
    te_uint flags;
    char glslVersion[32];
    te_bool glES;
    te_ubyte glMag, glMin;
    te_uint vboMode, vboSize;
    te_uint iboMode, iboSize;
} te_config_t;

typedef unsigned int te_texture_t;

typedef struct te_vao_s te_vao_t;
typedef struct te_vao_format_s te_vao_format_t;
typedef struct {
    te_ubyte tag;
    te_ubyte offset;
    te_ubyte stride, size;
    te_uint type;
} te_vao_attrib_t;

#define TEA_MAX_VERTEX_ATTRIBS 32
struct te_vao_format_s {
    te_ubyte count;
    te_uint stride;
    te_vao_attrib_t attribs[TEA_MAX_VERTEX_ATTRIBS];
};

typedef struct te_buffer_s te_buffer_t;
typedef te_buffer_t te_vbo_t;
typedef te_buffer_t te_ibo_t;

typedef unsigned int te_fbo_t;
typedef unsigned int te_rbo_t;

typedef unsigned int te_shader_t;
typedef unsigned int te_program_t;

TEAPI te_config_t teaConfig(const char *glslVersion);
TEAPI te_int teaInit(te_config_t *config);
TEAPI void teaQuit(void);

/* Immediate mode */
TEAPI void teaBegin(te_uint mode);
TEAPI void teaEnd(void);

TEAPI void teaDraw(te_uint mode, te_uint count);

TEAPI void teaVertex2f(te_float x, te_float y);
TEAPI void teaVertex3f(te_float x, te_float y, te_float z);
TEAPI void teaVertex4f(te_float x, te_float y, te_float z, te_float w);
TEAPI void teaVertex2fv(te_float *v);
TEAPI void teaVertex3fv(te_float *v);
TEAPI void teaVertex4fv(te_float *v);

TEAPI void teaTexCoord2f(te_float x, te_float y);
TEAPI void teaTexCoord2fv(te_float *v);

TEAPI void teaColor3f(te_float r, te_float g, te_float b);
TEAPI void teaColor3ub(te_ubyte r, te_ubyte g, te_ubyte b);
TEAPI void teaColor4f(te_float r, te_float g, te_float b, te_float a);
TEAPI void teaColor4ub(te_ubyte r, te_ubyte g, te_ubyte b, te_ubyte a);
TEAPI void teaColor3fv(te_float *v);
TEAPI void teaColor3ubv(te_ubyte *v);
TEAPI void teaColor4fv(te_float *v);
TEAPI void teaColor4ubv(te_ubyte *v);

TEAPI void teaNormal3f(te_float x, te_float y, te_float z);
TEAPI void teaNormal3fv(te_float *v);

/* Transforming */
#define teaLoadMatrix teaLoadMatrixf
#define teaLoadTransposeMatrix teaLoadTransposeMatrixf
#define teaMultMatrix teaMultMatrixf
#define teaMultTransposeMatrix teaMultTransposeMatrixf
#define teaTranslate teaTranslatef
#define teaScale teaScalef
#define teaRotate teaRotatef

typedef void(*TeaPushMatrixProc)(void);
typedef void(*TeaPopMatrixProc)(void);
typedef void(*TeaMatrixModeProc)(te_uint mode);
typedef void(*TeaLoadIdentityProc)(void);

TEAPI TeaPushMatrixProc teaPushMatrix;
TEAPI TeaPopMatrixProc teaPopMatrix;
TEAPI TeaMatrixModeProc teaMatrixMode;
TEAPI TeaLoadIdentityProc teaLoadIdentity;

#define TEA_MATRIX_X(X, T)\
typedef void(*TeaLoadMatrix##X##Proc)(const T*);\
typedef void(*TeaLoadTransposeMatrix##X##Proc)(const T*);\
typedef void(*TeaMultMatrix##X##Proc)(const T*);\
typedef void(*TeaMultTransposeMatrix##X##Proc)(const T*);\
typedef void(*TeaTranslate##X##Proc)(T x, T y, T z);\
typedef void(*TeaScale##X##Proc)(T x, T y, T z);\
typedef void(*TeaRotate##X##Proc)(T angle, T x, T y, T z);\
typedef void(*TeaOrtho##X##Proc)(T left, T right, T bottom, T top, T zNear, T zFar);\
typedef void(*TeaFrustum##X##Proc)(T left, T right, T bottom, T top, T zNear, T zFar);\
TEAPI TeaLoadMatrix##X##Proc teaLoadMatrix##X;\
TEAPI TeaLoadTransposeMatrix##X##Proc teaLoadTransposeMatrix##X;\
TEAPI TeaMultMatrix##X##Proc teaMultMatrix##X;\
TEAPI TeaMultTransposeMatrix##X##Proc teaMultTransposeMatrix##X;\
TEAPI TeaTranslate##X##Proc teaTranslate##X;\
TEAPI TeaScale##X##Proc teaScale##X;\
TEAPI TeaRotate##X##Proc teaRotate##X;\
TEAPI TeaOrtho##X##Proc teaOrtho##X;\
TEAPI TeaFrustum##X##Proc teaFrustum##X

TEA_MATRIX_X(f, te_float);
TEA_MATRIX_X(d, te_double);

#define teaOrtho teaOrthod
#define teaFrustum teaFrustumd
TEAPI void teaPerspective(te_double fovy, te_double aspect, te_double zNear, te_double zFar);
typedef void(*TeaViewportProc)(te_int x, te_int y, te_int width, te_int height);
TEAPI TeaViewportProc teaViewport;

/* Clear */
typedef void(*TeaClearColorProc)(te_float r, te_float g, te_float b, te_float a);
typedef void(*TeaClearDepthProc)(te_float depth);
TEAPI TeaClearColorProc teaClearColor;
TEAPI TeaClearDepthProc teaClearDepth;
TEAPI void teaClearMask(te_uint mask);
TEAPI void teaClear(void);

typedef void(*TeaScissorProc)(te_int x, te_int y, te_int width, te_int height);
TEAPI TeaScissorProc teaScissor;

typedef void(*TeaEnableProc)(te_uint cap);
TEAPI TeaEnableProc teaEnable;
typedef void(*TeaDisableProc)(te_uint cap);
TEAPI TeaDisableProc teaDisable;

/* Blend functions */
typedef void(*TeaBlendFuncProc)(te_uint sfactor, te_uint dfactor);
typedef void(*TeaBlendFuncSeparateProc)(te_uint sfactorRGB, te_uint dfactorRGB, te_uint sfactorAlpha, te_uint dfactorAlpha);
typedef void(*TeaBlendEquationProc)(te_uint mode);
typedef void(*TeaBlendEquationSeparateProc)(te_uint modeRGB, te_uint modeAlpha);
typedef void(*TeaBlendColorProc)(te_float r, te_float g, te_float b, te_float a);
TEAPI TeaBlendFuncProc teaBlendFunc;
TEAPI TeaBlendFuncSeparateProc teaBlendFuncSeparate;
TEAPI TeaBlendEquationProc teaBlendEquation;
TEAPI TeaBlendEquationSeparateProc teaBlendEquationSeparate;
TEAPI TeaBlendColorProc teaBlendColor;
TEAPI TeaBlendEquationSeparateProc teaBlendEquationSeparate;

/* Depth functions */
typedef void(*TeaDepthFuncProc)(te_uint func);
typedef void(*TeaDepthMaskProc)(te_bool mask);
typedef void(*TeaDepthRangeProc)(te_double n, te_double f);
TEAPI TeaDepthFuncProc teaDepthFunc;
TEAPI TeaDepthMaskProc teaDepthMask;
TEAPI TeaDepthRangeProc teaDepthRange;

/* Stencil functions */
typedef void(*TeaStencilFuncProc)(te_uint func, te_uint ref, te_uint mask);
typedef void(*TeaStencilMaskProc)(te_uint mask);
typedef void(*TeaStencilOpProc)(te_uint fail, te_uint zfail, te_uint zpass);
TEAPI TeaStencilFuncProc teaStencilFunc;
TEAPI TeaStencilMaskProc teaStencilMask;
TEAPI TeaStencilOpProc teaStencilOp;

/* Texture functions */
#define teaTexture teaTexture2D
TEAPI te_texture_t teaTexture1D(const char* data, te_uint width, te_uint format);
TEAPI te_texture_t teaTexture2D(const char* data, te_uint width, te_uint height, te_uint format);
TEAPI void teaFreeTexture(te_texture_t texture);

typedef void(*TeaBindTextureProc)(te_uint target, te_texture_t texture);
TEAPI TeaBindTextureProc teaBindTexture;

TEAPI void teaTexImage1D(te_uint target, te_int level, te_uint width, const void* pixels);
TEAPI void teaTexImage2D(te_uint target, te_int level, te_uint width, te_uint height, const void *pixels);
TEAPI void teaTexImage3D(te_uint target, te_int level, te_uint width, te_uint height, te_uint depth, const void *pixels);

TEAPI void teaTexSubImage1D(te_uint target, te_int level, te_int xoffset, te_int width, const void *pixels);
TEAPI void teaTexSubImage2D(te_uint target, te_int level, te_int xoffset, te_int yoffset, te_int width, te_int height, const void *pixels);
TEAPI void teaTexSubImage3D(te_uint target, te_int level, te_int xoffset, te_int yoffset, te_int zoffset, te_int width, te_int height, te_int depth, const void *pixels);

TEAPI void teaTexParameteri(te_uint param, te_int value);
TEAPI void teaTexParameteriv(te_uint param, te_int *value);
TEAPI void teaTexParameterf(te_uint param, te_float value);
TEAPI void teaTexParameterfv(te_uint param, te_float *value);

/* Framebuffer functions */
TEAPI te_fbo_t teaFBO(void);
TEAPI void teaFreeFBO(te_fbo_t fbo);

typedef void(*TeaBindFBOProc)(te_uint target, te_fbo_t fbo);
TEAPI TeaBindFBOProc teaBindFBO;

TEAPI void teaFBOTexture(te_fbo_t fbo, te_uint attachment, te_texture_t texture);
TEAPI void teaFBORenderbuffer(te_fbo_t fbo, te_uint attachment, te_rbo_t rbo);

/* Buffer functions */
TEAPI te_buffer_t* teaBuffer(te_uint target, te_uint size, te_uint usage);
TEAPI void teaFreeBuffer(te_buffer_t* buffer);

/* Change memory localy */
TEAPI void teaSeekBuffer(te_buffer_t* buffer, te_uint offset);
TEAPI void teaWriteBuffer(te_buffer_t* buffer, const void* data, te_uint size);
TEAPI void teaReadBuffer(te_buffer_t* buffer, void* data, te_uint size);

TEAPI void teaBindBuffer(te_uint target, te_buffer_t* buffer);

TEAPI void teaResizeBuffer(te_uint target, te_uint size, te_uint usage);
TEAPI void teaGrowBuffer(te_uint target);
/* Send data to GPU */
TEAPI void teaFlushBuffer(te_uint target);
TEAPI void teaSendBuffer(te_uint target, te_uint size);
TEAPI void teaSendRangeBuffer(te_uint target, te_uint offset, te_uint size);

/* VAO functions */
enum {
    TEA_ATTRIB_POSITION = 0,
    TEA_ATTRIB_POSITION_3D,
    TEA_ATTRIB_COLOR,
    TEA_ATTRIB_TEXCOORD,
    TEA_ATTRIB_NORMAL,
};

TEAPI void teaVAOFormat(te_vao_format_t *format);
TEAPI void teaVAOFormatAdd(te_vao_format_t *format, te_uint attr);

TEAPI void teaBindVAOFormat(te_vao_format_t *format);

TEAPI te_vao_t* teaVAO(void);
TEAPI void teaFreeVAO(te_vao_t* vao);

typedef void(*TeaBindVAOProc)(te_vao_t*);
TEAPI TeaBindVAOProc teaBindVAO;

typedef void(*TeaEnableVertexAttribArrayProc)(te_uint index);
typedef void(*TeaDisableVertexAttribArrayProc)(te_uint index);
TEAPI TeaEnableVertexAttribArrayProc teaEnableVertexAttribArray;
TEAPI TeaDisableVertexAttribArrayProc teaDisableVertexAttribArray;

TEAPI void teaVAOSetVertexAttribPointer(te_uint index, te_uint size, te_uint type, te_uint normalized, te_uint stride, te_uint offset);

/* Shader functions */
TEAPI te_shader_t teaShader(const char *fragSrc, const char *vertSrc);
TEAPI void teaFreeShader(te_shader_t shader);

TEAPI void teaUseShader(te_shader_t shader);

typedef te_int(*TeaGetUniformLocationProc)(te_shader_t shader, const char *name);
TEAPI TeaGetUniformLocationProc teaGetUniformLocation;

#define TEA_UNIFORM_X(X, T)\
typedef void(*TeaUniform1##X##Proc)(te_int location, T v0);\
typedef void(*TeaUniform2##X##Proc)(te_int location, T v0, T v1);\
typedef void(*TeaUniform3##X##Proc)(te_int location, T v0, T v1, T v2);\
typedef void(*TeaUniform4##X##Proc)(te_int location, T v0, T v1, T v2, T v3);\
typedef void(*TeaUniform##X##vProc)(te_int location, te_int count, const T *value);\
TEAPI TeaUniform1##X##Proc teaUniform1##X;\
TEAPI TeaUniform2##X##Proc teaUniform2##X;\
TEAPI TeaUniform3##X##Proc teaUniform3##X;\
TEAPI TeaUniform4##X##Proc teaUniform4##X;\
TEAPI TeaUniform##X##vProc teaUniform1##X##v;\
TEAPI TeaUniform##X##vProc teaUniform2##X##v;\
TEAPI TeaUniform##X##vProc teaUniform3##X##v;\
TEAPI TeaUniform##X##vProc teaUniform4##X##v

TEA_UNIFORM_X(f, te_float);
TEA_UNIFORM_X(i, te_int);

typedef void(*TeaUniformMatrixfvProc)(te_int location, te_int count, te_bool transpose, const te_float *value);
TEAPI TeaUniformMatrixfvProc teaUniformMatrix2fv;
TEAPI TeaUniformMatrixfvProc teaUniformMatrix3fv;
TEAPI TeaUniformMatrixfvProc teaUniformMatrix4fv;

/* Debug */
TEAPI void teaAbort(const char *fmt, ...);

/* Loader */
enum {
    TEA_PROC_OVERRIDE = (1 << 0),
    TEA_PROC_RET_ON_DUP = (1 << 1),
};

typedef struct te_proc_s te_proc_t;
typedef struct te_extension_s te_extension_t;

struct te_proc_s {
    te_ubyte tag;
    const te_byte* names[3];
};

TEAPI te_bool teaLoadProcs(te_proc_t *procs, te_uint flags);
TEAPI void* teaGetProc(te_uint tag);

#endif /* _TEA_H */