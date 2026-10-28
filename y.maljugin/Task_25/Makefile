CC = gcc
CFLAGS = -std=c99 -D_POSIX_C_SOURCE=200809L -Wall -Wextra -O2
TARGET = pipe_upper

all: $(TARGET)

$(TARGET): pipe_upper.c
	$(CC) $(CFLAGS) -o $(TARGET) pipe_upper.c

clean:
	rm -f $(TARGET)

.PHONY: all clean
