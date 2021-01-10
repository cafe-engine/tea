CC = gcc
SRC = src/tea.c
OUT = tea
CFLAGS = -std=c99 -Wall -lSDL2 -lm

LIBNAME = libtea

OBJS = $(SRC:%.c=%.o)

$(OUT): main.c $(LIBNAME).a
	$(CC) main.c -o $(OUT) -L. -ltea $(CFLAGS) $(INCLUDE)

$(LIBNAME).a: $(OBJS)
	ar rcs $(LIBNAME).a src/*.o

%.o: %.c
	$(CC) -c $< -o $@ $(INCLUDE) $(CFLAGS)

run: $(OUT)
	./$(OUT)

clean:
	rm -f $(OUT)
	rm -f $(LIBNAME).a
	rm -f $(OBJS)
