# Makefile for compiling main.c

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c11
TARGET = hip
.PHONY: all clean

all: $(TARGET)

$(TARGET): main.o
	$(CC) $(CFLAGS) -o hip main.o

main.o: main.c
	$(CC) $(CFLAGS) -c main.c

clean:
	rm -f $(TARGET) main.o
