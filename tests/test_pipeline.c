#define _DEFAULT_SOURCE

#include "pipeline.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <errno.h>
#include <signal.h>

#define TEST_FILE "/tmp/simple_shell_pipeline_test.txt"
#define INPUT_TEST_FILE "/tmp/simple_shell_pipeline_input.txt"
#define BG_STAGE1_FILE "/tmp/simple_shell_pipeline_stage1.pid"
#define BG_STAGE2_FILE "/tmp/simple_shell_pipeline_stage2.pid"

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

    while (i < SHELL_MAX_ARGS && args[i] != NULL) {
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

static pid_t read_pid_file(const char *path)
{
    FILE *file = fopen(path, "r");

    if (file == NULL)
        return -1;

    long value = -1;
    int result = fscanf(file, "%ld", &value);
    fclose(file);

    if (result != 1 || value <= 0)
        return -1;

    return (pid_t)value;
}

static int wait_for_pid_files(pid_t *pid1, pid_t *pid2)
{
    for (int i = 0; i < 100; i++) {
        *pid1 = read_pid_file(BG_STAGE1_FILE);
        *pid2 = read_pid_file(BG_STAGE2_FILE);

        if (*pid1 > 0 && *pid2 > 0)
            return 1;

        usleep(10000);
    }

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

    /* Test 1: Two-stage pipeline */

    init_command(&stages[0], printf_args);
    init_command(&stages[1], cat_args);

    total++;

    passed += run_pipeline_test(
        "two-stage pipeline",
        commands, 2, "hello", 0
    );

    /* Test 2: Three-stage pipeline */

    init_command(&stages[0], printf_args);
    init_command(&stages[1], cat_args);
    init_command(&stages[2], cat_args);

    total++;

    passed += run_pipeline_test(
        "three-stage pipeline",
        commands, 3, "hello", 0
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
        commands, 5, "hello", 0
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

    /*
     * Test 5: Invalid command error handling.
     * Verify the diagnostic and independently verify that
     * the following valid pipeline stage executes successfully.
     */

    char *invalid_args[] = {
        "nonexistent_command_xyz_12345", NULL
    };

    char *valid_stage_args[] = {
        "sh", "-c", "printf valid-stage-completed", NULL
    };

    init_command(&stages[0], invalid_args);
    init_command(&stages[1], valid_stage_args);

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
        unlink(TEST_FILE);

        stages[1].output_file = TEST_FILE;

        fflush(stderr);

        if (dup2(fileno(error_file), STDERR_FILENO) < 0) {
            printf("FAIL: could not redirect stderr\n");
            close(saved_stderr);
            fclose(error_file);
        } else {
            status = pipeline_execute(commands, 2);

            fflush(stderr);

            /* Restore the original stderr. */
            dup2(saved_stderr, STDERR_FILENO);
            close(saved_stderr);

            char error_buffer[512] = {0};

            rewind(error_file);

            size_t bytes_read = fread(
                error_buffer,
                1,
                sizeof(error_buffer) - 1,
                error_file
            );

            error_buffer[bytes_read] = '\0';

            fclose(error_file);

            int diagnostic_ok =
                strstr(error_buffer,
                       "nonexistent_command_xyz_12345") != NULL &&
                strstr(error_buffer,
                       "No such file or directory") != NULL;

            int valid_stage_ok =
                check_output("valid-stage-completed");

            if (diagnostic_ok && valid_stage_ok && status == 0) {
                printf(
                    "PASS: invalid command reports expected diagnostic\n"
                );

                printf(
                    "PASS: following valid pipeline stage executes successfully\n"
                );

                passed++;
            } else {
                printf("FAIL: invalid command pipeline validation\n");

                printf("  Diagnostic check: %s\n",
                       diagnostic_ok ? "PASS" : "FAIL");

                printf("  Valid stage check: %s\n",
                       valid_stage_ok ? "PASS" : "FAIL");

                printf("  Pipeline status: %d\n", status);

                printf("  Captured stderr: %s\n", error_buffer);
            }
        }
    }


    /*
     * Test 6: Input redirection for a single pipeline stage.
     */

    FILE *input_file = fopen(INPUT_TEST_FILE, "w");
    int input_setup_ok = 0;

    if (input_file != NULL) {
        input_setup_ok =
            fputs("pipeline-input-check", input_file) >= 0;
        fclose(input_file);
    }

    char *input_cat_args[] = {"cat", NULL};
    init_command(&stages[0], input_cat_args);
    stages[0].input_file = INPUT_TEST_FILE;
    stages[0].output_file = TEST_FILE;

    unlink(TEST_FILE);
    total++;

    status = input_setup_ok ? pipeline_execute(commands, 1) : -1;
    int input_output_ok =
        input_setup_ok && check_output("pipeline-input-check");

    if (status == 0 && input_output_ok) {
        printf("PASS: pipeline input redirection\n");
        passed++;
    } else {
        printf(
            "FAIL: pipeline input redirection (status=%d, output_ok=%d)\n",
            status, input_output_ok
        );
    }

    stages[0].input_file = NULL;
    stages[0].output_file = NULL;
    unlink(INPUT_TEST_FILE);


    /*
     * Test: Successful output redirection in truncate and append modes.
     */
    char *redirect_first_args[] = {"printf", "first", NULL};
    char *redirect_second_args[] = {"printf", "second", NULL};

    init_command(&stages[0], redirect_first_args);
    stages[0].output_file = TEST_FILE;
    stages[0].append_mode = 0;

    unlink(TEST_FILE);
    total++;

    int first_redirect_status = pipeline_execute(commands, 1);
    int first_redirect_ok = check_output("first");

    init_command(&stages[0], redirect_second_args);
    stages[0].output_file = TEST_FILE;
    stages[0].append_mode = 1;

    int append_redirect_status = pipeline_execute(commands, 1);
    int append_redirect_ok = check_output("firstsecond");

    if (first_redirect_status == 0 && first_redirect_ok &&
        append_redirect_status == 0 && append_redirect_ok) {
        printf("PASS: pipeline output redirection truncate and append\n");
        passed++;
    } else {
        printf(
            "FAIL: output redirection (truncate=%d/%d, append=%d/%d)\n",
            first_redirect_status, first_redirect_ok,
            append_redirect_status, append_redirect_ok
        );
    }

    stages[0].output_file = NULL;
    stages[0].append_mode = 0;
    unlink(TEST_FILE);

    /*
     * Additional validation and error-path tests.
     */

    /* Test: Null pipeline stage. */
    ParsedCommand *null_stage_commands[1] = {NULL};

    total++;
    status = pipeline_execute(null_stage_commands, 1);

    if (status == 2) {
        printf("PASS: null pipeline stage rejected\n");
        passed++;
    } else {
        printf("FAIL: null pipeline stage (status=%d)\n", status);
    }

    /* Test: Empty pipeline stage. */
    char *empty_args[] = {NULL};
    init_command(&stages[0], empty_args);

    total++;
    status = pipeline_execute(commands, 1);

    if (status == 2) {
        printf("PASS: empty pipeline stage rejected\n");
        passed++;
    } else {
        printf("FAIL: empty pipeline stage (status=%d)\n", status);
    }

    /* Test: Failed input redirection. */
    char *failure_cat_args[] = {"cat", NULL};
    init_command(&stages[0], failure_cat_args);
    stages[0].input_file =
        "/tmp/simple_shell_pipeline_missing_input_12345";

    total++;
    status = pipeline_execute(commands, 1);

    if (status == 1) {
        printf("PASS: failed pipeline input redirection\n");
        passed++;
    } else {
        printf("FAIL: input redirection failure (status=%d)\n", status);
    }

    stages[0].input_file = NULL;

    /* Test: Failed output redirection. */
    init_command(&stages[0], printf_args);
    stages[0].output_file =
        "/tmp/simple_shell_pipeline_missing_dir_12345/output.txt";

    total++;
    status = pipeline_execute(commands, 1);

    if (status == 1) {
        printf("PASS: failed pipeline output redirection\n");
        passed++;
    } else {
        printf("FAIL: output redirection failure (status=%d)\n", status);
    }

    stages[0].output_file = NULL;

    /* Test: Final pipeline stage terminated by a signal. */
    char *signal_args[] = {
        "sh", "-c", "kill -TERM $$", NULL
    };
    init_command(&stages[0], signal_args);

    total++;
    status = pipeline_execute(commands, 1);

    if (status == 128 + SIGTERM) {
        printf("PASS: final pipeline stage signal termination\n");
        passed++;
    } else {
        printf("FAIL: signal termination status (got %d)\n", status);
    }

    /* Restore a valid command for subsequent tests. */
    init_command(&stages[0], printf_args);

    /*
     * Test 6: Reject more than five stages before array access.
     * The supplied array contains only one pointer.
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
            "FAIL: file descriptor count changed (before=%d, after=%d)\n",
            before, after
        );
    }

    /*
     * Test 8: Background pipeline returns and its children complete.
     */

    char *background_printf_args[] = {
        "printf", "background-pipeline", NULL
    };

    char *background_cat_args[] = {"cat", NULL};

    init_command(&stages[0], background_printf_args);
    init_command(&stages[1], background_cat_args);

    stages[1].output_file = TEST_FILE;
    stages[1].is_background = true;

    unlink(TEST_FILE);

    total++;
    status = pipeline_execute(commands, 2);

    int children_reaped = 0;

    for (int i = 0; i < 2; i++) {
        int child_status;
        pid_t child;

        do {
            child = waitpid(-1, &child_status, 0);
        } while (child < 0 && errno == EINTR);

        if (child > 0)
            children_reaped++;
    }

    if (status == 0 && children_reaped == 2 &&
        check_output("background-pipeline")) {
        printf(
            "PASS: background pipeline executes asynchronously and completes\n"
        );
        passed++;
    } else {
        printf(
            "FAIL: background pipeline (status=%d, reaped=%d)\n",
            status, children_reaped
        );
    }

    /*
     * Test 9: All background pipeline stages share one separate
     * process group.
     */

    unlink(BG_STAGE1_FILE);
    unlink(BG_STAGE2_FILE);

    char stage1_command[256];
    char stage2_command[256];

    snprintf(
        stage1_command,
        sizeof(stage1_command),
        "echo $$ > %s; printf x",
        BG_STAGE1_FILE
    );

    snprintf(
        stage2_command,
        sizeof(stage2_command),
        "echo $$ > %s; cat > /dev/null; sleep 0.4",
        BG_STAGE2_FILE
    );

    char *stage1_args[] = {
        "sh", "-c", stage1_command, NULL
    };

    char *stage2_args[] = {
        "sh", "-c", stage2_command, NULL
    };

    init_command(&stages[0], stage1_args);
    init_command(&stages[1], stage2_args);

    stages[1].is_background = true;

    total++;

    status = pipeline_execute(commands, 2);

    pid_t stage1_pid = -1;
    pid_t stage2_pid = -1;

    int pid_files_ok = wait_for_pid_files(&stage1_pid, &stage2_pid);

    int group_check_ok = 0;

    if (pid_files_ok) {
        pid_t stage1_pgid = getpgid(stage1_pid);
        pid_t stage2_pgid = getpgid(stage2_pid);

        group_check_ok =
            stage1_pgid > 0 &&
            stage1_pgid == stage2_pgid &&
            stage1_pgid == stage1_pid &&
            stage1_pgid != getpgrp();
    }

    /*
     * Reap both background pipeline children.
     */
    children_reaped = 0;

    for (int i = 0; i < 2; i++) {
        int child_status;
        pid_t child;

        do {
            child = waitpid(-1, &child_status, 0);
        } while (child < 0 && errno == EINTR);

        if (child > 0)
            children_reaped++;
    }

    if (status == 0 && pid_files_ok && group_check_ok &&
        children_reaped == 2) {
        printf(
            "PASS: background pipeline stages share one separate process group\n"
        );
        passed++;
    } else {
        printf(
            "FAIL: background pipeline process-group isolation "
            "(status=%d, pid_files=%d, group_check=%d, reaped=%d)\n",
            status, pid_files_ok, group_check_ok, children_reaped
        );
    }

    unlink(BG_STAGE1_FILE);
    unlink(BG_STAGE2_FILE);
    unlink(TEST_FILE);

    printf("\nPipeline tests: %d/%d passed\n", passed, total);

    return passed == total ? EXIT_SUCCESS : EXIT_FAILURE;
}