CC     = gcc
CFLAGS = -Wall -Wextra -Werror -std=c99
TARGET = rtp

SRC = main.c \
      libeth/eth.c \
      libhip/hip.c \
      librtp/rtp.c

OBJ = $(SRC:.c=.o)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJ)
