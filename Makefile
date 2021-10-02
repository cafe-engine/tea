CC = cc
AR = ar
CFLAGS = -Wall -std=c99 -O2 -g
LDFLAGS = -lm

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

SRCS = tea.c
OBJS = $(SRCS:.c=.o)

LIBNAME = libtea
SLIBNAME = $(LIBNAME).a
DLIBNAME = $(LIBNAME).so

GFX_LIB = -lSDL2

.PHONY: all build
.SECONDARY: main.c tea.c

build: main.c $(SLIBNAME)
	$(CC) main.c -o tea -L. -ltea $(CFLAGS) $(LDFLAGS) $(GFX_LIB)

$(SLIBNAME): $(OBJS)
	$(AR) rcs $(SLIBNAME) $(OBJS)

%.o: %.c
	$(CC) -c $< -o $@ $(CFLAGS)

clean:
	rm -f *.o tea $(SLIBNAME)