CC = cc
AR = ar
CFLAGS = -Wall -std=c99 -O2 -g
LDFLAGS = -lm
RELEASE = 0

NAME = tea

ifeq ($(OS),Windows_NT)
	CFLAGS += -D_WIN32
	LDFLAGS += -mwindows -lopengl32
else
	LDFLAGS += -ldl
	UNAME_S = $(shell uname -s)
	ifeq ($(UNAME_S),Darwin)
		LDFLAGS += -framework OpenGL
	endif
	ifeq ($(UNAME_S),Linux)
		LDFLAGS += -lGL
	endif
endif

ifeq ($(RELEASE),1)
	CFLAGS += -O2
endif

SRC = $(NAME).c
OBJ = $(SRC:.c=.o)
DOBJ = $(SRC:.c=.do)
OUT = $(NAME)
MAIN = main.c

LIBNAME = lib$(NAME)
SLIBNAME = $(LIBNAME).a
DLIBNAME = $(LIBNAME).so

GFX_LIB = -lSDL2 

.PHONY: all build
.SECONDARY: $(MAIN) $(NAME).c

build: $(OUT)

all: $(SLIBNAME) $(DLIBNAME)

$(OUT): $(MAIN) $(SLIBNAME)
	$(CC) $(MAIN) -o $(OUT) -L. -l$(NAME) $(CFLAGS) $(LDFLAGS) $(GFX_LIB)

$(SLIBNAME): $(OBJ)
	$(AR) rcs $(SLIBNAME) $(OBJ)

$(DLIBNAME): $(DOBJ)
	$(CC) -shared $^ -o $@ $(CFLAGS) $(LDFLAGS) $(GFX_LIB)

%.o: %.c
	$(CC) -c $< -o $@ $(CFLAGS)

%.do: %.c
	$(CC) -c $< -o $@ -fPIC $(CFLAGS)

clean:
	rm -f $(OBJ) $(DOBJ) $(OUT)
	rm -f $(SLIBNAME) $(DLIBNAME)
