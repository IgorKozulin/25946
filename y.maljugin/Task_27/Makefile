CC = gcc
CFLAGS = -std=c99 -D_POSIX_C_SOURCE=200809L -Wall -Wextra -O2
TARGET = count_blank

all: $(TARGET)

$(TARGET): count_blank.c
	$(CC) $(CFLAGS) -o $(TARGET) count_blank.c

clean:
	rm -f $(TARGET)

.PHONY: all clean
