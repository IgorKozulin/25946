CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -Wpedantic -O2

TARGET = ex_time

.PHONY: all clean

all: $(TARGET)

$(TARGET): ex_time.c
	$(CC) $(CFLAGS) -o $(TARGET) ex_time.c

clean:
	rm -f $(TARGET)
