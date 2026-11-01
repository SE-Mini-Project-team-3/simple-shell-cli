
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "parser.h"
#include "shell.h"

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

    free_parsed_command(&cmd);
}

static void test_whitespace(void)
{
    char input[] = "   pwd   ";
    ParsedCommand cmd;

    assert(parse_command(input, &cmd) == 0);
    assert(cmd.is_builtin);
    assert(strcmp(cmd.argv[0], "pwd") == 0);
    assert(cmd.argv[1] == NULL);

    free_parsed_command(&cmd);
}

static void test_external_command(void)
{
    char input[] = "ls -l";
    ParsedCommand cmd;

    assert(parse_command(input, &cmd) == 0);
    assert(!cmd.is_builtin);
    assert(strcmp(cmd.argv[0], "ls") == 0);
    assert(strcmp(cmd.argv[1], "-l") == 0);

    free_parsed_command(&cmd);
}

static void test_all_builtins(void)
{
    const char *builtins[] = {
        "cd", "pwd", "echo", "exit", "help", "history"
    };

    for (size_t i = 0; i < sizeof(builtins) / sizeof(builtins[0]); i++) {
        char input[32];
        ParsedCommand cmd;

        snprintf(input, sizeof(input), "%s", builtins[i]);

        assert(parse_command(input, &cmd) == 0);
        assert(cmd.is_builtin);
        assert(strcmp(cmd.argv[0], builtins[i]) == 0);

        free_parsed_command(&cmd);
    }
}

static void test_empty_input(void)
{
    char input[] = "   ";
    ParsedCommand cmd;

    assert(parse_command(input, &cmd) == 0);
    assert(cmd.argv[0] == NULL);
    assert(!cmd.is_builtin);

    free_parsed_command(&cmd);
}

static void test_invalid_arguments(void)
{
    char input[] = "echo";
    ParsedCommand cmd;

    assert(parse_command(NULL, &cmd) == -1);
    assert(parse_command(input, NULL) == -1);
}

static void test_input_length_boundary(void)
{
    char input[SHELL_MAX_INPUT + 2];
    ParsedCommand cmd;

    /* Exactly SHELL_MAX_INPUT characters should be accepted. */
    memset(input, 'x', SHELL_MAX_INPUT);
    input[SHELL_MAX_INPUT] = '\0';

    assert(strlen(input) == SHELL_MAX_INPUT);
    assert(parse_command(input, &cmd) == 0);
    free_parsed_command(&cmd);

    /* One character below the limit should be accepted. */
    memset(input, 'x', SHELL_MAX_INPUT - 1);
    input[SHELL_MAX_INPUT - 1] = '\0';

    assert(strlen(input) == SHELL_MAX_INPUT - 1);
    assert(parse_command(input, &cmd) == 0);
    free_parsed_command(&cmd);

    /* One character above the limit must be rejected. */
    memset(input, 'x', SHELL_MAX_INPUT + 1);
    input[SHELL_MAX_INPUT + 1] = '\0';

    assert(strlen(input) == SHELL_MAX_INPUT + 1);
    assert(parse_command(input, &cmd) == -1);
}

static void test_too_many_arguments(void)
{
    char input[SHELL_MAX_INPUT + 1] = "echo";
    ParsedCommand cmd;

    for (int i = 0; i < SHELL_MAX_ARGS + 1; i++) {
        strcat(input, " x");
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
    test_input_length_boundary();
    test_too_many_arguments();
    test_free_parsed_command();

    printf("All parser tests passed.\n");
    return 0;
}
