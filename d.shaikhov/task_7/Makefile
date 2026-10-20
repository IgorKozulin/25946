CC = gcc
CFLAGS = -O2
TARGET = read_files_mmap
SRC = read_files_mmap.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)

.PHONY: all clean
