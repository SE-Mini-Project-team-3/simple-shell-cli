#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "parser.h"

static void test_normal_command(void)
{
    char input[] = "echo hello world";
    ParsedCommand cmd;

    assert(parse_command(input, &cmd) == 0);
    assert(cmd.is_builtin);
    assert(strcmp(cmd.argv[0], "echo") == 0);
    assert(strcmp(cmd.argv[1], "hello") == 0);
    assert(strcmp(cmd.argv[2], "world") == 0);
    assert(cmd.argv[3] == NULL);
}

static void test_whitespace(void)
{
    char input[] = "   pwd   ";
    ParsedCommand cmd;

    assert(parse_command(input, &cmd) == 0);
    assert(cmd.is_builtin);
    assert(strcmp(cmd.argv[0], "pwd") == 0);
    assert(cmd.argv[1] == NULL);
}

static void test_external_command(void)
{
    char input[] = "ls -l";
    ParsedCommand cmd;

    assert(parse_command(input, &cmd) == 0);
    assert(!cmd.is_builtin);
    assert(strcmp(cmd.argv[0], "ls") == 0);
    assert(strcmp(cmd.argv[1], "-l") == 0);
}

static void test_all_builtins(void)
{
    const char *builtins[] = {"cd", "pwd", "echo", "exit", "help"};

    for (size_t i = 0; i < sizeof(builtins) / sizeof(builtins[0]); i++) {
        char input[32];
        ParsedCommand cmd;

        snprintf(input, sizeof(input), "%s", builtins[i]);
        assert(parse_command(input, &cmd) == 0);
        assert(cmd.is_builtin);
    }
}

static void test_empty_input(void)
{
    char input[] = "   ";
    ParsedCommand cmd;

    assert(parse_command(input, &cmd) == 0);
    assert(cmd.argv[0] == NULL);
    assert(!cmd.is_builtin);
}

static void test_invalid_arguments(void)
{
    char input[] = "echo";
    ParsedCommand cmd;

    assert(parse_command(NULL, &cmd) == -1);
    assert(parse_command(input, NULL) == -1);
}

static void test_too_many_arguments(void)
{
    char input[1024] = "";
    ParsedCommand cmd;

    for (int i = 0; i < SHELL_MAX_ARGS + 1; i++) {
        strcat(input, "x ");
    }

    assert(parse_command(input, &cmd) == -1);
    assert(cmd.argv[0] == NULL);
}

static void test_free_parsed_command(void)
{
    char input[] = "echo hello";
    ParsedCommand cmd;

    assert(parse_command(input, &cmd) == 0);
    free_parsed_command(&cmd);
    assert(cmd.argv[0] == NULL);
    assert(!cmd.is_builtin);
}

int main(void)
{
    test_normal_command();
    test_whitespace();
    test_external_command();
    test_all_builtins();
    test_empty_input();
    test_invalid_arguments();
    test_too_many_arguments();
    test_free_parsed_command();

    printf("All parser tests passed.\n");
    return 0;
}
