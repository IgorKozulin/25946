export TMPDIR := $(CURDIR)
export TMP := $(CURDIR)
export TEMP := $(CURDIR)

CC = gcc
CFLAGS = -D_POSIX_C_SOURCE=200809L -std=c99 -Wall -Wextra -Wpedantic -O2

TARGET = options

.PHONY: all clean

all: $(TARGET)

$(TARGET): options.c
	$(CC) $(CFLAGS) -o $(TARGET) options.c

clean:
	rm -f $(TARGET)