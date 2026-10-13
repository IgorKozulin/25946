CC = gcc
CFLAGS = -std=c99 -O2 -Wall -Wextra -pedantic
TARGET = setuid_test
SRC = setuid_test.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

prepare-demo: $(TARGET)
	chmod 600 data.txt
	chmod u+s $(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: all prepare-demo clean
