use crate::{GlBind, GlObject, GlTarget, GlAttrib};
use std::ffi::c_void;

static mut CURRENT_VAO: u32 = 0;

pub trait VertexData {
    type Output;
    fn stride() -> usize {
        std::mem::size_of::<Self::Output>()
    }

    fn get_format() -> VertexFormat;
}

#[derive(Default, Debug, PartialEq, Eq, Clone)]
pub struct VertexFormat(Vec<AttribSlot>);

impl VertexFormat {
    pub fn attribs(&self) -> &Vec<AttribSlot> {
        &self.0
    }
}

#[derive(Default, Debug)]
pub struct VertexFormatBuilder {
    location: u32,
    offset: i32,
    format: VertexFormat,
}

impl VertexFormatBuilder {
    pub fn new() -> Self {
        VertexFormatBuilder::default()
    }

    pub fn push<T: GlAttrib>(&mut self, normalized: bool) -> &mut Self {
        let attrib = AttribSlot {
            location: self.location,
            type_: T::gl_enum(),
            size: T::size() as i32,
            normalized,
            offset: self.offset
        };
        self.location += 1;
        self.offset += T::stride() as i32;
        self.format.0.push(attrib);
        self
    }

    pub fn build(&self) -> VertexFormat {
        self.format.clone()
    }
}

#[derive(Default, Debug, PartialEq, Eq, Clone, Copy)]
pub struct AttribSlot {
    pub location: u32,
    pub type_: u32,
    pub size: i32,
    pub normalized: bool,
    pub offset: i32
}

impl AttribSlot {
    pub fn new<T: GlAttrib>(location: u32, normalized: bool, offset: i32) -> AttribSlot {
        AttribSlot {
            location,
            type_: T::gl_enum(),
            size: T::size() as i32,
            normalized,
            offset
        }
    }
}

#[derive(Default, Debug, PartialEq, Eq, Ord, PartialOrd)]
pub struct VertexArray(u32);

impl VertexArray {
    pub fn new() -> Result<VertexArray, String> {
        let mut handle: u32 = 0;
        unsafe { gl::GenVertexArrays(1, &mut handle) };
        Ok(VertexArray(handle))
    }

    pub fn check_bind(&self) {
        let cur_vao = Self::current_bind();
        assert_eq!(cur_vao, self.0);
    }

    pub fn setup_for<T: VertexData>(&self) {
        self.check_bind();
        let format = T::get_format();
        for a in format.0 {
            unsafe {
                gl::EnableVertexAttribArray(a.location);
                gl::VertexAttribPointer(a.location, a.size, a.type_, a.normalized as u8, T::stride() as i32, a.offset as *const c_void);
            }
            println!("{:?}", a);

        }
    }

    pub fn enable_attrib(&self, index: u32) {
        unsafe { gl::EnableVertexAttribArray(index) };
    }

    pub fn attrib_pointer<T: GlAttrib>(&self, index: u32, stride: i32, start: i32) {
        T::setup_attrib(index, false, stride, start);
    }
}

impl GlBind for VertexArray {
    fn bind(&self) {
        unsafe { CURRENT_VAO = self.0 };
        unsafe { gl::BindVertexArray(self.0) };
    }

    fn unbind(&self) {
        unsafe {
            gl::BindVertexArray(0);
            CURRENT_VAO = 0;
        }
    }
}

impl GlTarget for VertexArray {
    fn target() -> u32 { gl::NONE }
    fn binding() -> u32 { gl::VERTEX_ARRAY_BINDING }
    fn current_bind() -> u32 { unsafe { CURRENT_VAO } }
}

impl GlObject for VertexArray {
    fn get_id(&self) -> u32 {
        self.0
    }
}

impl Drop for VertexArray {
    fn drop(&mut self) {
        unsafe { gl::DeleteVertexArrays(1, &self.0) };
    }
}

#[macro_export]
macro_rules! impl_vertexdata {
    ($Name: ident, $($field: ident),+) => {
        #[repr(C)]
        #[derive(Debug, Copy, Clone)]
        pub struct $Name($($field),+);
        
        impl VertexData for $Name {
            type Output = $Name;
            fn get_format() -> VertexFormat {
                VertexFormatBuilder::new()
                    $(.push::<$field>(false))+
                    .build()
            }
        }
    };
}
