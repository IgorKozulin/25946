CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -Wpedantic -O2
TARGET = access_time
SRC = access_time.c

.PHONY: all clean

# собираем программу из исходника
all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

# удаляем результат сборки
clean:
	rm -f $(TARGET)
