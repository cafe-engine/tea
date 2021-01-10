# Tea

a tiny c lib that wraps SDL2 (and possibly GLFW in the future) for handle input, render and window.

The idea is to create a modular render, that can be compiled with different renders. 
Tea use a command based context, and push `Draw` and `Stack` commands to the command stack.

A Stack command push/pop a Canvas/Shader/Transform to/from their respective stacks.

## TODOS:


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
- [ ] Wrap for SDL_Renderer
	- [ ] primitives
		- [x] circle
		- [ ] triangle
		- [ ] polygon
 	- [ ] CPU transforms
- [ ] OpenGL batched render
 	- [ ] primitives
		- [ ] circle
		- [ ] polygon
 	- [ ] shaders
 	- [ ] framebuffers
 	- [ ] coordinate system (GPU transforms)
