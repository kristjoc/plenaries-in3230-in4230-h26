CC = gcc
CFLAGS = -I include -I libq -Wall

src = $(wildcard src/*.c)
obj = $(patsubst src/%.c, build/%.o, $(src))
headers = $(wildcard include/*.h) libq/queue.h

# Ensure build directory exists before compiling
build:
	mkdir -p build

# Main target
hip: $(obj) build/queue.o
	$(CC) $(CFLAGS) $(obj) build/queue.o -o hip

# Compile object files
build/%.o: src/%.c ${headers} | build
	$(CC) $(CFLAGS) -c $< -o $@

build/queue.o: libq/queue.c ${headers} | build
	$(CC) $(CFLAGS) -c $< -o $@

# All target
all: build hip install

# Install target
install:
	mkdir -p bin
	mv hip bin/

# Clean target
clean:
	rm -rf build/ bin/
