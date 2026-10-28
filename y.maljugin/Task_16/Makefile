CC = gcc
CFLAGS = -std=c99 -D_POSIX_C_SOURCE=200809L -Wall -Wextra -O2
TARGET = one_key

all: $(TARGET)

$(TARGET): one_key.c
	$(CC) $(CFLAGS) -o $(TARGET) one_key.c

clean:
	rm -f $(TARGET)

.PHONY: all clean
