export TMPDIR := $(CURDIR)
export TMP := $(CURDIR)
export TEMP := $(CURDIR)

CC = gcc
CFLAGS = -D_POSIX_C_SOURCE=200809L -std=c99 -Wall -Wextra -Wpedantic -O2

TARGET = setuid_test

.PHONY: all clean

all: $(TARGET)

$(TARGET): setuid_test.c
	$(CC) $(CFLAGS) -o $(TARGET) setuid_test.c

clean:
	rm -f $(TARGET)