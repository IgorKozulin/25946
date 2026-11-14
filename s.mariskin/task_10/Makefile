CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -Wpedantic -O2

.PHONY: all clean
all: run_command

run_command: run_command.c
	$(CC) $(CFLAGS) -o run_command run_command.c

clean:
	rm -f run_command
