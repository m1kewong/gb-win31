# GBDK C Compiler
CC = /tmp/gbdk/bin/lcc
# Linker
LINK = /tmp/gbdk/bin/lcc
CFLAGS=-Wl-j -Wm-yC -Wm-yc -Iinclude

all: win31.gbc

# Add all your .c files here
SRCS = src/main.c src/ui.c src/minesweeper.c src/paint.c src/sound.c src/tiles.c
OBJS = $(SRCS:.c=.o) # Not strictly needed for lcc direct to .gbc, but good practice for larger projects

all: win31.gbc

win31.gbc: $(SRCS) include/tiles.h include/ui.h include/minesweeper.h include/paint.h include/sound.h
	$(CC) $(CFLAGS) -o win31.gbc $(SRCS)

clean:
	rm -f win31.gbc
	rm -f src/*.o