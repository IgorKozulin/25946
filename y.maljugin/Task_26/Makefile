CC = gcc
CFLAGS = -std=c99 -D_POSIX_C_SOURCE=200809L -Wall -Wextra -O2
TARGET = popen_upper

all: $(TARGET)

$(TARGET): popen_upper.c
	$(CC) $(CFLAGS) -o $(TARGET) popen_upper.c

clean:
	rm -f $(TARGET)

.PHONY: all clean
