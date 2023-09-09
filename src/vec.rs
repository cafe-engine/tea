use std::ops::{Add, AddAssign, Sub, SubAssign, Mul, MulAssign};

use crate::{GlEnum, GlAttrib, GlUniform, GlslType, impl_glsl};

macro_rules! impl_vec {
    ($Name: ident, $($field: ident),+; $size: expr) => {
        #[repr(C)]
        #[derive(Default, Debug, Clone, Copy, PartialEq)]
        pub struct $Name<T> { $(pub $field: T),+ }

        impl<T> $Name<T> {
            pub fn new($($field: T),+) -> $Name<T> {
                $Name { $($field: $field),+ }
            }

            // pub fn from_array(data: [T; $size])-> $Name<T> where T: Default {
            //     let mut vec = $Name::default();
            //     let ptr = &vec;
            //     vec
            // }

            pub fn as_array(&self) -> [&T; $size] {
                [$(&self.$field),+]
            }

            // pub fn dot(&self, rhs: &Self) -> T {
            //     let sum: T = ($(&self.$field * rhs.$field),+);
            //     sum
            // }

            /* pub fn from_value(v: T) -> $Name<T> {
                $Name { $($field: v),+ }
            } */
            pub fn stride() -> usize { $size * std::mem::size_of::<T>() }
        }

        impl<T: Add> Add<$Name<T> > for $Name<T>
            where T: Add<Output = T>
        {
            type Output = $Name<T>;
            fn add(self, rhs: $Name<T>) -> Self::Output {
                $Name::new($(self.$field + rhs.$field),+)
            }
        }

        impl<T: Sub> Sub<$Name<T> > for $Name<T>
            where T: Sub<Output = T>
        {
            type Output = $Name<T>;
            fn sub(self, rhs: $Name<T>) -> Self::Output {
                $Name::new($(self.$field - rhs.$field),+)
            }
        }

        impl<T: AddAssign> AddAssign<$Name<T> > for $Name<T> {
            fn add_assign(&mut self, rhs: Self) {
                $(self.$field += rhs.$field);+
            }
        }

        impl<T: SubAssign> SubAssign<$Name<T> > for $Name<T> {
            fn sub_assign(&mut self, rhs: Self) {
                $(self.$field -= rhs.$field);+
            }
        }

        impl<T: GlEnum> GlEnum for $Name<T> {
            fn gl_enum() -> u32 { T::gl_enum() }

            fn to_enum(&self) -> u32 { T::gl_enum() }
        }

        impl<T: GlEnum> GlAttrib for $Name<T> {
            fn size() -> u32 { $size }
        }

        impl<T: GlUniform> GlUniform for $Name<T> {
            fn uniform(location: i32, val: &Self) {
                Self::uniformv(location, $size, val as *const Self);
            }
            
            fn uniformv(location: i32, count: i32, ptr: *const Self) {
                T::uniformv(location, count * $size, ptr as *const T);
            }

            fn send_uniform(&self, location: i32) {
                let ptr: *const T = &self.x;
                T::uniformv(location, $size, ptr);
            }
        }
    }
}

pub trait UniformType {
    fn get_type(&self) -> u32;
}

impl_vec!(Vector2, x, y; 2);
impl_vec!(Vector3, x, y, z; 3);
impl_vec!(Vector4, x, y, z, w; 4);

pub type BVec2 = Vector2<bool>;
pub type IVec2 = Vector2<i32>;
pub type UVec2 = Vector2<u32>;
pub type Vec2 = Vector2<f32>;

impl_glsl!(BVec2, "bvec2");
impl_glsl!(IVec2, "ivec2");
impl_glsl!(UVec2, "uvec2");
impl_glsl!(Vec2, "vec2");

pub type BVec3 = Vector3<bool>;
pub type IVec3 = Vector3<i32>;
pub type UVec3 = Vector3<u32>;
pub type Vec3 = Vector3<f32>;

impl_glsl!(BVec3, "bvec3");
impl_glsl!(IVec3, "ivec3");
impl_glsl!(UVec3, "uvec3");
impl_glsl!(Vec3, "vec3");

pub type BVec4 = Vector4<bool>;
pub type IVec4 = Vector4<i32>;
pub type UVec4 = Vector4<u32>;
pub type Vec4 = Vector4<f32>;

impl_glsl!(BVec4, "bvec4");
impl_glsl!(IVec4, "ivec4");
impl_glsl!(UVec4, "uvec4");
impl_glsl!(Vec4, "vec4");

impl Vec3 {
    pub fn zero() -> Self {
        Self { x: 0.0, y: 0.0, z: 0.0 }
    }

    pub fn up() -> Self { Self { x: 0.0, y: 1.0, z: 0.0 } }
    pub fn down() -> Self { Self { x: 0.0, y: -1.0, z: 0.0 } }
    pub fn front() -> Self { Self { x: 0.0, y: 0.0, z: -1.0 } }
    pub fn back() -> Self { Self { x: 0.0, y: 0.0, z: 1.0 } }

    pub fn from_array(data: [f32; 4]) -> Self {
        Vec3 { x: data[0], y: data[1], z: data[2] }
    }

    pub fn dot(&self, rhs: &Self) -> f32 {
        self.x*rhs.x + self.y*rhs.y + self.z*rhs.z
    }

    pub fn len(&self) -> f32 {
        self.x*self.x + self.y*self.y + self.z*self.z
    }

    pub fn cross(&self, other: &Self) -> Self {
        let x = self.y*other.z - self.z*other.y;
        let y = self.z*other.x - self.x*other.z;
        let z = self.x*other.y - self.y*other.x;
        Vec3::new(x, y, z)
    }

    pub fn normalize(&self) -> Self {
        let len = self.len().sqrt();
        Vec3::new(self.x / len, self.y / len, self.z / len)
    }

    pub fn scalar(&self, val: f32) -> Self {
        Vec3::new(self.x * val, self.y * val, self.z * val)
    }
}

impl Vec4 {
    pub fn zero() -> Vec4 {
        Self { x: 0.0, y: 0.0, z: 0.0, w: 0.0 }
    }

    pub fn from_array(data: [f32; 4]) -> Vec4 {
        Vec4 { x: data[0], y: data[1], z: data[2], w: data[3] }
    }

    pub fn dot(&self, rhs: &Self) -> f32 {
        self.x*rhs.x + self.y*rhs.y + self.z*rhs.z + self.w*rhs.w
    }

    pub fn len(&self) -> f32 {
        self.x*self.x + self.y*self.y + self.z*self.z + self.w*self.w
    }

    pub fn cross(&self, other: &Self) -> Self {
        let x = self.y*other.z - self.z*other.y;
        let y = self.z*other.x - self.x*other.z;
        let z = self.x*other.y - self.y*other.x;
        Vec4::new(x, y, z, 0.0)
    }

    pub fn normalize(&self) -> Self {
        let len = self.len().sqrt();
        Vec4::new(self.x / len, self.y / len, self.z / len, self.w / len)
    }

    pub fn apply_matrix(&mut self, matrix: Mat4) {
        let aux = self.clone();
        let m = matrix.transpose();
        self.x = aux.dot(&m.data[0]);
        self.y = aux.dot(&m.data[1]);
        self.z = aux.dot(&m.data[2]);
        self.w = aux.dot(&m.data[3]);
    }
}

#[repr(C)]
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct Mat4 {
    pub data: [Vec4; 4],
}

impl Default for Mat4 {
    fn default() -> Self { Mat4::identity() }
}

impl Mat4 {
    pub fn new(data: [Vec4; 4]) -> Mat4 {
        Mat4 { data }
    }

    pub fn identity() -> Mat4 {
        let data: [Vec4; 4] = [
            Vec4::new(1.0, 0.0, 0.0, 0.0),
            Vec4::new(0.0, 1.0, 0.0, 0.0),
            Vec4::new(0.0, 0.0, 1.0, 0.0),
            Vec4::new(0.0, 0.0, 0.0, 1.0),
        ];
        Mat4 { data }
    }

    pub fn ortho(left: f32, right: f32, bottom: f32, top: f32, near: f32, far: f32) -> Mat4 {
        let mut data: [Vec4; 4] = [Vec4::zero(); 4];
        data[0].x = 2.0 / (right - left);
        data[0].y = 0.0;
        data[0].z = 0.0;
        data[0].w = 0.0;

        data[1].x = 0.0;
        data[1].y = 2.0 / (top - bottom);
        data[1].z = 0.0;
        data[1].w = 0.0;
        
        data[2].x = 0.0;
        data[2].y = 0.0;
        data[2].z = -2.0 / (far - near);
        data[2].w = 0.0;

        data[3].x = -(right + left) / (right - left);
        data[3].y = -(top + bottom) / (top - bottom);
        data[3].z = -(far + near) / (far - near);
        data[3].w = 1.0;
        Mat4 { data }
    }

    pub fn perspective(fov: f32, aspect: f32, near: f32, far: f32) -> Mat4 {
        let mut data: [Vec4; 4] = [Vec4::zero(); 4];
        let f = 1.0 / (fov / 2.0).tan();
        let inv_range = 1.0 / (near - far);

        data[0].x = f / aspect;
        data[0].y = 0.0;
        data[0].z = 0.0;
        data[0].w = 0.0;

        data[1].x = 0.0;
        data[1].y = f;
        data[1].z = 0.0;
        data[1].w = 0.0;
        
        data[2].x = 0.0;
        data[2].y = 0.0;
        data[2].z = (far + near) * inv_range;
        data[2].w = -1.0;

        data[3].x = 0.0;
        data[3].y = 0.0;
        data[3].z = 2.0 * far * near * inv_range;
        data[3].w = 0.0;
        Mat4 { data }
    }

    pub fn look_at(position: Vec3, target: Vec3, up: Vec3) -> Mat4 {
        let f = (target - position).normalize();
        let r = up.cross(&f).normalize();
        let u = f.cross(&r);
        
        let data: [Vec4; 4] = [
            Vec4::new(r.x, u.x, -f.x, 0.0),
            Vec4::new(r.y, u.y, -f.y, 0.0),
            Vec4::new(r.z, u.z, -f.z, 0.0),
            Vec4::new(-r.dot(&position), -u.dot(&position), f.dot(&position), 1.0),
        ];
        Mat4 { data }
    }

    pub fn from_array(data: [f32; 16]) -> Mat4 {
        let mut dt: [Vec4; 4] = [Vec4::zero(); 4];

        for i in 0..4 {
            let index = 4 * i;
            dt[i] = Vec4::new(data[index], data[index+1], data[index+2], data[index+3]);
        }
        Mat4 { data: dt }
    }

    pub fn translation_matrix(pos: Vec3) -> Mat4 {
        let mut mat = Mat4::identity();
        mat.data[3].x = pos.x;
        mat.data[3].y = pos.y;
        mat.data[3].z = pos.z;
        mat
    }

    pub fn scale_matrix(scale: Vec3) -> Mat4 {
        let mut mat = Mat4::identity();
        mat.data[0].x = scale.x;
        mat.data[1].y = scale.y;
        mat.data[2].z = scale.z;
        mat
    }

    pub fn rotation_matrix(axis: Vec3, angle: f32) -> Mat4 {
        let r = angle.to_radians();
        let mut m = Mat4::identity();
        m
    }

    pub fn rotation_matrix_z(angle: f32) -> Mat4 {
        let rad = angle.to_radians();
        let mut m = Mat4::identity();
        m.data[0].x = rad.cos();
        m.data[0].y = rad.sin();
        m.data[1].x = -rad.sin();
        m.data[1].y = rad.cos();
        m.data[2].z = 1.0;
        m.data[3].w = 1.0;
        m
    }

    pub fn transpose(&self) -> Mat4 {
        let mut data: [f32; 16] = [0.0; 16];

        let mut i = 0;
        for vec in self.data {
            data[i] = vec.x;
            data[4+i] = vec.y;
            data[8+i] = vec.z;
            data[12+i] = vec.w;
            i += 1;
        }
        Mat4::from_array(data)
    }

    pub fn translate(&mut self, v: Vec3) {
        let mat = Mat4::translation_matrix(v);
        *self *= mat;
    }

    pub fn scale(&mut self, scale: Vec3) {
        let mat = Mat4::scale_matrix(scale);
        *self *= mat;
    }

    pub fn rotate(&mut self, angle: f32) {
        let mat = Mat4::rotation_matrix_z(angle);
        *self *= mat;
    }
}

impl Mul for Mat4 {
    type Output = Mat4;
    fn mul(self, rhs: Self) -> Self::Output {
        let aux = rhs.transpose();
        let mut data: [Vec4; 4] = [Vec4::zero(); 4];
        let mut i = 0;
        for d in &mut data {
            d.x = self.data[i].dot(&aux.data[0]);
            d.y = self.data[i].dot(&aux.data[1]);
            d.z = self.data[i].dot(&aux.data[2]);
            d.w = self.data[i].dot(&aux.data[3]);
            i += 1;
        }

        Mat4 { data }
    }
}

impl MulAssign for Mat4 {
    fn mul_assign(&mut self, rhs: Self) {
        let aux = rhs.transpose();
        let mut data: [Vec4; 4] = [Vec4::zero(); 4];
        let mut i = 0;
        for d in &mut data {
            d.x = self.data[i].dot(&aux.data[0]);
            d.y = self.data[i].dot(&aux.data[1]);
            d.z = self.data[i].dot(&aux.data[2]);
            d.w = self.data[i].dot(&aux.data[3]);
            i += 1;
        }
        self.data = data;
    }
}

impl_glsl!(Mat4, "mat4");

impl GlUniform for Mat4 {
    fn uniform(location: i32, val: &Self) {
        Self::uniformv(location, 1, val);
    }
    
    fn uniformv(location: i32, count: i32, ptr: *const Self) {
        unsafe {
            gl::UniformMatrix4fv(location, count, gl::FALSE, ptr as *const f32);
        }
    }

    fn send_uniform(&self, location: i32) {
        Self::uniform(location, self);
    }
}
