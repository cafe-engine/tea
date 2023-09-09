use crate::{GlEnum, GlObject, GlBind};

#[derive(Default, Debug, PartialEq, Eq, Copy, Clone)]
pub enum PixelFormat {
    Red,
    RG,
    RGB,
    #[default]
    RGBA,
    Depth,
    Depth16,
    Depth24,
    Depth32,
    Depth32F,
    Depth24Stencil8,
    Depth32FStencil8
}

impl GlEnum for PixelFormat {
    fn to_enum(&self) -> u32 {
        match self {
            PixelFormat::Red => { return gl::RED },
            PixelFormat::RG => { return gl::RG },
            PixelFormat::RGB => { return gl::RGB },
            PixelFormat::RGBA => { return gl::RGBA },
            PixelFormat::Depth => { return gl::DEPTH_COMPONENT },
            PixelFormat::Depth16 => { return gl::DEPTH_COMPONENT16 },
            PixelFormat::Depth24 => { return gl::DEPTH_COMPONENT24 },
            PixelFormat::Depth32 => { return gl::DEPTH_COMPONENT32 },
            PixelFormat::Depth32F => { return gl::DEPTH_COMPONENT32F },
            PixelFormat::Depth24Stencil8 => { return gl::DEPTH24_STENCIL8 },
            PixelFormat::Depth32FStencil8 => { return gl::DEPTH32F_STENCIL8 },
        }
    }
}

pub trait Texture: GlBind + GlObject {
    type Target;
}

#[derive(Default, Debug, PartialEq, Eq, Copy, Clone)]
pub enum TexTarget {
    Texture1D,
    #[default]
    Texture2D,
    CubeMapPositiveX,
    CubeMapNegativeX,
    CubeMapPositiveY,
    CubeMapNegativeY,
    CubeMapPositiveZ,
    CubeMapNegativeZ,
    ProxyTexture2D,
    ProxyCubeMap,
    Texture3D,
    Texture1DArray,
    Texture2DArray,
    Texture2DMultisample,
    Texture2DMultisampleArray,
}

impl GlEnum for TexTarget {
    fn to_enum(&self) -> u32 {
        match self {
            Self::Texture1D => { gl::TEXTURE_1D },
            Self::Texture2D => { gl::TEXTURE_2D },
            Self::CubeMapPositiveX => { gl::TEXTURE_CUBE_MAP_POSITIVE_X },
            Self::CubeMapNegativeX => { gl::TEXTURE_CUBE_MAP_NEGATIVE_X },
            Self::CubeMapPositiveY => { gl::TEXTURE_CUBE_MAP_POSITIVE_Y },
            Self::CubeMapNegativeY => { gl::TEXTURE_CUBE_MAP_NEGATIVE_Y },
            Self::CubeMapPositiveZ => { gl::TEXTURE_CUBE_MAP_POSITIVE_Z },
            Self::CubeMapNegativeZ => { gl::TEXTURE_CUBE_MAP_NEGATIVE_Z },
            Self::ProxyTexture2D => { gl::PROXY_TEXTURE_2D },
            Self::ProxyCubeMap => { gl::PROXY_TEXTURE_CUBE_MAP },
            Self::Texture3D => { gl::TEXTURE_3D },
            Self::Texture1DArray => { gl::TEXTURE_1D_ARRAY },
            Self::Texture2DArray => { gl::TEXTURE_2D_ARRAY },
            Self::Texture2DMultisample => { gl::TEXTURE_2D_MULTISAMPLE },
            Self::Texture2DMultisampleArray => { gl::TEXTURE_2D_MULTISAMPLE_ARRAY },
        }
    }
}

impl TexTarget {
    pub fn get_binding(&self) -> u32 {
        match self {
            TexTarget::Texture2D => { gl::TEXTURE_BINDING_2D },
            _ => { gl::NONE }
        }
    }
}

#[derive(Default, Debug, PartialEq, Eq, Copy, Clone)]
pub enum TexFilter {
    Nearest,
    #[default]
    Linear,
    // only for min filter
    NearestMipmapNearest,
    LinearMipmapNearest,
    NearestMipmapLinear,
    LinearMipmapLinear
}

impl GlEnum for TexFilter {
    fn to_enum(&self) -> u32 {
        match self {
            Self::Nearest => { gl::NEAREST },
            Self::Linear => { gl::LINEAR },
            Self::NearestMipmapNearest => { gl::NEAREST_MIPMAP_NEAREST },
            Self::LinearMipmapNearest => { gl::LINEAR_MIPMAP_NEAREST },
            Self::NearestMipmapLinear => { gl::NEAREST_MIPMAP_LINEAR },
            Self::LinearMipmapLinear => { gl::LINEAR_MIPMAP_LINEAR },
        }
    }
}

#[derive(Default, Debug, PartialEq, Eq, Copy, Clone)]
pub enum TexWrap {
    #[default]
    Repeat,
    MirroredRepeat,
    ClampToEdge,
    ClampToBorder,
}

impl GlEnum for TexWrap {
    fn to_enum(&self) -> u32 {
        match self {
            Self::Repeat => { gl::REPEAT },
            Self::MirroredRepeat => { gl::MIRRORED_REPEAT }
            Self::ClampToEdge => { gl::CLAMP_TO_EDGE },
            Self::ClampToBorder => { gl::CLAMP_TO_BORDER },
        }
    }
}

#[derive(Debug, PartialEq, Eq, Copy, Clone)]
pub enum TexParam {
    MinFilter(TexFilter),
    MagFilter(TexFilter),
    WrapS(TexWrap),
    WrapT(TexWrap),
    WrapR(TexWrap),
}

impl Default for TexParam {
    fn default() -> Self {
        TexParam::MinFilter(TexFilter::default())
    }
}

impl GlEnum for TexParam {
    fn to_enum(&self) -> u32 {
        match self {
            Self::MinFilter(_) => { gl::TEXTURE_MIN_FILTER },
            Self::MagFilter(_) => { gl::TEXTURE_MAG_FILTER },
            Self::WrapS(_) => { gl::TEXTURE_WRAP_S },
            Self::WrapT(_) => { gl::TEXTURE_WRAP_T }
            Self::WrapR(_) => { gl::TEXTURE_WRAP_R }
        }
    }
}
