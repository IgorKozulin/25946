# Перенаправляем временные файлы компилятора в текущую папку
export TMPDIR := $(CURDIR)
export TMP    := $(CURDIR)
export TEMP   := $(CURDIR)

CC      = gcc
CFLAGS  = -D_POSIX_C_SOURCE=200809L -std=c99 -Wall -Wextra -Wpedantic -O2

# Определяем расширение .exe для Windows
ifeq ($(OS),Windows_NT)
    EXE := .exe
    RM  := del /Q
else
    EXE :=
    RM  := rm -f
endif

TARGET = ex_time$(EXE)
SRC    = ex_time.c
OBJ    = ex_time.o
DEP    = ex_time.d

.PHONY: all run clean rebuild

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ)

# -MMD -MP генерирует .d-файл со списком зависимостей от заголовков
$(OBJ): $(SRC)
	$(CC) $(CFLAGS) -MMD -MP -c $(SRC) -o $(OBJ)

-include $(DEP)

# Запуск: make run CITY=Europe/Moscow
CITY ?= Europe/Moscow
run: $(TARGET)
	./$(TARGET) $(CITY)

clean:
	$(RM) $(OBJ) $(DEP) $(TARGET)

rebuild: clean allssh n.kalugin1@ccfit.nsu.ru