
CC = gcc
CFLAGS = -std=c11 -D_POSIX_C_SOURCE=200809L -Wall -Wextra -Wpedantic -Iinclude

TARGET = simple-shell
TEST_TARGET = tests/test_parser

SRC = src/main.c \
      src/parser.c \
      src/executor.c \
      src/pipeline.c \
      src/history.c \
      src/env_manager.c

OBJ = $(SRC:.c=.o)

.PHONY: all clean test test-parser-spec

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ)

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Parser tests
$(TEST_TARGET): tests/test_parser.c src/parser.c
	$(CC) $(CFLAGS) -o $@ tests/test_parser.c src/parser.c

# History tests
tests/test_history: tests/test_history.c src/history.c
	$(CC) $(CFLAGS) -o $@ tests/test_history.c src/history.c

# Environment manager tests
tests/test_env_manager: tests/test_env_manager.c src/env_manager.c
	$(CC) $(CFLAGS) -o $@ tests/test_env_manager.c src/env_manager.c

# Executor tests
test_executor: src/test_executor.c src/executor.c
	$(CC) $(CFLAGS) -o $@ src/test_executor.c src/executor.c

# Combined test target
test: all $(TEST_TARGET) tests/test_history tests/test_env_manager test_executor
	@echo "=== Running Parser Tests ==="
	./$(TEST_TARGET)
	@echo "=== Running History Tests ==="
	./tests/test_history
	@echo "=== Running Environment Manager Tests ==="
	./tests/test_env_manager
	@echo "=== Running Executor Tests ==="
	./test_executor

# Clean generated files
clean:
	rm -f $(OBJ) $(TARGET) $(TEST_TARGET) tests/test_history tests/test_env_manager tests/test_parser_spec test_executor

# US-01 parser acceptance specification
tests/test_parser_spec: tests/test_parser_spec.c src/parser.c
	$(CC) $(CFLAGS) -o $@ tests/test_parser_spec.c src/parser.c

test-parser-spec: tests/test_parser_spec
	./tests/test_parser_spec
