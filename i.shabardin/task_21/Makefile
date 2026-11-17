CC = gcc
CFLAGS = -std=c99 -O2 -Wall -Wextra -pedantic
TARGET = beeper

all: $(TARGET)

$(TARGET): beeper.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(TARGET)

.PHONY: all clean
