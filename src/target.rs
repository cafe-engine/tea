extern crate gl;

use crate::{GlBind, GlObject, GlTarget, GlEnum};
use crate::texture::GlTexture;

#[derive(Debug, PartialEq, Eq, Copy, Clone)]
pub enum FramebufferAttachment {
    ColorAttachment(u32),
    DepthAttachment,
    StencilAttachment
}

impl Default for FramebufferAttachment {
    fn default() -> Self {
        FramebufferAttachment::ColorAttachment(0)
    }
}

impl GlEnum for FramebufferAttachment {
    fn to_enum(&self) -> u32 {
        match self {
            Self::ColorAttachment(c) => { return gl::COLOR_ATTACHMENT0 + c; },
            Self::DepthAttachment => { return gl::DEPTH_ATTACHMENT },
            Self::StencilAttachment => { return gl::STENCIL_ATTACHMENT }
        }
    }
}

#[derive(Default, Debug, Eq, PartialEq)]
pub struct Framebuffer(u32);

/******************************
 * Framebuffer
 ******************************/
impl Framebuffer {
    pub fn new() -> Result<Self, String> {
        let mut handle: u32 = 0;
        unsafe {
            gl::GenFramebuffers(1, &mut handle);
        }
        Ok(Framebuffer(handle))
    }

    pub fn attach_texture<T: GlTexture>(&self, attachment: FramebufferAttachment, texture: &T) {
        unsafe {
            gl::FramebufferTexture2D(
                Self::target(),
                attachment.to_enum(), 
                T::target(), texture.get_id(), 0
            );
        }
    }

    pub fn bind_read(&self) {
        unsafe { gl::BindFramebuffer(gl::READ_FRAMEBUFFER, self.0) };
    }

    pub fn bind_draw(&self) {
        unsafe { gl::BindFramebuffer(gl::DRAW_FRAMEBUFFER, self.0) };
    }
}

impl GlTarget for Framebuffer {
    fn target() -> u32 { gl::FRAMEBUFFER }
    fn binding() -> u32 { gl::FRAMEBUFFER_BINDING }
}

impl GlObject for Framebuffer {
    fn get_id(&self) -> u32 {
        self.0
    }
}

impl GlBind for Framebuffer {
    fn bind(&self) {
        unsafe { gl::BindFramebuffer(Self::target(), self.0) };
    }

    fn unbind(&self) {
        unsafe { gl::BindFramebuffer(Self::target(), 0) };
    }
}

impl Drop for Framebuffer {
    fn drop(&mut self) {
        unsafe { gl::DeleteFramebuffers(1, &self.0) };
    }
}
