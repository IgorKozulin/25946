CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -Wpedantic -O2

.PHONY: all clean
all: process_manager

process_manager: process_manager.c
	$(CC) $(CFLAGS) -o process_manager process_manager.c

clean:
	rm -f process_manager
