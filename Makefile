CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -Iinclude
TARGET = simple-shell

SRC = src/main.c \
      src/parser.c \
      src/executor.c \
      src/pipeline.c \
      src/history.c \
      src/env_manager.c

OBJ = $(SRC:.c=.o)

.PHONY: all clean test

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ)

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET) test_executor

test: src/test_executor.c src/executor.c
	$(CC) $(CFLAGS) src/test_executor.c src/executor.c -o test_executor
	./test_executor
	rm -f test_executor