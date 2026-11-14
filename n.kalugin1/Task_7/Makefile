export TMPDIR := $(CURDIR)
export TMP := $(CURDIR)
export TEMP := $(CURDIR)

CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -O2
TARGET = line_lookup_mmap
SRC = line_lookup_mmap.c

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)
