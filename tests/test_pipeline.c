

#include "pipeline.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>

#define TEST_FILE "/tmp/simple_shell_pipeline_test.txt"

static int count_open_fds(void)
{
    DIR *dir = opendir("/proc/self/fd");
    if (dir == NULL)
        return -1;

    int count = 0;
    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") != 0 &&
            strcmp(entry->d_name, "..") != 0)
            count++;
    }

    closedir(dir);
    return count;
}

static void init_command(ParsedCommand *cmd, char *args[])
{
    memset(cmd, 0, sizeof(*cmd));

    int i = 0;
    while (args[i] != NULL && i < SHELL_MAX_ARGS) {
        cmd->argv[i] = args[i];
        i++;
    }

    cmd->argv[i] = NULL;
}

static int check_output(const char *expected)
{
    char buffer[128] = {0};

    int fd = open(TEST_FILE, O_RDONLY);
    if (fd < 0)
        return 0;

    ssize_t n = read(fd, buffer, sizeof(buffer) - 1);
    close(fd);

    return n >= 0 && strcmp(buffer, expected) == 0;
}

static int run_pipeline_test(
    const char *name,
    ParsedCommand *commands[],
    int count,
    const char *expected,
    int expected_status)
{
    unlink(TEST_FILE);

    commands[count - 1]->output_file = TEST_FILE;

    int status = pipeline_execute(commands, count);
    int output_ok = check_output(expected);

    if (status == expected_status && output_ok) {
        printf("PASS: %s\n", name);
        return 1;
    }

    printf("FAIL: %s (status=%d, output_ok=%d)\n",
           name, status, output_ok);
    return 0;
}

int main(void)
{
    int passed = 0;
    int total = 0;

    ParsedCommand stages[SHELL_MAX_PIPELINE_STAGES];
    ParsedCommand *commands[SHELL_MAX_PIPELINE_STAGES];

    for (int i = 0; i < SHELL_MAX_PIPELINE_STAGES; i++)
        commands[i] = &stages[i];

    char *printf_args[] = {"printf", "hello", NULL};
    char *cat_args[] = {"cat", NULL};
    char *false_args[] = {"false", NULL};
    char *invalid_args[] = {
        "nonexistent_command_xyz_12345", NULL
    };

    /* Two-stage pipeline */
    init_command(&stages[0], printf_args);
    init_command(&stages[1], cat_args);

    total++;
    passed += run_pipeline_test(
        "two-stage pipeline",
        commands, 2, "hello", 0);

    /* Three-stage pipeline */
    init_command(&stages[0], printf_args);
    init_command(&stages[1], cat_args);
    init_command(&stages[2], cat_args);

    total++;
    passed += run_pipeline_test(
        "three-stage pipeline",
        commands, 3, "hello", 0);

    /* Maximum five-stage pipeline */
    for (int i = 0; i < 5; i++)
        init_command(&stages[i], i == 0 ? printf_args : cat_args);

    total++;
    passed += run_pipeline_test(
        "maximum five-stage pipeline",
        commands, 5, "hello", 0);

    /* Final-stage exit status */
    init_command(&stages[0], false_args);

    total++;
    int status = pipeline_execute(commands, 1);

    if (status == 1) {
        printf("PASS: final-stage exit status\n");
        passed++;
    } else {
        printf("FAIL: final-stage exit status (got %d)\n", status);
    }

    /* Invalid command followed by a valid command */
    init_command(&stages[0], invalid_args);
    init_command(&stages[1], cat_args);

    total++;
    status = pipeline_execute(commands, 2);

    if (status == 0) {
        printf("PASS: invalid command does not prevent pipeline completion\n");
        passed++;
    } else {
        printf("FAIL: invalid command pipeline (status=%d)\n", status);
    }

    /* Reject more than five stages */
    total++;
    status = pipeline_execute(
        commands, SHELL_MAX_PIPELINE_STAGES + 1);

    if (status == 2) {
        printf("PASS: pipeline rejects more than five stages\n");
        passed++;
    } else {
        printf("FAIL: pipeline accepted more than five stages (status=%d)\n",
               status);
    }

    /* Parent file descriptor leak check */
    init_command(&stages[0], printf_args);
    init_command(&stages[1], cat_args);
    stages[1].output_file = "/dev/null";

    int before = count_open_fds();

    for (int i = 0; i < 10; i++)
        pipeline_execute(commands, 2);

    int after = count_open_fds();

    total++;
    if (before >= 0 && before == after) {
        printf("PASS: no parent file descriptor leaks across repeated pipelines\n");
        passed++;
    } else {
        printf("FAIL: file descriptor count changed (before=%d, after=%d)\n",
               before, after);
    }

    unlink(TEST_FILE);

    printf("\nPipeline tests: %d/%d passed\n", passed, total);

    return passed == total ? EXIT_SUCCESS : EXIT_FAILURE;
}
