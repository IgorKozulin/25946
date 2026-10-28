CC = gcc
CFLAGS = -std=c99 -D_POSIX_C_SOURCE=200809L -Wall -Wextra -O2
TARGET = my_ls

all: $(TARGET)

$(TARGET): my_ls.c
	$(CC) $(CFLAGS) -o $(TARGET) my_ls.c

clean:
	rm -f $(TARGET)

.PHONY: all clean
