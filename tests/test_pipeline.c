
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
            strcmp(entry->d_name, "..") != 0) {
            count++;
        }
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

    for (int i = 0; i < SHELL_MAX_PIPELINE_STAGES; i++) {
        commands[i] = &stages[i];
    }

    char *printf_args[] = {"printf", "hello", NULL};
    char *cat_args[] = {"cat", NULL};
    char *false_args[] = {"false", NULL};

    char *invalid_args[] = {
        "nonexistent_command_xyz_12345",
        NULL
    };

    /* Test 1: Two-stage pipeline */

    init_command(&stages[0], printf_args);
    init_command(&stages[1], cat_args);

    total++;

    passed += run_pipeline_test(
        "two-stage pipeline",
        commands,
        2,
        "hello",
        0
    );

    /* Test 2: Three-stage pipeline */

    init_command(&stages[0], printf_args);
    init_command(&stages[1], cat_args);
    init_command(&stages[2], cat_args);

    total++;

    passed += run_pipeline_test(
        "three-stage pipeline",
        commands,
        3,
        "hello",
        0
    );

    /* Test 3: Maximum five-stage pipeline */

    for (int i = 0; i < SHELL_MAX_PIPELINE_STAGES; i++) {
        init_command(
            &stages[i],
            i == 0 ? printf_args : cat_args
        );
    }

    total++;

    passed += run_pipeline_test(
        "maximum five-stage pipeline",
        commands,
        5,
        "hello",
        0
    );

    /* Test 4: Final-stage exit status */

    init_command(&stages[0], false_args);

    total++;

    int status = pipeline_execute(commands, 1);

    if (status == 1) {
        printf("PASS: final-stage exit status\n");
        passed++;
    } else {
        printf("FAIL: final-stage exit status (got %d)\n", status);
    }

    /* Test 5: Invalid command error handling */

    init_command(&stages[0], invalid_args);
    init_command(&stages[1], cat_args);

    FILE *error_file = tmpfile();
    int saved_stderr = dup(STDERR_FILENO);

    total++;

    if (error_file == NULL || saved_stderr < 0) {
        printf("FAIL: invalid command error capture setup\n");

        if (error_file != NULL)
            fclose(error_file);

        if (saved_stderr >= 0)
            close(saved_stderr);
    } else {
        fflush(stderr);

        dup2(fileno(error_file), STDERR_FILENO);

        status = pipeline_execute(commands, 2);

        fflush(stderr);

        dup2(saved_stderr, STDERR_FILENO);

        close(saved_stderr);

        char error_buffer[512] = {0};

        rewind(error_file);

        fread(
            error_buffer,
            1,
            sizeof(error_buffer) - 1,
            error_file
        );

        fclose(error_file);

        if (status == 0 &&
            strstr(error_buffer,
                   "nonexistent_command_xyz_12345") != NULL &&
            strstr(error_buffer,
                   "No such file or directory") != NULL) {

            printf(
                "PASS: invalid command reports expected error and pipeline completes\n"
            );

            passed++;
        } else {
            printf(
                "FAIL: invalid command error handling "
                "(status=%d, stderr=%s)\n",
                status,
                error_buffer
            );
        }
    }

    /* Test 6: Reject more than five stages before array access */

    /*
     * Deliberately provide an array containing only one pointer.
     * pipeline_execute() must reject the stage count before
     * attempting to access any additional array elements.
     */

    ParsedCommand *undersized_array[1] = {
        &stages[0]
    };

    total++;

    status = pipeline_execute(
        undersized_array,
        SHELL_MAX_PIPELINE_STAGES + 1
    );

    if (status == 2) {
        printf(
            "PASS: excessive pipeline stages rejected before array access\n"
        );

        passed++;
    } else {
        printf(
            "FAIL: excessive pipeline stages were not rejected (status=%d)\n",
            status
        );
    }

    /* Test 7: Parent file-descriptor leak check */

    init_command(&stages[0], printf_args);
    init_command(&stages[1], cat_args);

    stages[1].output_file = "/dev/null";

    int before = count_open_fds();

    for (int i = 0; i < 10; i++) {
        pipeline_execute(commands, 2);
    }

    int after = count_open_fds();

    total++;

    if (before >= 0 && before == after) {
        printf(
            "PASS: no parent file descriptor leaks across repeated pipelines\n"
        );

        passed++;
    } else {
        printf(
            "FAIL: file descriptor count changed "
            "(before=%d, after=%d)\n",
            before,
            after
        );
    }

    unlink(TEST_FILE);

    printf("\nPipeline tests: %d/%d passed\n", passed, total);

    return passed == total ? EXIT_SUCCESS : EXIT_FAILURE;
}
