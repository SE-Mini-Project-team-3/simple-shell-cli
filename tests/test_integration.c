#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/*
 * Cross-module integration tests.
 *
 * Unlike test_history.c / test_env_manager.c (which call each module's
 * API directly) and test_env_integration.c (which exercises export/unset/
 * env in isolation), these tests drive the compiled shell binary through
 * command sequences that span History, Environment, Pipeline/Redirection
 * and Process Execution together, the way a real session would. This is
 * what actually proves the modules are integrated, not just individually
 * correct (SAD §4.1 "one command-processing cycle"; US-09).
 */

#define TEST_VAR "SIMPLE_SHELL_INTEGRATION_VAR"
#define TEST_VALUE "integration_test_value_456"

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
    const char *input_path = "tests/.tmp_integration_input";
    const char *output_path = "tests/.tmp_integration_output";

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

static int count_occurrences(const char *haystack, const char *needle)
{
    int count = 0;
    const char *p = haystack;

    while ((p = strstr(p, needle)) != NULL)
    {
        count++;
        p += strlen(needle);
    }

    return count;
}

/* FR-HE-02: history prints stored commands with numeric indices. */
static void test_history_print_format(void)
{
    char *output = run_shell_script(
        "pwd\n"
        "echo hi\n"
        "history\n"
        "exit\n");

    CHECK(output != NULL, "shell script for history format ran");
    CHECK(output != NULL && strstr(output, "    1  pwd") != NULL,
          "history numbers the first command '1' with the stored text");
    CHECK(output != NULL && strstr(output, "    2  echo hi") != NULL,
          "history numbers the second command '2' with the stored text");

    free(output);
}

/*
 * F3 (pipeline) + F4 (env): an exported variable must be visible to an
 * external command used as a pipeline stage, not just a standalone
 * external command (test_env_integration.c only covers the latter).
 */
static void test_export_visible_inside_pipeline(void)
{
    char *output = run_shell_script(
        "export " TEST_VAR "=" TEST_VALUE "\n"
        "printenv " TEST_VAR " | cat\n"
        "exit\n");

    CHECK(output != NULL, "shell script for export-through-pipeline ran");
    CHECK(output != NULL && strstr(output, TEST_VALUE) != NULL,
          "a pipeline stage launched after export sees the variable");

    free(output);
}

/* F3 (redirection) + F4 (env): `env > file` must capture real output. */
static void test_env_output_redirection(void)
{
    const char *out_file = "tests/.tmp_integration_env_output.txt";
    char script[256];

    snprintf(script, sizeof(script),
             "export " TEST_VAR "=" TEST_VALUE "\n"
             "env > %s\n"
             "exit\n",
             out_file);

    char *output = run_shell_script(script);
    CHECK(output != NULL, "shell script for env redirection ran");

    FILE *f = fopen(out_file, "r");
    CHECK(f != NULL, "redirected env output file was created");

    if (f != NULL)
    {
        fseek(f, 0, SEEK_END);
        long size = ftell(f);
        fseek(f, 0, SEEK_SET);

        char *contents = malloc((size_t)size + 1);
        size_t read_bytes = fread(contents, 1, (size_t)size, f);
        contents[read_bytes] = '\0';

        CHECK(strstr(contents, TEST_VAR "=" TEST_VALUE) != NULL,
              "env redirected to a file contains the exported variable");

        /* The redirected command's own output must not also appear on
         * the shell's stdout (redirection must actually redirect). */
        CHECK(strstr(output, TEST_VAR "=" TEST_VALUE) == NULL,
              "env output sent to a file does not also leak to the terminal");

        free(contents);
        fclose(f);
    }

    remove(out_file);
    free(output);
}

/*
 * F2 (background execution) + F4 (env): a background external process
 * must also inherit environment updates, not just foreground ones.
 */
static void test_export_visible_to_background_process(void)
{
    const char *out_file = "tests/.tmp_integration_bg_output.txt";
    char script[256];

    snprintf(script, sizeof(script),
             "export " TEST_VAR "=" TEST_VALUE "\n"
             "printenv " TEST_VAR " > %s &\n"
             "exit\n",
             out_file);

    char *output = run_shell_script(script);
    CHECK(output != NULL, "shell script for export-to-background ran");

    /* Background work may still be finishing when the shell exits;
     * give the reaper/poll loop a brief window. */
    system("sleep 0.3");

    FILE *f = fopen(out_file, "r");
    CHECK(f != NULL, "background process output file was created");

    if (f != NULL)
    {
        char buf[256] = {0};
        size_t read_bytes = fread(buf, 1, sizeof(buf) - 1, f);
        (void)read_bytes;

        CHECK(strstr(buf, TEST_VALUE) != NULL,
              "a background external process launched after export sees the variable");

        fclose(f);
    }

    remove(out_file);
    free(output);
}

/*
 * FR-HE-01/03 exercised end-to-end: 55 distinct commands through the
 * real shell (parser -> dispatcher -> history_add), not direct calls
 * into history_add() as in test_history.c's unit test.
 *
 * Note: the `history` command itself is a successfully parsed command
 * line, so running it consumes a history slot too (recorded before it
 * prints) -- 55 echoes + the `history` invocation = 56 entries added,
 * so with a 50-entry cap the oldest *6* are evicted (cmd1..cmd6), not
 * 5. This is correct FR-HE-01 behaviour, not a test-harness quirk, and
 * is asserted explicitly below rather than avoided.
 */
static void test_history_boundary_through_real_shell(void)
{
    char script[4096] = "";
    char line[32];

    for (int i = 1; i <= 55; i++)
    {
        snprintf(line, sizeof(line), "echo cmd%d\n", i);
        strncat(script, line, sizeof(script) - strlen(script) - 1);
    }

    strncat(script, "history\n", sizeof(script) - strlen(script) - 1);
    strncat(script, "exit\n", sizeof(script) - strlen(script) - 1);

    char *output = run_shell_script(script);
    CHECK(output != NULL, "shell script for history boundary ran");

    /* 49 surviving "echo cmdX" lines + 1 "history" line (its own
     * invocation) = 50, the SHELL_MAX_HISTORY cap. */
    CHECK(output != NULL && count_occurrences(output, "  echo cmd") == 49,
          "49 of the 55 echoed commands survive the history cap");
    CHECK(output != NULL && count_occurrences(output, "  history") == 1,
          "the history command's own invocation occupies the 50th slot");

    CHECK(output != NULL && strstr(output, "    7  echo cmd7") != NULL,
          "history's oldest surviving entry is numbered 7 "
          "(6 evicted: cmd1..cmd6)");
    CHECK(output != NULL && strstr(output, "   55  echo cmd55") != NULL,
          "the last echoed command is numbered 55");
    CHECK(output != NULL && strstr(output, "   56  history") != NULL,
          "the history command's own entry is numbered 56");
    CHECK(output != NULL && strstr(output, "echo cmd6\n") == NULL &&
              strstr(output, "    6  echo cmd6") == NULL,
          "cmd6 and earlier no longer appear in history output");

    free(output);
}

static void test_pipeline_split_edge_cases(void)
{
    char *output = run_shell_script(
        "echo \"left|right\"\n"
        "echo \"unterminated\n"
        "true | true | true | true | true | true\n"
        "echo PIPELINE_TEST_SURVIVED\n"
        "exit\n");

    CHECK(output != NULL, "pipeline edge-case script ran");
    CHECK(output != NULL && strstr(output, "left|right") != NULL,
          "a pipe inside quotes is treated as ordinary text");
    CHECK(output != NULL &&
              strstr(output, "PIPELINE_TEST_SURVIVED") != NULL,
          "shell continues after malformed and excessive pipelines");

    free(output);
}

static void test_builtin_missing_arguments(void)
{
    char *output = run_shell_script(
        "export\n"
        "unset\n"
        "echo after_builtin_errors\n"
        "exit\n");

    CHECK(output != NULL, "built-in error script ran");
    CHECK(output != NULL &&
              strstr(output, "shell: export: usage: export NAME=VALUE") != NULL,
          "export without arguments displays usage");
    CHECK(output != NULL &&
              strstr(output, "shell: unset: usage: unset NAME") != NULL,
          "unset without arguments displays usage");
    CHECK(output != NULL &&
              strstr(output, "after_builtin_errors") != NULL,
          "shell continues after built-in usage errors");

    free(output);
}

static void test_background_cd_preserves_parent_directory(void)
{
    char cwd[1024];

    if (getcwd(cwd, sizeof(cwd)) == NULL)
    {
        CHECK(0, "could not obtain test working directory");
        return;
    }

    char *output = run_shell_script(
        "pwd\n"
        "cd /tmp &\n"
        "pwd\n"
        "exit\n");

    CHECK(output != NULL, "background cd script ran");
    CHECK(output != NULL &&
              count_occurrences(output, cwd) >= 2,
          "background cd does not change the parent shell directory");

    free(output);
}

static void test_cd_home_and_error_paths(void)
{
    const char *home = getenv("HOME");

    if (home == NULL)
    {
        CHECK(0, "HOME is available for cd tests");
        return;
    }

    char *output = run_shell_script(
        "cd\n"
        "pwd\n"
        "cd ~\n"
        "pwd\n"
        "cd ~/\n"
        "pwd\n"
        "cd /this_directory_does_not_exist\n"
        "echo CD_TEST_SURVIVED\n"
        "exit\n");

    CHECK(output != NULL, "cd edge-case script ran");
    CHECK(output != NULL && count_occurrences(output, home) >= 3,
          "cd, cd ~ and cd ~/ all resolve to HOME");
    CHECK(output != NULL &&
              strstr(output, "shell: cd:") != NULL,
          "cd to a nonexistent directory reports an error");
    CHECK(output != NULL &&
              strstr(output, "CD_TEST_SURVIVED") != NULL,
          "shell continues after cd failure");

    free(output);
}

static void test_main_input_edge_cases(void)
{
    char script[1200];

    script[0] = '\n';
    memset(script + 1, 'x', 1100);
    script[1101] = '\n';

    strcpy(script + 1102,
           "echo INPUT_TEST_SURVIVED\nexit\n");

    char *output = run_shell_script(script);

    CHECK(output != NULL, "input edge-case script ran");
    CHECK(output != NULL &&
              strstr(output, "shell: empty command") != NULL,
          "empty input is rejected");
    CHECK(output != NULL &&
              strstr(output, "shell: input line too long") != NULL,
          "oversized input is rejected");
    CHECK(output != NULL &&
              strstr(output, "INPUT_TEST_SURVIVED") != NULL,
          "shell continues after invalid input");

    free(output);
}

int main(void)
{
    test_history_print_format();
    test_export_visible_inside_pipeline();
    test_env_output_redirection();
    test_export_visible_to_background_process();
    test_history_boundary_through_real_shell();
    test_pipeline_split_edge_cases();
    test_builtin_missing_arguments();
    test_background_cd_preserves_parent_directory();
    test_cd_home_and_error_paths();
    test_main_input_edge_cases();

    if (failures == 0)
    {
        printf("\nAll integration tests passed.\n");
        return 0;
    }

    printf("\n%d integration test(s) failed.\n", failures);
    return 1;
}
