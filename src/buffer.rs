extern crate gl;

use std::ffi::c_void;

use gl::types::GLenum;
use crate::*;

#[derive(Default, Debug, PartialEq, Eq, PartialOrd, Clone, Copy)]
pub enum BufferUsage {
    StaticRead,
    StaticCopy,
    StaticDraw,
    StreamRead,
    StreamCopy,
    StreamDraw,
    DynamicRead,
    DynamicCopy,
    #[default]
    DynamicDraw
}

impl GlEnum for BufferUsage {
    fn to_enum(&self) -> GLenum {
        match self {
            BufferUsage::StaticRead => { gl::STATIC_READ },
            BufferUsage::StaticCopy => { gl::STATIC_COPY },
            BufferUsage::StaticDraw => { gl::STATIC_DRAW },
            BufferUsage::StreamRead => { gl::STREAM_READ },
            BufferUsage::StreamCopy => { gl::STREAM_COPY },
            BufferUsage::StreamDraw => { gl::STREAM_DRAW },
            BufferUsage::DynamicRead => { gl::DYNAMIC_READ },
            BufferUsage::DynamicCopy => { gl::DYNAMIC_COPY },
            BufferUsage::DynamicDraw => { gl::DYNAMIC_DRAW }
        }
    }
}

pub trait GlBuffer: GlObject + GlBind + GlTarget {
    fn data<T: Sized>(&self, data: &Vec<T>, usage: BufferUsage) {
        let size = std::mem::size_of::<T>() * data.capacity();
        let ptr: *const c_void;
        if data.is_empty() { ptr = std::ptr::null(); }
        else { ptr = data.as_ptr() as *const c_void; }
        unsafe {
            gl::BufferData(Self::target(), size as isize, ptr, usage.to_enum());
        }
    }

    fn subdata<T: Sized>(&self, offset: isize, count: usize, data: &Vec<T>) {
        let size = std::mem::size_of::<T>() * count;
        let ptr: *const c_void;
        if data.is_empty() { ptr = std::ptr::null(); }
        else { ptr = data.as_ptr() as *const c_void; }
        unsafe {
            gl::BufferSubData(Self::target(), offset, size as isize, ptr);
        }
    }

    fn bind_read(&self) {
        unsafe { gl::BindBuffer(gl::COPY_READ_BUFFER, self.get_id()) };
    }

    fn bind_write(&self) {
        unsafe { gl::BindBuffer(gl::COPY_WRITE_BUFFER, self.get_id()) };
    }

    fn copy(&self, read_offset: isize, write_offset: isize, size: isize) {
        unsafe { gl::CopyBufferSubData(gl::COPY_READ_BUFFER, gl::COPY_WRITE_BUFFER, read_offset, write_offset, size) };
    }

    fn copy_from(&self, other: impl GlBuffer, read_offset: isize, write_offset: isize, size: isize) {
        other.bind_read();
        unsafe { gl::CopyBufferSubData(gl::COPY_READ_BUFFER, gl::COPY_WRITE_BUFFER, read_offset, write_offset, size) };
    }

    fn is_binded(&self) -> bool {
        Self::current_bind() == self.get_id()
    }
}

macro_rules! impl_buffer {
    ($Name: ident, $target: expr, $binding: expr) => {
        #[derive(Default, Debug, PartialEq, Eq, PartialOrd, Ord)]
        pub struct $Name(u32);

        impl $Name {
            pub fn new() -> Self {
                let mut handle = 0;
                unsafe { gl::GenBuffers(1, &mut handle) };
                Self(handle)
            }
        }

        impl GlObject for $Name {
            fn get_id(&self) -> u32 { self.0 }
        }

        impl GlTarget for $Name {
            fn target() -> u32 { $target }
            fn binding() -> u32 { $binding }
        }

        impl GlBind for $Name {
            fn bind(&self) {
                unsafe { gl::BindBuffer(Self::target(), self.0) };
            }

            fn unbind(&self) {
                unsafe { gl::BindBuffer(Self::target(), 0) };
            }
        }

        impl GlBuffer for $Name {}

        impl Drop for $Name {
            fn drop(&mut self) {
                unsafe { gl::DeleteBuffers(1, &mut self.0) };
            }
        }
    }
}

impl_buffer!(ArrayBuffer, gl::ARRAY_BUFFER, gl::ARRAY_BUFFER_BINDING);
impl_buffer!(ElementArrayBuffer, gl::ELEMENT_ARRAY_BUFFER, gl::ELEMENT_ARRAY_BUFFER_BINDING);
impl_buffer!(UniformBuffer, gl::UNIFORM_BUFFER, gl::UNIFORM_BUFFER_BINDING);
impl_buffer!(TextureBuffer, gl::TEXTURE_BUFFER, gl::TEXTURE_BUFFER_BINDING);
