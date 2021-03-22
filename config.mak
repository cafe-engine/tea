NAME = tea
CSTD=c99
INCLUDE += -Iexternal
CFLAGS += `sdl2-config --cflags`
LFLAGS = `sdl2-config --libs` -lm -ldl
