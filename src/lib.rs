extern crate gl;

use std::ffi::{c_void, CStr};

pub mod buffer;
pub mod glsl;
pub mod vec;
pub mod texture;
pub mod target;
pub mod vertex;

pub use vec::{Vec2, Vec3, Vec4};

pub trait GlslType {
    fn to_glsl() -> String {
        "".to_owned()
    }
}

#[macro_export]
macro_rules! impl_glsl {
    ($tp: ty, $name: literal) => {
        impl GlslType for $tp {
            fn to_glsl() -> String { $name.to_owned() }
        }
    };
}

impl_glsl!(bool, "bool");
impl_glsl!(i32, "int");
impl_glsl!(u32, "uint");
impl_glsl!(f32, "float");
impl_glsl!(f64, "double");

pub trait GlTarget {
    fn target() -> u32;
    fn binding() -> u32;
    fn current_bind() -> u32 {
        let mut gl_id = 0;
        unsafe { gl::GetIntegerv(Self::binding(), &mut gl_id) };
        return gl_id as u32;
    }
}

pub trait GlBind {
    fn bind(&self);
    fn unbind(&self);
}

pub trait GlUse {
    fn set_used(&self);
    fn set_unused(&self);
}

pub trait GlEnum {
    fn gl_enum() -> u32 { gl::NONE }
    fn to_enum(&self) -> u32 { gl::NONE }
}

pub trait GlObject {
    fn get_id(&self) -> u32;
}

pub trait GlAttrib: GlEnum + Sized {
    fn size() -> u32 { 1 }
    fn stride() -> usize { std::mem::size_of::<Self>() }
    fn get_size(&self) -> u32 { Self::size() }
    fn get_stride(&self) -> usize { Self::stride() }

    fn setup_attrib(index: u32, normalized: bool, stride: i32, start: i32) {
        unsafe { gl::VertexAttribPointer(index, Self::size() as i32, Self::gl_enum(), normalized as u8, stride, start as *const c_void) };
    }
}

pub trait GlUniform {
    fn uniform(location: i32, val: &Self);
    fn uniformv(location: i32, count: i32, ptr: *const Self);

    fn send_uniform(&self, location: i32);
}

#[derive(Default, Debug)]
pub enum GlType {
    #[default]
    Byte,
    UByte,
    Short,
    UShort,
    Int,
    UInt,
    Float,
    Double
}

impl GlEnum for GlType {
    fn to_enum(&self) -> u32 {
        match self {
            GlType::Byte => { return gl::BYTE },
            GlType::UByte => { return gl::UNSIGNED_BYTE },
            GlType::Short => { return gl::SHORT },
            GlType::UShort => { return gl::UNSIGNED_SHORT },
            GlType::Int => { return gl::INT },
            GlType::UInt => { return gl::UNSIGNED_INT },
            GlType::Float => { return gl::FLOAT },
            GlType::Double => { return gl::DOUBLE }
        }
    }
}

macro_rules! impl_type {
    ($tp: ty, $Enum: expr) => {
        impl GlEnum for $tp {
            fn gl_enum() -> u32 { $Enum }
            fn to_enum(&self) -> u32 { $Enum }
        }

        impl GlAttrib for $tp {}
    };
    ($tp: ty, $Enum: expr, $Uniform: ident, $Uniformv: ident) => {
        impl GlEnum for $tp {
            fn gl_enum() -> u32 { $Enum }
            fn to_enum(&self) -> u32 { $Enum }
        }

        impl GlAttrib for $tp {}

        impl GlUniform for $tp {
            fn uniform(location: i32, val: &$tp) {
                unsafe { gl::$Uniform(location, *val) };
            }

            fn uniformv(location: i32, count: i32, ptr: *const $tp) {
                unsafe { gl::$Uniformv(location, count, ptr) };
            }

            fn send_uniform(&self, location: i32) {
                unsafe { gl::$Uniform(location, *self) };
            }
        }

        // impl GlUniform for &[$tp] {
        //     fn send_uniform(&self, location: i32) {
        //         let count = std::mem::size_of_val(self) /
        //             std::mem::size_of::<$tp>();
        //         unsafe {
        //             gl::$Uniform(location, count as i32, self.as_ptr() as *const $tp);
        //         }
        //     }
        // }
    };
    ($tp: ty, $Enum: expr, $Uniform: ident, $Uniformv: ident, $convert: ty) => {
        impl GlEnum for $tp {
            fn gl_enum() -> u32 { $Enum }
            fn to_enum(&self) -> u32 { $Enum }
        }

        impl GlAttrib for $tp {}

        impl GlUniform for $tp {
            fn uniform(location: i32, val: &$tp) {
                unsafe { gl::$Uniform(location, *val as $convert) };
            }
            fn uniformv(location: i32, count: i32, ptr: *const Self) {
                unsafe { gl::$Uniformv(location, count, ptr as *const $convert) };
            }

            fn send_uniform(&self, location: i32) {
                unsafe { gl::$Uniform(location, *self as $convert) };
            }
        }

        // impl GlUniform for $tp {
        //     fn send_uniform(&self, location: i32) {
        //         let p = self as *const Self;
        //         unsafe { gl::$Uniform(location, 1, p as *const $convert) };
        //     }
        // }

        // impl GlUniform for &[$tp] {
        //     fn send_uniform(&self, location: i32) {
        //         unsafe {
        //             gl::$Uniform(location, 1, self.as_ptr() as *const $convert)
        //         }
        //     }
        // }
    };
}

// impl<T: GlUniform> GlUniform for &[T] {
//     fn send_uniform(&self, location: i32) {
//         for v in *self {
//             v.send_uniform(location);
//         }
//     }
// }

impl_type!(bool, gl::BOOL, Uniform1i, Uniform1iv, i32);
impl_type!(i8, gl::BYTE, Uniform1i, Uniform1iv, i32);
impl_type!(u8, gl::UNSIGNED_BYTE, Uniform1ui, Uniform1uiv, u32);
impl_type!(i16, gl::SHORT, Uniform1i, Uniform1iv, i32);
impl_type!(u16, gl::UNSIGNED_SHORT, Uniform1ui, Uniform1uiv, u32);
impl_type!(i32, gl::INT, Uniform1i, Uniform1iv);
impl_type!(u32, gl::UNSIGNED_INT, Uniform1ui, Uniform1uiv);
impl_type!(f32, gl::FLOAT, Uniform1f, Uniform1fv);
impl_type!(f64, gl::DOUBLE, Uniform1d, Uniform1dv);

#[derive(Default, Debug, PartialEq, Eq, Clone, Copy)]
pub enum ClearFlags {
    #[default]
    ColorBufferBit,
    DepthBufferBit,
    StencilBufferBit
}

impl GlEnum for ClearFlags {
    fn to_enum(&self) -> u32 {
        match self {
            Self::ColorBufferBit => { gl::COLOR_BUFFER_BIT },
            Self::DepthBufferBit => { gl::DEPTH_BUFFER_BIT },
            Self::StencilBufferBit => { gl::STENCIL_BUFFER_BIT },
        }
    }
}

#[derive(Default, Debug, PartialEq, Eq, Clone, Copy)]
pub enum EnableFlags {
    #[default]
    Blend,
    CullFace,
    DepthTest,
    Dither,
    Texture2D,
    ScissorTest,
}

impl GlEnum for EnableFlags {
    fn to_enum(&self) -> u32 {
        match self {
            Self::Blend => { gl::BLEND },
            Self::CullFace => { gl::CULL_FACE },
            Self::DepthTest => { gl::DEPTH_TEST },
            Self::Dither => { gl::DITHER },
            Self::Texture2D => { gl::TEXTURE_2D },
            Self::ScissorTest => { gl::SCISSOR_TEST }
        }
    }
}


#[derive(Default, Debug, PartialEq, Eq, Copy, Clone)]
pub enum DrawMode {
    Points,
    Lines,
    #[default]
    Triangles
}

impl GlEnum for DrawMode {
    fn to_enum(&self) -> u32 {
        match self {
            Self::Points => { gl::POINTS },
            Self::Lines => { gl::LINES },
            Self::Triangles => { gl::TRIANGLES }
        }
    }
}

#[derive(Debug, PartialEq, Eq, PartialOrd, Ord, Copy, Clone)]
pub enum BlendFunc {
    Zero,
    One,
    SrcColor,
    OneMinusSrcColor,
    DstColor,
    OneMinusDstColor,
    SrcAlpha,
    OneMinusSrcAlpha,
    DstAlpha,
    OneMinusDstAlpha,
    ConstantColor,
    OneMinusConstantColor,
    ConstantAlpha,
    OneMinusConstantAlpha,
}

impl GlEnum for BlendFunc {
    fn to_enum(&self) -> u32 {
        match self {
            Self::Zero => { gl::ZERO },
            Self::SrcAlpha => { gl::SRC_ALPHA },
            Self::OneMinusSrcAlpha => { gl::ONE_MINUS_SRC_ALPHA },
            _ => { gl::NONE }
        }
    }
}

#[derive(Default, Debug, PartialEq, Eq, PartialOrd, Ord, Copy, Clone)]
pub enum CullMode {
    #[default]
    Front,
    Back,
    FrontAndBack,
}

impl GlEnum for CullMode {
    fn to_enum(&self) -> u32 {
        match self {
            Self::Front => { gl::FRONT },
            Self::Back => { gl::BACK },
            Self::FrontAndBack => { gl::FRONT_AND_BACK },
        }
    }
}

#[derive(Default, Debug, PartialEq, Eq, PartialOrd, Ord, Copy, Clone)]
pub enum FrontFace {
    #[default]
    CCW,
    CW
}

impl GlEnum for FrontFace {
    fn to_enum(&self) -> u32 {
        match self {
            Self::CCW => { gl::CCW },
            Self::CW => { gl::CW },
        }
    }
}

#[derive(Debug, PartialEq, Eq, PartialOrd, Ord, Copy, Clone)]
pub enum GlFunc {
    Never,
    Less,
    Equal,
    LEqual,
    Greater,
    NotEqual,
    GEqual,
    Always
}

impl GlEnum for GlFunc {
    fn to_enum(&self) -> u32 {
        match self {
            Self::Never => { gl::NEVER },
            Self::Less => { gl::LESS },
            Self::Equal => { gl::EQUAL },
            Self::LEqual => { gl::LEQUAL },
            Self::Greater => { gl::GREATER },
            Self::NotEqual => { gl::NOTEQUAL },
            Self::GEqual => { gl::GEQUAL },
            Self::Always => { gl::ALWAYS },
        }
    }
}

pub fn gl_version() -> String {
    let version = unsafe { gl::GetString(gl::VERSION) };
    #[cfg(target_arch="aarch64")]
    let ptr = unsafe { CStr::from_ptr(version as *const u8) };
    #[cfg(not(target_arch="aarch64"))]
    let ptr = unsafe { CStr::from_ptr(version as *const i8) };
    return ptr.to_str().unwrap().to_string()
}

pub fn glsl_version() -> String {
    let version = unsafe { gl::GetString(gl::SHADING_LANGUAGE_VERSION) };
    #[cfg(target_arch="aarch64")]
    let ptr = unsafe { CStr::from_ptr(version as *const u8) };
    #[cfg(not(target_arch="aarch64"))]
    let ptr = unsafe { CStr::from_ptr(version as *const i8) };
    return ptr.to_str().unwrap().to_string()
}

pub fn load_with<F: FnMut(&'static str) -> *const c_void>(loadfn: F) { gl::load_with(loadfn) }

pub fn enable(flags: &[EnableFlags]) {
    for flag in flags {
        unsafe { gl::Enable(flag.to_enum()) };
    }
}

pub fn disable(flags: &[EnableFlags]) {
    for flag in flags {
        unsafe { gl::Disable(flag.to_enum()) };
    }
}

// Blend
pub fn blend_color(color: Vec4) {
    unsafe { gl::BlendColor(color.x, color.y, color.z, color.w) };
}

pub fn blend_func(sfactor: BlendFunc, dfactor: BlendFunc) {
    unsafe { gl::BlendFunc(sfactor.to_enum(), dfactor.to_enum()) };
}

pub fn blend_func_separate(src_rgb: BlendFunc, dst_rgb: BlendFunc, src_alpha: BlendFunc, dst_alpha: BlendFunc) {
    unsafe { gl::BlendFuncSeparate(src_rgb.to_enum(), dst_rgb.to_enum(), src_alpha.to_enum(), dst_alpha.to_enum()) };
}

// Color
pub fn color_mask(r: bool, g: bool, b: bool, a: bool) {
    unsafe { gl::ColorMask(r.to_enum() as u8, g.to_enum() as u8, b.to_enum() as u8, a.to_enum() as u8) }
}

pub fn clear_color(r: f32, g: f32, b: f32, a: f32) {
    unsafe { gl::ClearColor(r, g, b, a) };
}

// Culling
pub fn cull_face(mode: CullMode) {
    unsafe { gl::CullFace(mode.to_enum()) };
}

pub fn front_face(mode: FrontFace) {
    unsafe { gl::FrontFace(mode.to_enum()) };
}

// Depth
pub fn depth_func(func: GlFunc) {
    unsafe { gl::DepthFunc(func.to_enum()) };
}

pub fn depth_mask(enabled: bool) {
    unsafe { gl::DepthMask(enabled.to_enum() as u8) };
}

pub fn clear_depth(d: f64) {
    unsafe { gl::ClearDepth(d) };
}

// Stencil
pub fn stencil_func(func: GlFunc, ref_: i32, mask: u32) {
    unsafe { gl::StencilFunc(func.to_enum(), ref_, mask) };
}

pub fn stencil_func_separate(face: CullMode, func: GlFunc, ref_: i32, mask: u32) {
    unsafe { gl::StencilFuncSeparate(face.to_enum(), func.to_enum(), ref_, mask) };
}

pub fn clear_stencil(s: i32) {
    unsafe { gl::ClearStencil(s) };
}

// States
pub fn clear(flags: &[ClearFlags]) {
    let mut mask = 0;
    for flag in flags {
        mask = mask | (flag.to_enum());
    }
    unsafe { gl::Clear(mask) };
}

pub fn scissor(x: i32, y: i32, width: i32, height: i32) {
    unsafe { gl::Scissor(x, y, width, height) };
}

pub fn viewport(x: i32, y: i32, width: i32, height: i32) {
    unsafe { gl::Viewport(x, y, width, height) };
}

pub fn draw_arrays(mode: DrawMode, start: i32, count: i32) {
    unsafe { gl::DrawArrays(mode.to_enum(), start, count) };
}

pub fn draw_elements(mode: DrawMode, start: i32, count: i32) {
    unsafe { gl::DrawElements(mode.to_enum(), count, gl::UNSIGNED_INT, start as *const c_void) };
}
