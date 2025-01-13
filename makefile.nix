CFLAGS = -Wall
LDFLAGS = -lSDL2 -lSDL2_ttf

all: gameoflife

gameoflife: main.o gameoflife.o gameoflife_sdl.o
	$(CC) $^ -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o gameoflife