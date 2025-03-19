CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -pthread
SRC = src/master.c src/player.c src/view.c
BIN = bin/master bin/player bin/view

all: $(BIN)

bin/master: src/master.c
	$(CC) $(CFLAGS) -o $@ $<

bin/player: src/player.c
	$(CC) $(CFLAGS) -o $@ $<

bin/view: src/view.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(BIN)

.PHONY: all clean
