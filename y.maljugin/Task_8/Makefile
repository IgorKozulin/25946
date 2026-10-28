CC = gcc
CFLAGS = -std=c99 -D_POSIX_C_SOURCE=200809L -Wall -Wextra -O2
TARGET = locked_editor

all: $(TARGET)

$(TARGET): locked_editor.c
	$(CC) $(CFLAGS) -o $(TARGET) locked_editor.c

clean:
	rm -f $(TARGET)

.PHONY: all clean
