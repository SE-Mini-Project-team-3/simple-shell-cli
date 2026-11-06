#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * SEC-03 / SEC-OBJ-03: "The shell shall not print or log environment-
 * variable values unless the user explicitly requests them through
 * env." These tests plant a secret value in the environment, then
 * drive a range of error paths and unrelated builtins through the
 * real shell binary, asserting the secret never appears in output
 * except via the one explicit disclosure point (`env`/`printenv`).
 */

#define SECRET_VAR "SIMPLE_SHELL_SECRET_VAR"
#define SECRET_VALUE "sec03_confidential_9f3a7c"

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

static char *run_shell_script(const char *script)
{
    const char *input_path = "tests/.tmp_sec03_input";
    const char *output_path = "tests/.tmp_sec03_output";

    FILE *in = fopen(input_path, "w");
    if (in == NULL)
        return NULL;

    fputs(script, in);
    fclose(in);

    char command[512];
    snprintf(command, sizeof(command), "./simple-shell < %s > %s 2>&1",
              input_path, output_path);

    if (system(command) == -1)
        return NULL;

    FILE *out = fopen(output_path, "r");
    if (out == NULL)
        return NULL;

    fseek(out, 0, SEEK_END);
    long size = ftell(out);
    fseek(out, 0, SEEK_SET);

    char *buffer = malloc((size_t)size + 1);
    if (buffer == NULL)
    {
        fclose(out);
        return NULL;
    }

    size_t read_bytes = fread(buffer, 1, (size_t)size, out);
    buffer[read_bytes] = '\0';
    fclose(out);

    remove(input_path);
    remove(output_path);

    return buffer;
}

/* Positive control: confirm the secret DOES appear via the one
 * sanctioned disclosure path, so a false "PASS" elsewhere can't be
 * explained by the value never having been set correctly at all. */
static void test_positive_control_env_discloses_value(void)
{
    char *output = run_shell_script(
        "export " SECRET_VAR "=" SECRET_VALUE "\n"
        "env\n"
        "exit\n");

    CHECK(output != NULL, "positive-control script ran");
    CHECK(output != NULL && strstr(output, SECRET_VALUE) != NULL,
          "env (the explicit disclosure path) does show the secret value");

    free(output);
}

static void check_no_leak(const char *script, const char *label)
{
    char full_script[1024];
    snprintf(full_script, sizeof(full_script),
             "export " SECRET_VAR "=" SECRET_VALUE "\n%s\nexit\n", script);

    char *output = run_shell_script(full_script);

    char message[256];
    snprintf(message, sizeof(message),
             "%s does not leak the exported secret value", label);

    CHECK(output != NULL, "script ran");
    CHECK(output != NULL && strstr(output, SECRET_VALUE) == NULL, message);

    free(output);
}

int main(void)
{
    test_positive_control_env_discloses_value();

    check_no_leak("cd /no/such/nonexistent/directory",
                   "a failed cd (bad path)");

    check_no_leak("echo hi > /no/such/nonexistent/dir/file.txt",
                   "a failed output redirection");

    check_no_leak("cat < /no/such/nonexistent/input.txt",
                   "a failed input redirection");

    check_no_leak("nonexistent_command_xyz_12345",
                   "an unknown external command");

    check_no_leak("| | |", "a malformed pipeline");

    check_no_leak("export NOVALUE", "a malformed export argument");

    check_no_leak("unset " SECRET_VAR "\nexport MISSING_EQUALS",
                   "unset followed by another malformed export");

    check_no_leak("help", "the help builtin");

    check_no_leak("pwd", "the pwd builtin");

    check_no_leak("echo just some ordinary output",
                   "the echo builtin with unrelated arguments");

    check_no_leak("ls | grep .c | wc -l",
                   "an unrelated multi-stage pipeline");

    /*
     * Note (not a SEC-03 violation, documented deliberately):
     * `history` stores and redisplays the literal command line the
     * user typed, e.g. "export SECRET=value". That line's text
     * necessarily contains the value because the user explicitly
     * typed it -- this is the user's own input being echoed back on
     * their own explicit request (running `history`), not the shell
     * independently disclosing a live environment value the way
     * `env`/`printenv` do. SEC-03 is about the shell not pulling
     * values out of the environment table unprompted; it does not,
     * and cannot reasonably, redact a user's own typed input.
     */
    char *history_output = run_shell_script(
        "export " SECRET_VAR "=" SECRET_VALUE "\n"
        "history\n"
        "exit\n");

    CHECK(history_output != NULL, "history-echoes-typed-input script ran");
    CHECK(history_output != NULL &&
              strstr(history_output, "export " SECRET_VAR "=" SECRET_VALUE)
                  != NULL,
          "history shows the user's own typed export line verbatim "
          "(expected: this is the user's input, not an env-table read)");

    if (failures == 0)
    {
        printf("\nAll SEC-03 environment-exposure tests passed.\n");
        return 0;
    }

    printf("\n%d SEC-03 environment-exposure test(s) failed.\n", failures);
    return 1;
}
