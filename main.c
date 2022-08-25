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
    // te_config_t config = tea_config();
    tea_init(NULL);

    // te_shader_t shader = tea_shader(NULL, NULL);
    te_buffer_t buffer = tea_buffer(TEA_VERTEX_BUFFER, 1000);
    te_program_t prog = tea_program(NULL, NULL);
    i32 world, modelview;
    world = tea_program_uniform_location(prog, "u_World");
    modelview = tea_program_uniform_location(prog, "u_ModelView");

    // fprintf(stderr, "world location: %d\n", world);
    // fprintf(stderr, "modelview location: %d\n", modelview);

    u8 pixels[] = {
        255, 255, 255, 255
    };
    te_texture_t tex = tea_texture(TEA_RGBA, 1, 1, pixels, 0);
	// tea_batch_fill_triangle(batch, (vec2){0.0, 0.5}, (vec2){-0.5, -0.5}, (vec2){0.5, -0.5});
    float aspect = 640.f / 380.f;
    float angle = TEA_DEG2RAD(45);

    tea_matrix_mode(TEA_PERSPECTIVE);
    tea_load_identity();
    tea_ortho(0, 640, 380, 0, 0, 1);
    // tea_perspective(angle, aspect, 0.1, 500);
    tea_matrix_mode(TEA_MODELVIEW);
    tea_load_identity();

    tea_setup_buffer(buffer);
    tea_bind_texture(tex);

    tea_buffer_color4f(buffer, 1, 1, 1, 1);
    tea_buffer_fill_triangle(buffer, (vec2){16, 16}, (vec2){0, 32}, (vec2){32, 32});
    tea_buffer_flush(buffer);
    float vertices[] = {
        // positions          // colors           // texture coords
         0.5f,  0.5f,   0.0f, 1.0f, 0.0f, 1.0f,   1.0f, 1.0f, // top right
         0.5f, -0.5f,   0.0f, 0.0f, 1.0f, 1.0f,   1.0f, 0.0f, // bottom right
        -0.5f, -0.5f,   1.0f, 0.0f, 0.0f, 1.0f,   0.0f, 0.0f, // bottom left

         0.5f,  0.5f,   0.0f, 1.0f, 0.0f, 1.0f,   1.0f, 1.0f, // top right
         0.5f, -0.5f,   0.0f, 0.0f, 1.0f, 1.0f,   1.0f, 0.0f, // bottom right
        -0.5f,  0.5f,   1.0f, 1.0f, 1.0f, 1.0f,   0.0f, 1.0f  // top left
    };
    // tea_buffer_send_vertices(buffer, 6, vertices);
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
        // tea_begin();
        // tea_clear(TEA_RGB(0, 0, 0));
        // tea_set_shader(shader);
        // tea_batch_draw(batch);
        // tea_end();
        // tea_begin();
        // tea_color4f(1.f, 0.f, 1.f, 1.f);
        // tea_vertex2f(32, 32);
        // tea_vertex2f(0, 0);
        // tea_vertex2f(0, 32);
        // tea_vertex2f(0, 0);
        // tea_vertex2f(32, 32);
        // tea_vertex2f(32, 0);
        // tea_end();
        SDL_GL_SwapWindow(window);
    }

    tea_quit();
    return 0;
}
