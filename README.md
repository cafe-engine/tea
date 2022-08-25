# Tea

OpenGL loader and simple immediate mode core versions

### using SDL2
```c
#include "tea.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <stdio.h>

int main(int argc, char **argv) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        return 1;
    }
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

    SDL_Window *window = SDL_CreateWindow("Tea", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 640, 380, SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (window == NULL) {
        printf("SDL_CreateWindow Error: %s\n", SDL_GetError());
        return 1;
    }
    SDL_GLContext context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, context);
    SDL_Event event;
    tea_init(NULL);

    te_buffer_t buffer = tea_buffer(TEA_VERTEX_BUFFER, 1000);
    te_program_t prog = tea_program(NULL, NULL);
    i32 world, modelview;
    world = tea_program_uniform_location(prog, "u_World");
    modelview = tea_program_uniform_location(prog, "u_ModelView");

    u8 pixels[] = {
        255, 255, 255, 255
    };
    te_texture_t tex = tea_texture(TEA_RGBA, 1, 1, pixels, 0);

    tea_matrix_mode(TEA_PERSPECTIVE);
    tea_load_identity();
    tea_ortho(0, 640, 380, 0, 0, 1);p
    tea_matrix_mode(TEA_MODELVIEW);
    tea_load_identity();

    tea_setup_buffer(buffer);
    tea_bind_texture(tex);

    while(event.type != SDL_QUIT) {
        SDL_PollEvent(&event);
        tea_viewport(0, 0, 640, 380);
        tea_clear(0.3f, 0.4f, 0.4f, 1.f);
        tea_buffer_seek(buffer, 0);
        tea_buffer_color4f(buffer, 1, 1, 1, 1);

        tea_use_program(prog);
        tea_program_set_uniform_matrix4fv(world, 1, TEA_FALSE, tea_get_matrix(TEA_PERSPECTIVE));
        tea_program_set_uniform_matrix4fv(modelview, 1, TEA_FALSE, tea_get_matrix(TEA_MODELVIEW));

        tea_buffer_fill_rectangle(buffer, (vec2){32, 32}, (vec2){32, 128});
        tea_buffer_flush(buffer);
        tea_draw(TEA_FILL);
        SDL_GL_SwapWindow(window);
    }

    tea_quit();
    return 0;
}
```

### using GLFW
```c
#include <stdio.h>
#include "tea.h"
#include <GLFW/glfw3.h>


int main(int argc, char **argv) {
    if (!glfwInit()) {
        fprintf(stderr, "Failed to initialize GLFW\n");
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    GLFWwindow *window = glfwCreateWindow(640, 480, "Hello World", NULL, NULL);
    if (!window) {
        fprintf(stderr, "Failed to open GLFW window.\n");
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    tea_init(NULL);

    te_buffer_t buffer = tea_buffer(TEA_VERTEX_BUFFER, 1000);
    te_program_t prog = tea_program(NULL, NULL);
    i32 world, modelview;
    world = tea_program_uniform_location(prog, "u_World");
    modelview = tea_program_uniform_location(prog, "u_ModelView");

    u8 pixels[] = {
        255, 255, 255, 255
    };
    te_texture_t tex = tea_texture(TEA_RGBA, 1, 1, pixels, 0);

    tea_matrix_mode(TEA_PERSPECTIVE);
    tea_load_identity();
    tea_ortho(0, 640, 380, 0, 0, 1);p
    tea_matrix_mode(TEA_MODELVIEW);
    tea_load_identity();

    tea_setup_buffer(buffer);
    tea_bind_texture(tex);


    while (!glfwWindowShouldClose(window)) {
        tea_viewport(0, 0, 640, 380);
        tea_clear(0.3f, 0.4f, 0.4f, 1.f);
        tea_buffer_seek(buffer, 0);
        tea_buffer_color4f(buffer, 1, 1, 1, 1);

        tea_use_program(prog);
        tea_program_set_uniform_matrix4fv(world, 1, TEA_FALSE, tea_get_matrix(TEA_PERSPECTIVE));
        tea_program_set_uniform_matrix4fv(modelview, 1, TEA_FALSE, tea_get_matrix(TEA_MODELVIEW));

        tea_buffer_fill_rectangle(buffer, (vec2){32, 32}, (vec2){32, 128});
        tea_buffer_flush(buffer);
        tea_draw(TEA_FILL);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    tea_quit();
    glfwTerminate();
    return 0;
}
```
