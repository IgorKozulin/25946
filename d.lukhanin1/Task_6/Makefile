CC = gcc
CFLAGS = -std=c11 -D_POSIX_C_SOURCE=200809L -D_FILE_OFFSET_BITS=64 -Wall -Wextra -Wpedantic -O2
TARGET = line_lookup_alarm

all: $(TARGET)

$(TARGET): line_lookup_alarm.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ line_lookup_alarm.c

clean:
	rm -f $(TARGET)

.PHONY: all clean
