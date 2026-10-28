CC = gcc
CFLAGS = -std=c99 -D_POSIX_C_SOURCE=200809L -Wall -Wextra -O2
TARGET = match_files

all: $(TARGET)

$(TARGET): match_files.c pattern_match.h
	$(CC) $(CFLAGS) -o $(TARGET) match_files.c

clean:
	rm -f $(TARGET)

.PHONY: all clean
