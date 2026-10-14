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
	rm -f $(OBJ) $(TARGET) tests/test_history

tests/test_history: tests/test_history.c src/history.c
	$(CC) $(CFLAGS) -o $@ tests/test_history.c src/history.c

test: tests/test_history
	./tests/test_history
