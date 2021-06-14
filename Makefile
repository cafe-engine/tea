NAME = tea
CC = cc
SRC = $(NAME).c external/GL/gl3w.c
OBJ = $(SRC:%.c=%.o)

INCLUDE = -I. -Iexternal

CFLAGS = -Wall -std=c99 `sdl2-config --cflags`
LFLAGS = -L. -ltea -lm `sdl2-config --libs` -lGL

LIBNAME = lib$(NAME)
SLIBNAME = $(LIBNAME).a
DLIBNAME = $(LIBNAME).so

.PHONY: build clean
.SECONDARY: $(OBJ) $(SLIBNAME)

build: $(SLIBNAME)

hello: $(SLIBNAME) examples/hello/main.o
	@echo "*******************************************************"
	@echo "** COMPILING $@"
	@echo "*******************************************************"
	$(CC) examples/hello/main.o -o hello $(INCLUDE) $(CFLAGS) $(LFLAGS)

%.a: $(OBJ)
	@echo "*******************************************************"
	@echo " ** CREATING $@"
	@echo "*******************************************************"
	$(AR) rcs $@ $(OBJ)

%.o: %.c
	@echo "*******************************************************"
	@echo "** COMPILING SOURCE $<"
	@echo "*******************************************************"
	$(CC) -c $< -o $@ $(INCLUDE) $(CFLAGS)

clean:
	rm -f $(OBJ)
	rm -f $(SLIBNAME)
	rm -f $(DLIBNAME)
	rm -f hello
