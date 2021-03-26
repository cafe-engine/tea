NAME = tea
RENDER = gl
CSTD=c99
INCLUDE += -Iexternal
SRC = src/back/tea_$(RENDER).c
SRC += external/GL/gl3w.c
CFLAGS += `sdl2-config --cflags`
LFLAGS = `sdl2-config --libs` -lm -ldl -lGL

CC = clang
