CC = gcc
CFLAGS = -std=c99 -D_POSIX_C_SOURCE=200809L -D__EXTENSIONS__ -Wall -Wextra -O2
TARGET = sorted_random

all: $(TARGET)

$(TARGET): sorted_random.c
	$(CC) $(CFLAGS) -o $(TARGET) sorted_random.c -lgen

clean:
	rm -f $(TARGET)

.PHONY: all clean
