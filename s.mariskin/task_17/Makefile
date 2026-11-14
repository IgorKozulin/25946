CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -Wpedantic -O2

.PHONY: all clean
all: line_editor

line_editor: line_editor.c
	$(CC) $(CFLAGS) -o line_editor line_editor.c

clean:
	rm -f line_editor
