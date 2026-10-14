/*
 * Acceptance-test spec for US-01 (command prompt & input parsing, issue #3).
 *
 * parser.c is still a placeholder (owned by the parsing module's implementer),
 * so these cases are expected to FAIL until that work lands. They exist to
 * pin down the parse_command() contract ahead of time, per the team's
 * coordination rule: agree on shared signatures/behavior before implementing.
 *
 * Assumed contract for `int parse_command(char *input, ParsedCommand *cmd)`:
 *   - Returns 0 on success, -1 on any error.
 *   - On success: cmd->argv is NULL-terminated, argv[0] is the command name,
 *     cmd->is_builtin is true iff argv[0] is one of cd/pwd/echo/exit/help
 *     (FR-CI-03, FR-CI-04).
 *   - Errors (FR-CI-05, FR-CI-06, SEC-02):
 *       * NULL input, empty input, or input that is only whitespace
 *       * input of length >= SHELL_MAX_INPUT (oversized input rejected)
 *       * more than SHELL_MAX_ARGS tokens
 *       * malformed redirection (">"/"<" with no following filename)
 *       * unterminated quotes
 *   - free_parsed_command() releases anything parse_command() allocated and
 *     is safe to call on a ParsedCommand that failed to parse.
 *
 * Run explicitly with `make test-parser-spec`; not part of the default
 * `make test` target while parsing is unimplemented.
 */

#include "parser.h"
#include "shell.h"

#include <stdio.h>
#include <string.h>

static int failures = 0;

#define CHECK(cond, message)                \
    do                                        \
    {                                          \
        if (!(cond))                           \
        {                                       \
            printf("FAIL: %s\n", message);      \
            failures++;                          \
        }                                           \
        else                                         \
        {                                             \
            printf("PASS: %s\n", message);              \
        }                                                  \
    } while (0)

static int parse(char *input, ParsedCommand *cmd)
{
    memset(cmd, 0, sizeof(*cmd));
    return parse_command(input, cmd);
}

static void test_simple_builtin_command(void)
{
    char input[] = "pwd";
    ParsedCommand cmd;

    CHECK(parse(input, &cmd) == 0, "a simple builtin command parses successfully");
    CHECK(cmd.argv[0] != NULL && strcmp(cmd.argv[0], "pwd") == 0, "argv[0] is the command name");
    CHECK(cmd.argv[1] == NULL, "argv is NULL-terminated after a single token");
    CHECK(cmd.is_builtin, "pwd is classified as a builtin (FR-CI-03/FR-CI-04)");

    free_parsed_command(&cmd);
}

static void test_command_with_arguments(void)
{
    char input[] = "echo hello world";
    ParsedCommand cmd;

    CHECK(parse(input, &cmd) == 0, "a command with arguments parses successfully");
    CHECK(cmd.argv[0] != NULL && strcmp(cmd.argv[0], "echo") == 0, "argv[0] is the command name");
    CHECK(cmd.argv[1] != NULL && strcmp(cmd.argv[1], "hello") == 0, "argv[1] is the first argument");
    CHECK(cmd.argv[2] != NULL && strcmp(cmd.argv[2], "world") == 0, "argv[2] is the second argument");
    CHECK(cmd.argv[3] == NULL, "argv is NULL-terminated after all tokens");
    CHECK(cmd.is_builtin, "echo is classified as a builtin");

    free_parsed_command(&cmd);
}

static void test_external_command_is_not_builtin(void)
{
    char input[] = "ls -la";
    ParsedCommand cmd;

    CHECK(parse(input, &cmd) == 0, "an external command parses successfully");
    CHECK(!cmd.is_builtin, "ls is classified as external, not a builtin (FR-CI-03)");

    free_parsed_command(&cmd);
}

static void test_rejects_null_input(void)
{
    ParsedCommand cmd;
    memset(&cmd, 0, sizeof(cmd));

    CHECK(parse_command(NULL, &cmd) == -1, "NULL input is rejected (FR-CI-05)");
}

static void test_rejects_empty_and_whitespace_input(void)
{
    char empty[] = "";
    char whitespace[] = "   \t  ";
    ParsedCommand cmd;

    CHECK(parse(empty, &cmd) == -1, "empty input is rejected (FR-CI-05)");
    CHECK(parse(whitespace, &cmd) == -1, "whitespace-only input is rejected (FR-CI-05)");
}

static void test_rejects_oversized_input(void)
{
    static char oversized[SHELL_MAX_INPUT + 16];
    memset(oversized, 'a', sizeof(oversized) - 1);
    oversized[sizeof(oversized) - 1] = '\0';

    ParsedCommand cmd;
    CHECK(parse(oversized, &cmd) == -1, "input at/over SHELL_MAX_INPUT is rejected (FR-CI-01/FR-CI-06/SEC-02)");
}

static void test_rejects_too_many_arguments(void)
{
    char input[SHELL_MAX_INPUT] = "echo";
    for (int i = 0; i < SHELL_MAX_ARGS + 4; i++)
    {
        strcat(input, " x");
    }

    ParsedCommand cmd;
    CHECK(parse(input, &cmd) == -1, "more than SHELL_MAX_ARGS tokens is rejected (FR-CI-02/SEC-02)");
}

static void test_rejects_malformed_redirection(void)
{
    char input[] = "echo hello >";
    ParsedCommand cmd;

    CHECK(parse(input, &cmd) == -1, "a redirection operator with no filename is rejected (FR-CI-05)");
}

static void test_rejects_unterminated_quote(void)
{
    char input[] = "echo \"unterminated";
    ParsedCommand cmd;

    CHECK(parse(input, &cmd) == -1, "an unterminated quote is rejected (FR-CI-05)");
}

int main(void)
{
    test_simple_builtin_command();
    test_command_with_arguments();
    test_external_command_is_not_builtin();
    test_rejects_null_input();
    test_rejects_empty_and_whitespace_input();
    test_rejects_oversized_input();
    test_rejects_too_many_arguments();
    test_rejects_malformed_redirection();
    test_rejects_unterminated_quote();

    if (failures == 0)
    {
        printf("\nAll parser spec tests passed.\n");
        return 0;
    }

    printf("\n%d parser spec test(s) failed (expected until US-01 is implemented).\n", failures);
    return 1;
}
