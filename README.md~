# Tea

a tiny c lib that wraps SDL2 (and possibly GLFW in the future) for handle input, render and window.

The idea is to create a modular render, that can be compiled with different renders. 
Tea use a command based context, and push `Draw` and `Stack` commands to the command stack.

A Stack command push/pop a Canvas/Shader/Transform to/from their respective stacks.

## TODOS:

- [x] Load images (stb_image.h)
- [x] Load fonts (stb_truetype.h)
- [ ] Window
	- [ ] wrap SDL get and set functions (title, width, height, fullscreen, resizable, ...)
- [ ] Input
	- [ ] mouse input
	- [ ] keyboard input
	- [ ] joystick input
- [ ] Software Render
	- [ ] primitives
		- [ ] point
		- [ ] line
		- [ ] rectangle
		- [ ] circle
		- [ ] triangle
		- [ ] polygon
	- [ ] textures
 	- [ ] render texture
 	- [ ] render text
- [ ] Wrap for SDL_Renderer
	- [ ] primitives
		- [x] circle
		- [x] triangle
		- [ ] polygon
 	- [ ] CPU transforms
 	- [ ] render text
- [ ] OpenGL batched render
 	- [ ] primitives
		- [ ] circle
		- [ ] polygon
 	- [ ] shaders
 	- [ ] framebuffers
 	- [ ] coordinate system (GPU transforms)
 	- [ ] render text
