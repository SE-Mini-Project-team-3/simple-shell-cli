CC = gcc
CFLAGS = -std=c11 -D_DEFAULT_SOURCE -Wall -Wextra -Wpedantic -Iinclude
TARGET = simple-shell
TEST_TARGET = tests/test_parser

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

# Parser test target
$(TEST_TARGET): tests/test_parser.c src/parser.c
	$(CC) $(CFLAGS) -o $@ tests/test_parser.c src/parser.c

# Combined test target: runs both parser tests and your executor tests!
test: all $(TEST_TARGET)
	@echo "=== Running Parser Tests ==="
	./$(TEST_TARGET)
	@echo "=== Running Executor Tests ==="
	$(CC) $(CFLAGS) src/test_executor.c src/executor.c -o test_executor
	./test_executor
	rm -f test_executor

clean:
	rm -f $(OBJ) $(TARGET) $(TEST_TARGET) test_executor