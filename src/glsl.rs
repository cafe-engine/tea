extern crate gl;

use gl::types::{GLenum, GLint};
use std::ffi::{CStr, CString};
use crate::{GlEnum, GlUse, GlUniform, GlslType, GlObject};

#[derive(PartialEq, PartialOrd, Eq, Clone, Copy)]
pub enum ShaderType {
    VertexShader,
    FragmentShader,
    GeometryShader,
    ComputeShader,
}

impl GlEnum for ShaderType {
    fn to_enum(&self) -> GLenum {
        match *self {
            ShaderType::VertexShader => { gl::VERTEX_SHADER },
            ShaderType::FragmentShader => { gl::FRAGMENT_SHADER },
            _ => { 0 }
        }
    }
}

pub enum GlslVersion {
    V1_00,
    V1_20,
    V1_30,
    V1_40,
    V3_00,
    V3_30,
}

#[derive(Default, Debug, PartialEq, PartialOrd, Eq, Clone, Copy)]
pub enum GlslQualifier {
    Default,
    #[default]
    Const,
    Input,
    Output,
    Uniform
}

impl ToString for GlslQualifier {
    fn to_string(&self) -> String {
        match *self {
            Self::Default => { return "".to_string() },
            Self::Const => { return "const".to_string() },
            Self::Input => { return "in".to_string() },
            Self::Output => { return "out".to_string() },
            Self::Uniform => { return "uniform".to_string() }
        }
    }
}

#[derive(Default, Debug, PartialEq, Eq, Clone)]
pub struct GlslVariable {
    qualifier: GlslQualifier,
    type_: String,
    size: u32,
    name: String
}

impl GlslVariable {
    pub fn new<T: GlslType>(qualifier: GlslQualifier, name: &str) -> GlslVariable {
        GlslVariable { qualifier, type_: T::to_glsl(), size: 1, name: name.to_string() }
    }

    pub fn new_const<T: GlslType>(name: &str) -> GlslVariable {
        GlslVariable::new::<T>(GlslQualifier::Const, name)
    }
    
    pub fn new_input<T: GlslType>(name: &str) -> GlslVariable {
        GlslVariable::new::<T>(GlslQualifier::Input, name)
    }
    
    pub fn new_output<T: GlslType>(name: &str) -> GlslVariable {
        GlslVariable::new::<T>(GlslQualifier::Output, name)
    }
    
    pub fn new_uniform<T: GlslType>(name: &str) -> GlslVariable {
        GlslVariable::new::<T>(GlslQualifier::Uniform, name)
    }

    pub fn set_size(&mut self, size: u32) {
        debug_assert_ne!(size, 0);
        self.size = size;
    }
}

impl ToString for GlslVariable {
    fn to_string(&self) -> String {
        format!("{} {} {}", self.qualifier.to_string(), self.type_, self.name.to_string())
    }
}

#[derive(Default, Debug, Eq, PartialEq, PartialOrd, Ord)]
pub struct Shader(u32);

#[derive(Default, Debug, PartialEq, Eq)]
pub struct ShaderBuilder {
    version: String,
    precision: String,
    variables: Vec<GlslVariable>,
    pre_main: Vec<String>,
    main: String
}

#[derive(Default, Debug, Eq, PartialEq, PartialOrd, Ord)]
pub struct Program(u32);

/******************************
 * Shader
 ******************************/
impl Shader {
    pub fn new(kind: ShaderType) -> Result<Self, String> {
        let handle = unsafe { gl::CreateShader(kind.to_enum()) };
        Ok(Shader(handle))
    }

    pub fn source(&self, source: Vec<String>) {
        let v: Vec<CString> = source.iter()
            .map(|s| CString::new(s.as_str()).unwrap())
            .collect();
        let src: Vec<_> = v.iter()
            .map(|s| s.as_ptr())
            .collect();

        // println!("{} || {ptr:?} || {:?}", source.len(), source);
        #[cfg(target_arch="aarch64")]
        unsafe { gl::ShaderSource(self.0, source.len() as i32, src.as_ptr() as *const *const u8, std::ptr::null()) };
        #[cfg(not(target_arch="aarch64"))]
        unsafe { gl::ShaderSource(self.0, source.len() as i32, src.as_ptr() as *const *const i8, std::ptr::null()) };
    }
    
    pub fn compile(&self) -> Result<(), String> {
        unsafe { gl::CompileShader(self.0) };
        let mut success = 1;
        unsafe {
            gl::GetShaderiv(self.0, gl::COMPILE_STATUS, &mut success);
        }
        if success == 0 {
            let mut len = 0;
            unsafe { gl::GetShaderiv(self.0, gl::INFO_LOG_LENGTH, &mut len); }
            let error = create_whitespace_cstring_with_len(len as usize);
            unsafe {
                gl::GetShaderInfoLog(
                    self.0,
                    len,
                    std::ptr::null_mut(),
                    error.as_ptr() as *mut gl::types::GLchar
                );
            }
            return Err(error.to_string_lossy().into_owned());
        }
        Ok(())
    }
    
    pub fn from_source(kind: ShaderType, source: &CStr) -> Result<Self, String> {
        let shader = Self::new(kind).unwrap();
        unsafe { gl::ShaderSource(shader.get_id(), 1 as i32, &source.as_ptr(), std::ptr::null()) };
        shader.compile().expect("Failed to compile shader");
        Ok(shader)
    }

    // pub fn new(source: &CStr, kind: ShaderType) -> Result<Self, String> {
    //     let handle = unsafe { gl::CreateShader(kind.to_enum()) };
    //     unsafe {
    //         gl::ShaderSource(handle, 1, &source.as_ptr(), std::ptr::null());
    //         gl::CompileShader(handle);
    //     }
        
    //     Ok(Shader(handle))
    // }
}

impl GlObject for Shader {
    fn get_id(&self) -> u32 {
        self.0
    }
}

impl Drop for Shader {
    fn drop(&mut self) {
        unsafe { gl::DeleteShader(self.0) };
    }
}

/******************************
 * Program
 ******************************/

impl Program {

    pub fn new() -> Result<Self, String> {
        let handle = unsafe { gl::CreateProgram() };
        Ok(Program(handle))
    }

    pub fn attach_shader(&self, shader: &Shader) {
        unsafe { gl::AttachShader(self.0, shader.get_id()) };
    }

    pub fn link(&self) -> Result<(), String> {
        let mut success = 0;
        unsafe {
            gl::LinkProgram(self.0);
            gl::GetProgramiv(self.0, gl::LINK_STATUS, &mut success);
        }
        if success == 0 {
            let mut len: i32 = 0;
            unsafe { gl::GetProgramiv(self.0, gl::INFO_LOG_LENGTH, &mut len) };
            let error = create_whitespace_cstring_with_len(len as usize);
            unsafe {
                gl::GetProgramInfoLog(
                    self.0,
                    len,
                    std::ptr::null_mut(),
                    error.as_ptr() as *mut gl::types::GLchar
                );
            }
            return Err(error.to_string_lossy().into_owned());
        }
        Ok(())
    }

    pub fn from_shaders(shaders: &[Shader]) -> Result<Self, String> {
        let program = Self::new().unwrap();
        let handle = unsafe { gl::CreateProgram() };
        for shader in shaders {
            program.attach_shader(shader);
            unsafe { gl::AttachShader(handle, shader.get_id()); }
        }
        program.link().expect("Failed to link program");
        Ok(program)
    }

    pub fn from_source(vert_src: &CStr, frag_src: &CStr) -> Result<Self, String> {
        let vert_shd = Shader::from_source( ShaderType::VertexShader, vert_src).expect("Failed to create vertex shader");
        let frag_shd = Shader::from_source( ShaderType::FragmentShader, frag_src).expect("Failed to create fragment shader");
        let program = Self::new().unwrap();
        program.attach_shader(&vert_shd);
        program.attach_shader(&frag_shd);
        program.link().expect("Failed to link program");
        Ok(program)
    }

    pub fn get_attrib_location(&self, name: &str) -> GLint {
        #[cfg(target_arch="aarch64")]
        let loc = unsafe { gl::GetAttribLocation(self.0, name.as_ptr() as *const u8) };
        #[cfg(not(target_arch="aarch64"))]
        let loc = unsafe { gl::GetAttribLocation(self.0, name.as_ptr() as *const i8) };
        return loc
    }

    pub fn get_uniform_location(&self, name: &str) -> GLint {
        #[cfg(target_arch="aarch64")]
        unsafe { gl::GetUniformLocation(self.0, name.as_ptr() as *const u8) }
        #[cfg(not(target_arch="aarch64"))]
        unsafe { gl::GetUniformLocation(self.0, name.as_ptr() as *const i8) }
    }

    pub fn send_uniform<T: GlUniform>(&self, location: i32, data: T) {
        data.send_uniform(location);
    }

}

impl GlObject for Program {
    fn get_id(&self) -> u32 { self.0 }
}

impl GlUse for Program {
    fn set_used(&self) {
        unsafe { gl::UseProgram(self.0) };
    }

    fn set_unused(&self) {
        unsafe { gl::UseProgram(0) };
    }
}

impl Drop for Program {
    fn drop(&mut self) {
        unsafe { gl::DeleteProgram(self.0) };
    }
}

/******************************
 * Shader Builder
 ******************************/

impl ShaderBuilder {
    pub fn new() -> Self {
        ShaderBuilder { 
            version: "#version 140".to_string(),
            precision: "".to_string(),
            variables: vec![],
            pre_main: vec![],
            main: "".to_string()
        }
    }

    pub fn glsl_version(&mut self, version: &str) -> &mut Self {
        self.version = version.to_string();
        self
    }

    pub fn position<T: GlslType>(&mut self) -> &mut Self {
        self.variables.insert(0, GlslVariable::new_input::<T>( "a_Position"));
        self
    }

    pub fn color<T: GlslType>(&mut self) -> &mut Self {
        self.variables.insert(1, GlslVariable::new_input::<T>("a_Color"));
        self
    }

    pub fn texcoord<T: GlslType>(&mut self) -> &mut Self {
        self.variables.insert(2, GlslVariable::new_input::<T>("a_Texcoord"));
        self
    }

    pub fn normal<T: GlslType>(&mut self) -> &mut Self {
        self.variables.insert(3, GlslVariable::new_input::<T>("a_Normal"));
        self
    }

    pub fn add_input<T: GlslType>(&mut self, name: &str) -> &mut Self {
        self.variables.push(GlslVariable::new_input::<T>(name));
        self
    }

    pub fn add_output<T: GlslType>(&mut self, name: &str) -> &mut Self {
        self.variables.push(GlslVariable::new_output::<T>(name));
        self
    }

    pub fn add_uniform<T: GlslType>(&mut self, name: &str) -> &mut Self {
        self.variables.push(GlslVariable::new_uniform::<T>(name));
        self
    }

    pub fn push_pre_main(&mut self, src: &str) -> &mut Self {
        self.pre_main.push(src.to_string());
        self
    }

    pub fn set_main(&mut self, main: &str) -> &mut Self {
        self.main = main.to_string();
        self
    }

    pub fn build(&self, kind: ShaderType) -> Result<Shader, String> {
        let mut source: Vec<String> = vec![];

        source.push(format!("{}\n", self.version));
        source.push(format!("{}\n", self.precision));
        // source.push(self.precision.clone());

        // writeln!(source, "{}", self.version).unwrap();
        // write!(source, "{}", self.precision).unwrap();

        match kind {
            ShaderType::VertexShader {} => {},
            _ => {}
        }

        for variable in &self.variables {
            source.push(format!("{};", variable.to_string()));
            // writeln!(source, "{};", variable.to_string()).unwrap();
        }

        let src = self.pre_main.clone();
        source.extend(src);

        source.push(self.main.clone());
        
        // write!(source, "{}", self.main).unwrap();
        // println!("{:?}", source);
        // let src: &CStr = unsafe { CStr::from_ptr(source.as_ptr() as *const i8) };
        // Shader::from_source(kind, src)
        
        let shader = Shader::new(kind).unwrap();
        shader.source(source);
        shader.compile().expect("Failed to compile shader");
        Ok(shader)
    }
}

fn create_whitespace_cstring_with_len(len: usize) -> CString {
    let mut buffer: Vec<u8> = Vec::with_capacity(len + 1);
    buffer.extend([b' '].iter().cycle().take(len));
    unsafe { CString::from_vec_unchecked(buffer) }
}
