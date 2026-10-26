
CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -Iinclude
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

$(TEST_TARGET): tests/test_parser.c src/parser.c
	$(CC) $(CFLAGS) -o $@ tests/test_parser.c src/parser.c

tests/test_history: tests/test_history.c src/history.c
	$(CC) $(CFLAGS) -o $@ tests/test_history.c src/history.c

tests/test_env_manager: tests/test_env_manager.c src/env_manager.c
	$(CC) $(CFLAGS) -o $@ tests/test_env_manager.c src/env_manager.c

test: all $(TEST_TARGET) tests/test_history tests/test_env_manager
	./$(TEST_TARGET)
	./tests/test_history
	./tests/test_env_manager

clean:
	rm -f $(OBJ) $(TARGET) $(TEST_TARGET) tests/test_history tests/test_env_manager tests/test_parser_spec

# US-01 (parser) acceptance spec, now largely redundant with tests/test_parser.c
# now that parsing is implemented; kept to flag any contract gaps.
tests/test_parser_spec: tests/test_parser_spec.c src/parser.c
	$(CC) $(CFLAGS) -o $@ tests/test_parser_spec.c src/parser.c

test-parser-spec: tests/test_parser_spec
	-./tests/test_parser_spec
