CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -pthread
LDFLAGS = -lrt
SRC = src/master.c src/player.c src/view.c
BIN = bin/master bin/player bin/view

all: $(BIN)

bin/master: src/master.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

bin/player: src/player.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

bin/view: src/view.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -f $(BIN)

.PHONY: all clean
