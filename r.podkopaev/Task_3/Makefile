CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -O2

all: setuid_file
	chmod 600 data.txt

setuid_file: setuid_file.c
	$(CC) $(CFLAGS) -o setuid_file setuid_file.c

clean:
	rm -f setuid_file

.PHONY: all clean
