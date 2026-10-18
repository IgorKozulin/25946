CC = gcc
CFLAGS = -O2
TARGET = read_files_alarm
SRC = read_files_alarm.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)

.PHONY: all clean
