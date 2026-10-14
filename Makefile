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

.PHONY: all clean test test-parser-spec

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ)

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET) tests/test_history tests/test_env_manager tests/test_parser_spec

tests/test_history: tests/test_history.c src/history.c
	$(CC) $(CFLAGS) -o $@ tests/test_history.c src/history.c

tests/test_env_manager: tests/test_env_manager.c src/env_manager.c
	$(CC) $(CFLAGS) -o $@ tests/test_env_manager.c src/env_manager.c

test: tests/test_history tests/test_env_manager
	./tests/test_history
	./tests/test_env_manager

# US-01 (parser) acceptance spec. Not wired into `test`: parser.c is still a
# placeholder, so these cases are expected to fail until it's implemented.
tests/test_parser_spec: tests/test_parser_spec.c src/parser.c
	$(CC) $(CFLAGS) -o $@ tests/test_parser_spec.c src/parser.c

test-parser-spec: tests/test_parser_spec
	-./tests/test_parser_spec
