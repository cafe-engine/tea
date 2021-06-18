NAME = tea
CC = cc
SRC = $(NAME).c # external/GL/gl3w.c
OBJ = $(SRC:%.c=%.o)
DOBJ = $(SRC:%.c=%.do)

LIBNAME = lib$(NAME)
LIB_EXT = so

CFLAGS = -Wall -std=c99
LFLAGS =

OUT = hello
CLEAN_FILES = 

ifeq ($(TARGET),Web)
    CLEAN_FILES = $(NAME).wasm $(NAME).js
endif

ifeq ($(OS),Windows_NT)
    TARGET ?= Windows
    LIB_EXT = dll
else
    UNAME_S = $(shell uname -s)
    ifeq ($(UNAME_S),Darwin)
    TARGET ?= OSX
    LIB_EXT = dylib
    else
    TARGET ?= $(UNAME_S)
    endif
endif

ifeq ($(TARGET),Windows)
LIB_EXT = dll
endif
ifeq ($(TARGET),Darwin)
LIB_EXT = dylib
endif

INCLUDE += -I. -Iexternal

include cross/Makefile.$(TARGET)

SLIBNAME = $(LIBNAME).a
DLIBNAME = $(LIBNAME).$(LIB_EXT)

.PHONY: all build clean
.SECONDARY: $(OBJ) $(SLIBNAME)

build: $(OUT)

all: $(OUT) $(SLIBNAME) $(DLIBNAME)

$(OUT): $(SLIBNAME) examples/hello/main.o
	@echo "*******************************************************"
	@echo "** COMPILING $@"
	@echo "*******************************************************"
	$(CC) examples/hello/main.o -o $(OUT) $(INCLUDE) $(CFLAGS) -L. -ltea $(LFLAGS)

$(SLIBNAME): $(OBJ)
	@echo "*******************************************************"
	@echo " ** CREATING $@"
	@echo "*******************************************************"
	$(AR) rcs $@ $(OBJ)

$(DLIBNAME): $(DOBJ)
	@echo "*******************************************************"
	@echo " ** CREATING $@ $<"
	@echo "*******************************************************"
	$(CC) -shared -o $@ $(DOBJ) $(INCLUDE) $(CFLAGS) $(LFLAGS)

%.o: %.c
	@echo "*******************************************************"
	@echo "** COMPILING SOURCE $<"
	@echo "*******************************************************"
	$(CC) -c $< -o $@ $(INCLUDE) $(CFLAGS)

%.do: %.c
	@echo "*******************************************************"
	@echo "** COMPILING SOURCE $<"
	@echo "*******************************************************"
	$(CC) -c $< -o $@ -fPIC $(INCLUDE) $(CFLAGS)

clean:
	rm -f $(OBJ) $(DOBJ)
	rm -f $(SLIBNAME)
	rm -f $(DLIBNAME)
	rm -f $(OUT) $(CLEAN_FILES)
	rm -f examples/hello/main.o
