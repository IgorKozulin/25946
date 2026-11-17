CC = gcc
CFLAGS = -std=c11 -D_POSIX_C_SOURCE=200809L -D_FILE_OFFSET_BITS=64 -Wall -Wextra -Wpedantic -O2
TARGET = line_editor

all: $(TARGET)

$(TARGET): line_editor.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ line_editor.c

test: $(TARGET)
	python3 test_editor.py

clean:
	rm -f $(TARGET)

.PHONY: all test clean
