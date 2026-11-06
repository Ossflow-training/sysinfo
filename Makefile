CC ?= cc
CFLAGS ?= -std=c11 -O2 -Wall -Wextra -Wpedantic -Wformat=2 -Wshadow -Wconversion -Wsign-conversion
SRC = src/sysinfo.c
DEST = bin/sysinfo

all: lint build run

lint: $(SRC)
	$(CC) $(CFLAGS) -fsyntax-only $(SRC)

build: $(SRC)
	mkdir -p $(dir $(DEST))
	$(CC) $(CFLAGS) -o $(DEST) $(SRC)

run: $(DEST)
	./$(DEST)

clean:
	rm -f $(DEST)

.PHONY: all lint build run clean
