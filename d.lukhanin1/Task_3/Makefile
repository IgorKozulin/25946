CC = gcc
CFLAGS = -std=c11 -D_POSIX_C_SOURCE=200809L -Wall -Wextra -Wpedantic -O2
TARGET = access_file

all: $(TARGET) private

$(TARGET): access_file.c
	$(CC) $(CFLAGS) -o $@ $<

private:
	chmod 600 private.txt

clean:
	rm -f $(TARGET)

.PHONY: all clean private
