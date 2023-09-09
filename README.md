# Tea

Tea is a lib wrapper for some OpenGL functions with some abstractions.

### using with SDL2

``` rust
extern crate sdl2;
extern crate tea;

use sdl2::event::Event;
use sdl2::keyboard::Scancode;
use tea::buffer::{ArrayBuffer, GlBuffer};
use tea::glsl::{Program, Shader};
use tea::vec::{Vec2, Vec4};
use tea::vertex::VertexArray;
use tea::{ClearFlags, GlBind, GlUse, glsl::ShaderType};

fn main() {
    let sdl = sdl2::init().unwrap();
    let video = sdl.video().unwrap();

    let window = video.window("Hello Window", 640, 480)
        .position_centered()
        .opengl()
        .build()
        .unwrap();

    let _ctx = window.gl_create_context().unwrap();
    tea::load_with(|name| video.gl_get_proc_address(name) as *const _);

    let mut event = sdl.event_pump().unwrap();

    let vert = Shader::new(ShaderType::VertexShader).unwrap();
    vert.source(vec!["
#version 140
in vec2 a_Position;
in vec4 a_Color;
out vec4 v_Color;
void main() {
    gl_Position = vec4(a_Position, 0.0, 1.0);
    v_Color = a_Color;
}
".to_string()]);
    vert.compile().expect("Failed to compile vertex shader");

    let frag = Shader::new(ShaderType::FragmentShader).unwrap();
    frag.source(vec!["
#version 140
in vec4 v_Color;
void main() {
    gl_FragColor = v_Color;
}
".to_string()]);
    frag.compile().expect("Failed to compile fragment shader");

    let program = Program::new().unwrap();
    program.attach_shader(&vert);
    program.attach_shader(&frag);
    program.link().expect("Failed to link program");

    let vao = VertexArray::new().unwrap();
    let vbo = ArrayBuffer::new();

    let data: Vec<f32> = vec![
        0.0, 0.5, 1.0, 0.0, 1.0, 1.0,
        0.5, -0.5, 1.0, 1.0, 0.0, 1.0,
        -0.5, -0.5, 0.0, 1.0, 1.0, 1.0,
    ];

    vao.bind();
    vbo.bind();
    vbo.data(&data, tea::buffer::BufferUsage::StaticDraw);
    vao.enable_attrib(0);
    vao.enable_attrib(1);
    let stride = (6 * std::mem::size_of::<f32>()) as i32;
    vao.attrib_pointer::<Vec2>(0, stride, 0);
    vao.attrib_pointer::<Vec4>(1, stride, (std::mem::size_of::<f32>() * 2) as i32);

    vao.unbind();
    vbo.unbind();


    'running: loop {
        tea::clear_color(0.3, 0.4, 0.4, 1.0);
        tea::clear(&[ClearFlags::ColorBufferBit]);

        program.set_used();
        vao.bind();
        tea::draw_arrays(tea::DrawMode::Triangles, 0, 3);
        vao.unbind();
        program.set_unused();
        window.gl_swap_window();
        for ev in event.poll_iter() {
            match ev {
                Event::Quit {..} | Event::KeyDown {scancode: Some(Scancode::Escape), ..} => {
                    break 'running
                },
                _ => {}
            }
        }
    }
}
```
