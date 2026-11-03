#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TEST_VAR "SIMPLE_SHELL_TEST_VAR"
#define TEST_VALUE "integration_value_123"

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

/*
 * Drives the actual compiled shell binary (not the env_manager API
 * directly) so these tests exercise the real export/unset/env builtin
 * dispatch path, including the parser's command classification.
 */
static char *run_shell_script(const char *script)
{
    const char *input_path = "tests/.tmp_env_integration_input";
    const char *output_path = "tests/.tmp_env_integration_output";

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

static void test_export_visible_via_env(void)
{
    char *output = run_shell_script(
        "export " TEST_VAR "=" TEST_VALUE "\n"
        "env\n"
        "exit\n");

    CHECK(output != NULL, "shell script for export/env ran");
    CHECK(output != NULL && strstr(output, TEST_VAR "=" TEST_VALUE) != NULL,
          "export followed by env shows the new variable");

    free(output);
}

static void test_unset_removes_variable(void)
{
    char *output = run_shell_script(
        "export " TEST_VAR "=" TEST_VALUE "\n"
        "unset " TEST_VAR "\n"
        "env\n"
        "exit\n");

    CHECK(output != NULL, "shell script for unset ran");
    CHECK(output != NULL && strstr(output, TEST_VAR "=" TEST_VALUE) == NULL,
          "unset removes the variable from subsequent env output");

    free(output);
}

static void test_export_visible_to_external_child(void)
{
    /* FR-HE-07: environment updates must reach subsequently created
     * children, exercised here through a real external command
     * (not the "env" builtin) launched via the process executor. */
    char *output = run_shell_script(
        "export " TEST_VAR "=" TEST_VALUE "\n"
        "printenv " TEST_VAR "\n"
        "exit\n");

    CHECK(output != NULL, "shell script for export/printenv ran");
    CHECK(output != NULL && strstr(output, TEST_VALUE) != NULL,
          "an external child process launched after export sees the variable");

    free(output);
}

static void test_export_requires_name_value_pair(void)
{
    char *output = run_shell_script("export NOVALUE\nenv\nexit\n");

    CHECK(output != NULL, "shell script for malformed export ran");

    /* Assert the exact diagnostic, not just a loose substring match
     * (a bare "export" could coincidentally appear in the prompt or
     * help text even if the real error path never ran). */
    CHECK(output != NULL &&
              strstr(output,
                     "shell: export: invalid argument 'NOVALUE'") != NULL,
          "export without NAME=VALUE reports the exact expected error");

    /* And confirm the malformed argument was never actually set. */
    CHECK(output != NULL && strstr(output, "NOVALUE=") == NULL,
          "the malformed export argument is not set as an environment variable");

    free(output);
}

int main(void)
{
    test_export_visible_via_env();
    test_unset_removes_variable();
    test_export_visible_to_external_child();
    test_export_requires_name_value_pair();

    if (failures == 0)
    {
        printf("\nAll env integration tests passed.\n");
        return 0;
    }

    printf("\n%d env integration test(s) failed.\n", failures);
    return 1;
}
