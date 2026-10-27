CC = gcc
CFLAGS = -std=c99 -D_POSIX_C_SOURCE=200809L -Wall -Wextra -O2
TARGET = beeper

all: $(TARGET)

$(TARGET): beeper.c
	$(CC) $(CFLAGS) -o $(TARGET) beeper.c

clean:
	rm -f $(TARGET)

.PHONY: all clean
