#[derive(Default, Debug, PartialEq, Eq, Copy, Clone)]
pub enum PixelFormat {
    Alpha,
    Luminance,
    LuminanceAlpha,
    RGB,
    #[default]
    RGBA,
}

impl GlEnum for PixelFormat {
    fn to_enum(&self) -> u32 {
        match self {
            PixelFormat::Alpha => { return gl::ALPHA },
            PixelFormat::Luminance => { return gl::LUMINANCE },
            PixelFormat::LuminanceAlpha => { return gl::LUMINANCE_ALPHA },
            PixelFormat::RGB => { return gl::RGB },
            PixelFormat::RGBA => { return gl::RGBA }
        }
    }
}

#[derive(Default, Debug, PartialEq, Eq, Copy, Clone)]
pub enum TexTarget {
    Texture2D,
    CubeMapPositiveX,
    CubeMapNegativeX,
    CubeMapPositiveY,
    CubeMapNegativeY,
    CubeMapPositiveZ,
    CubeMapNegativeZ,

    Count
}

impl GlEnum for TexTarget {
    fn to_enum(&self) -> u32 {
        match self {
            Self::Texture2D => { gl::TEXTURE_2D },
            Self::CubeMapPositiveX => { gl::TEXTURE_CUBE_MAP_POSITIVE_X },
            Self::CubeMapNegativeX => { gl::TEXTURE_CUBE_MAP_NEGATIVE_X },
            Self::CubeMapPositiveY => { gl::TEXTURE_CUBE_MAP_POSITIVE_Y },
            Self::CubeMapNegativeY => { gl::TEXTURE_CUBE_MAP_NEGATIVE_Y },
            Self::CubeMapPositiveZ => { gl::TEXTURE_CUBE_MAP_POSITIVE_Z },
            Self::CubeMapNegativeZ => { gl::TEXTURE_CUBE_MAP_NEGATIVE_Z },
        }
    }
}

impl TexTarget {
    pub fn get_binding(&self) -> u32 {
        match self {
            Self::Texture2D => { gl::TEXTURE_BINDING_2D },
            _ => { gl::NONE }
        }
    }
}

#[derive(Default, Debug, PartialEq, Eq, Copy, Clone)]
pub enum TexFilter {
    #[default]
    Nearest,
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
}

impl GlEnum for TexWrap {
    fn to_enum(&self) -> u32 {
        match self {
            Self::Repeat => { gl::REPEAT },
            Self::MirroredRepeat => { gl::MIRRORED_REPEAT }
            Self::ClampToEdge => { gl::CLAMP_TO_EDGE },
        }
    }
}


#[derive(Default, Debug, PartialEq, Eq, Copy, Clone)]
pub enum TexParam {
    MinFilter(TexFilter),
    MagFilter(TexFilter),
    WrapS(TexWrap),
    WrapT(TexWrap)
}

impl GlEnum for TexParam {
    fn to_enum(&self) -> u32 {
        match self {
            Self::MinFilter(_) => { gl::TEXTURE_MIN_FILTER },
            Self::MagFilter(_) => { gl::TEXTURE_MAG_FILTER },
            Self::WrapS(_) => { gl::TEXTURE_WRAP_S },
            Self::WrapT(_) => { gl::TEXTURE_WRAP_T },
        }
    }
}
