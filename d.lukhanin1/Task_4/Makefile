CC = gcc
CFLAGS = -std=c11 -D_POSIX_C_SOURCE=200809L -D_FILE_OFFSET_BITS=64 -Wall -Wextra -Wpedantic -O2
TARGET = list_strings

all: $(TARGET)

$(TARGET): list_strings.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ list_strings.c

clean:
	rm -f $(TARGET)

.PHONY: all clean
