#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include "tea.h"

const char *vert =
"#version 120\n"
"uniform mat4 u_Projection;\n"
"uniform mat4 u_ModelView;\n"
"attribute vec3 in_Pos;"
"attribute vec4 in_Color;\n"
"varying vec4 fragColor;\n"
"void main() {\n"
"    fragColor = in_Color;\n"
"    gl_Position = u_Projection * u_ModelView * vec4(in_Pos, 1.0);\n"
"}\n";

const char *frag =
"#version 120\n"
"varying vec4 fragColor;\n"
"void main() {\n"
"    gl_FragColor = fragColor;\n"
"}\n";

int main(int argc, char **argv) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        return 1;
    }
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
    SDL_Window *window = SDL_CreateWindow("Tea", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 640, 480, SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL);
    if (window == NULL) {
        printf("SDL_CreateWindow Error: %s\n", SDL_GetError());
        return 1;
    }
    SDL_GLContext context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, context);
    SDL_Event event;

    te_config_t config = teaConfig(NULL);
    teaInit(&config);

    te_shader_t shader = teaShader(frag, vert);

    while (event.type != SDL_QUIT) {
        SDL_PollEvent(&event);
        teaClearColor(0.3, 0.4, 0.4, 1.0);
        teaClear();

        teaUseShader(shader);
        teaBegin(TEA_TRIANGLES);
        teaColor3f(1.f, 0.f, 0.f);
        teaVertex2f(-0.5f, -0.5f);
        teaColor3f(0.f, 1.f, 0.f);
        teaVertex2f(0.5f, -0.5f);
        teaColor3f(0.f, 0.f, 1.f);
        teaVertex2f(0.0f, 0.5f);
        teaEnd();
        teaUseShader(0);

        SDL_GL_SwapWindow(window);
    }

    return 0;
}