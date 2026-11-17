CC = gcc
CFLAGS = -std=c99 -O2 -Wall -Wextra -pedantic
TARGET = my_ls
SRC = my_ls.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)

.PHONY: all clean
