extern crate gl;

#[cfg_attr(feature = "gles2", path = "texture/gles2.rs")]
#[cfg_attr(feature = "gl2", path = "texture/gl2.rs")]
#[cfg_attr(all(not(feature = "gl2"), not(feature = "gles2")), path = "texture/gl3.rs")]
pub mod defs;

pub use crate::texture::defs::*;

use crate::{GlEnum, GlBind, GlObject, GlTarget, GlslType, impl_glsl};
use std::ffi::c_void;

#[derive(Default, Debug, Copy, Clone, PartialEq, Eq, PartialOrd, Ord)]
pub enum PixelDataType {
    Byte,
    #[default]
    UByte,
    Short,
    UShort,
    Int,
    UInt,
    Float,
    UByte3_3_2,
    UByte2_3_3Rev,
    UShort5_6_5,
    UShort5_6_5Rev,
    UShort4_4_4_4,
    UShort5_5_5_1,
    UShort1_5_5_5Rev,
    UInt8_8_8_8,
    UInt8_8_8_8Rev,
    UInt10_10_10_2,
    UInt2_10_10_10Rev,
}

impl GlEnum for PixelDataType {
    fn to_enum(&self) -> u32 {
        match self {
            Self::Byte => { gl::BYTE },
            Self::UByte => { gl::UNSIGNED_BYTE },
            Self::Short => { gl::SHORT },
            Self::UShort => { gl::UNSIGNED_SHORT },
            Self::Int => { gl::INT },
            Self::UInt => { gl::UNSIGNED_INT },
            Self::Float => { gl::FLOAT },
            _ => { gl::NONE }
        }
    }
}

fn gen_texture() -> u32 {
    let mut handle = 0;
    unsafe { gl::GenTextures(1, &mut handle) };
    handle
}

pub trait GlTexture: GlBind + GlObject + GlTarget {
    type Size;
    fn set_min_filter(&self, filter: TexFilter) {
        unsafe {
            gl::TexParameteri(Self::target(), gl::TEXTURE_MIN_FILTER, filter.to_enum() as i32);
        }
    }
    fn set_mag_filter(&self, filter: TexFilter) {
        unsafe {
            gl::TexParameteri(Self::target(), gl::TEXTURE_MAG_FILTER, filter.to_enum() as i32);
        }
    }

    fn set_wrap_s(&self, wrap: TexWrap) {
        unsafe {
            gl::TexParameteri(Self::target(), gl::TEXTURE_WRAP_S, wrap.to_enum() as i32);
        }
    }

    fn set_wrap_t(&self, wrap: TexWrap) {
        unsafe {
            gl::TexParameteri(Self::target(), gl::TEXTURE_WRAP_T, wrap.to_enum() as i32);
        }
    }

    fn set_wrap_r(&self, wrap: TexWrap) {
        unsafe {
            gl::TexParameteri(Self::target(), gl::TEXTURE_WRAP_R, wrap.to_enum() as i32);
        }
    }

    fn data(&self, level: i32, format: PixelFormat, size: Self::Size, data_type: PixelDataType, data: Vec<u8>);
}

macro_rules! impl_gl_traits {
    ($Name: ident, $target: expr, $binding: expr ) => {
        #[derive(Default, Debug, PartialEq, Eq, PartialOrd, Ord)]
        pub struct $Name(u32);

        impl GlObject for $Name {
            fn get_id(&self) -> u32 { self.0 }
        }

        impl GlBind for $Name {
            fn bind(&self) {
                unsafe { gl::BindTexture(Self::target(), self.0) }
            }

            fn unbind(&self) {
                unsafe { gl::BindTexture(Self::target(), 0) }
            }
        }

        impl GlTarget for $Name {
            fn target() -> u32 { $target }
            fn binding() -> u32 { $binding }
        }

        impl Drop for $Name {
            fn drop(&mut self) {
                unsafe { gl::DeleteTextures(1, &mut self.0) };
            }
        }
    }
}

impl_gl_traits!(Texture1D, gl::TEXTURE_1D, gl::TEXTURE_BINDING_1D);
impl_gl_traits!(Texture2D, gl::TEXTURE_2D, gl::TEXTURE_BINDING_2D);
impl_gl_traits!(TextureCubeMap, gl::TEXTURE_CUBE_MAP, gl::TEXTURE_BINDING_CUBE_MAP);
impl_gl_traits!(Texture3D, gl::TEXTURE_3D, gl::TEXTURE_BINDING_3D);

fn tex_image_2d(target: u32, level: i32, internal: PixelFormat, size: (i32, i32), format: PixelFormat, data_type: PixelDataType, data: Vec<u8>) {
    let pixels: *const c_void;
    if data.is_empty() {
        pixels = std::ptr::null();
    } else {
        pixels = data.as_ptr() as *const c_void;
    }

    unsafe {
        gl::TexImage2D(
            target, level,
            internal.to_enum() as i32, size.0, size.1, 0,
            format.to_enum(), data_type.to_enum(), pixels
        );
    }
}

fn tex_subimage_2d(target: u32, level: i32, offset: (i32, i32), size: (i32, i32), format: PixelFormat, data_type: PixelDataType, data: Vec<u8>) {
    let pixels: *const c_void;
    if data.is_empty() {
        pixels = std::ptr::null();
    } else {
        pixels = data.as_ptr() as *const c_void;
    }

    unsafe {
        gl::TexSubImage2D(
            target, level,
            offset.0, offset.1,
            size.0, size.1,
            format.to_enum(), data_type.to_enum(), pixels
        );
    }
}

impl GlTexture for Texture2D {
    type Size = (i32, i32);
    fn data(&self, level: i32, format: PixelFormat, size: Self::Size, data_type: PixelDataType, data: Vec<u8>) {
        tex_image_2d(Self::target(), level, format, size, format, data_type, data);
    }
}

impl Texture1D {
    pub fn new() -> Result<Self, String> { Ok(Self(gen_texture())) }
}

impl Texture2D {
    pub fn new() -> Result<Self, String> { Ok(Self(gen_texture())) }

    pub fn data(&self, level: i32, format: PixelFormat, size: (i32, i32), data_type: PixelDataType, data: Vec<u8>) {
        tex_image_2d(Self::target(), level, format, size, format, data_type, data);
    }

    pub fn subdata(&self, level: i32, offset: (i32, i32), size: (i32, i32), format: PixelFormat, data_type: PixelDataType, data: Vec<u8>) {
        tex_subimage_2d(Self::target(), level, offset, size, format, data_type, data); 
    }
}
impl Texture3D { pub fn new() -> Result<Self, String> { Ok(Self(gen_texture())) } }

#[derive(Default, Debug, Copy, Clone, PartialEq, PartialOrd, Eq, Ord)]
pub enum CubeMapSide {
    #[default]
    PositiveX,
    NegativeX,
    PositiveY,
    NegativeY,
    PositiveZ,
    NegativeZ
}

impl GlEnum for CubeMapSide {
    fn to_enum(&self) -> u32 {
        match self {
            Self::PositiveX => { gl::TEXTURE_CUBE_MAP_POSITIVE_X },
            Self::NegativeX => { gl::TEXTURE_CUBE_MAP_NEGATIVE_X },
            Self::PositiveY => { gl::TEXTURE_CUBE_MAP_POSITIVE_Y },
            Self::NegativeY => { gl::TEXTURE_CUBE_MAP_POSITIVE_Y },
            Self::PositiveZ => { gl::TEXTURE_CUBE_MAP_NEGATIVE_Z },
            Self::NegativeZ => { gl::TEXTURE_CUBE_MAP_NEGATIVE_Z },
        }
    }
}

impl TextureCubeMap {
    pub fn new() -> Result<Self, String> {
        let mut handle = 0;
        unsafe { gl::GenTextures(1, &mut handle) };
        Ok(Self(handle))
    }

    pub fn data(&self, side: CubeMapSide, level: i32, format: PixelFormat, size: (i32, i32), data_type: PixelDataType, data: Vec<u8>) { 
        tex_image_2d(side.to_enum(), level, format, size, format, data_type, data);
    }

    pub fn subdata(&self, side: CubeMapSide, level: i32, offset: (i32, i32), size: (i32, i32), format: PixelFormat, data_type: PixelDataType, data: Vec<u8>) {
        tex_subimage_2d(side.to_enum(), level, offset, size, format, data_type, data);
    }
}

pub struct Sampler2D;
impl_glsl!(Sampler2D, "sampler2D");
