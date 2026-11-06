CC = gcc
CFLAGS = -std=c11 -D_POSIX_C_SOURCE=200809L -Wall -Wextra -Wpedantic -Iinclude -fsanitize=address -fno-omit-frame-pointer
LDFLAGS = -fsanitize=address

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
	$(CC) $(CFLAGS) -o $@ $(OBJ) $(LDFLAGS)

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Parser tests
$(TEST_TARGET): tests/test_parser.c src/parser.c
	$(CC) $(CFLAGS) -o $@ tests/test_parser.c src/parser.c $(LDFLAGS)

# History tests
tests/test_history: tests/test_history.c src/history.c
	$(CC) $(CFLAGS) -o $@ tests/test_history.c src/history.c $(LDFLAGS)

# Environment manager tests
tests/test_env_manager: tests/test_env_manager.c src/env_manager.c
	$(CC) $(CFLAGS) -o $@ tests/test_env_manager.c src/env_manager.c $(LDFLAGS)

# Executor tests
test_executor: src/test_executor.c src/executor.c
	$(CC) $(CFLAGS) -o $@ src/test_executor.c src/executor.c $(LDFLAGS)

# Environment builtin integration tests
tests/test_env_integration: tests/test_env_integration.c
	$(CC) $(CFLAGS) -o $@ tests/test_env_integration.c $(LDFLAGS)

# Pipeline tests
tests/test_pipeline: tests/test_pipeline.c src/pipeline.c
	$(CC) $(CFLAGS) -o $@ tests/test_pipeline.c src/pipeline.c $(LDFLAGS)

# Parser specification tests
tests/test_parser_spec: tests/test_parser_spec.c src/parser.c
	$(CC) $(CFLAGS) -o $@ tests/test_parser_spec.c src/parser.c $(LDFLAGS)

# Combined test target
test: all $(TEST_TARGET) tests/test_history tests/test_env_manager test_executor tests/test_env_integration tests/test_pipeline tests/test_parser_spec
	@echo "=== Running Parser Tests ==="
	./$(TEST_TARGET)
	@echo "=== Running History Tests ==="
	./tests/test_history
	@echo "=== Running Environment Manager Tests ==="
	./tests/test_env_manager
	@echo "=== Running Executor Tests ==="
	./test_executor
	@echo "=== Running Environment Integration Tests ==="
	./tests/test_env_integration
	@echo "=== Running Pipeline Tests ==="
	./tests/test_pipeline
	@echo "=== Running Parser Specification Tests ==="
	./tests/test_parser_spec

# Standalone parser specification target
test-parser-spec: tests/test_parser_spec
	./tests/test_parser_spec

# Clean generated files
clean:
	rm -f $(OBJ) $(TARGET) $(TEST_TARGET) \
	      tests/test_history tests/test_env_manager \
	      tests/test_parser_spec test_executor \
	      tests/test_env_integration \
	      tests/.tmp_env_integration_input \
	      tests/.tmp_env_integration_output \
	      tests/test_pipeline