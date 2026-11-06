
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

# Environment builtin integration tests
tests/test_env_integration: tests/test_env_integration.c
	$(CC) $(CFLAGS) -o $@ tests/test_env_integration.c

# Pipeline tests
tests/test_pipeline: tests/test_pipeline.c src/pipeline.c
	$(CC) $(CFLAGS) -o $@ tests/test_pipeline.c src/pipeline.c

# Parser specification tests
tests/test_parser_spec: tests/test_parser_spec.c src/parser.c
	$(CC) $(CFLAGS) -o $@ tests/test_parser_spec.c src/parser.c

# Cross-module integration tests (drive the real shell binary)
tests/test_integration: tests/test_integration.c
	$(CC) $(CFLAGS) -o $@ tests/test_integration.c

# SEC-03 environment-variable exposure tests
tests/test_sec03_env_exposure: tests/test_sec03_env_exposure.c
	$(CC) $(CFLAGS) -o $@ tests/test_sec03_env_exposure.c

# Combined test target
test: all $(TEST_TARGET) tests/test_history tests/test_env_manager test_executor tests/test_env_integration tests/test_pipeline tests/test_parser_spec tests/test_integration tests/test_sec03_env_exposure
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
	@echo "=== Running Cross-Module Integration Tests ==="
	./tests/test_integration
	@echo "=== Running SEC-03 Environment Exposure Tests ==="
	./tests/test_sec03_env_exposure

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
	      tests/test_pipeline \
	      tests/test_integration \
	      tests/.tmp_integration_input \
	      tests/.tmp_integration_output \
	      tests/.tmp_integration_env_output.txt \
	      tests/.tmp_integration_bg_output.txt \
	      tests/test_sec03_env_exposure \
	      tests/.tmp_sec03_input \
	      tests/.tmp_sec03_output \
	      *.gcda *.gcno *.gcov src/*.gcda src/*.gcno src/*.gcov
