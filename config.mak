NAME = tea
TEA_BACKEND ?= sdl
CSTD=c99
INCLUDE += -Iexternal
SRC = src/back/tea_$(TEA_BACKEND).c
CFLAGS += `sdl2-config --cflags`

ifeq ($(OS),Windows_NT)
    LFLAGS += -Lexternal
    LFLAGS += -pthread -lm -mwindows
    LFLAGS += -lSDL2
else
    LFLAGS = `sdl2-config --libs` -lm -ldl
    ifeq ($(TEA_BACKEND),gl)
	SRC += external/GL/gl3w.c
	LFLAGS += -lGL
    endif
endif
