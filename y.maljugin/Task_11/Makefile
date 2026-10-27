CC = gcc
CFLAGS = -std=c99 -D_POSIX_C_SOURCE=200809L -Wall -Wextra -O2
TARGET = execvpe_demo

all: $(TARGET)

$(TARGET): execvpe_demo.c execvpe.c
	$(CC) $(CFLAGS) -o $(TARGET) execvpe_demo.c execvpe.c

clean:
	rm -f $(TARGET)

.PHONY: all clean
