CC = gcc
CFLAGS = -std=c99 -D_POSIX_C_SOURCE=200809L -Wall -Wextra -O2
TARGET = line_editor

all: $(TARGET)

$(TARGET): line_editor.c
	$(CC) $(CFLAGS) -o $(TARGET) line_editor.c

clean:
	rm -f $(TARGET)

.PHONY: all clean
