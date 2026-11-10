#define _DEFAULT_SOURCE

#include <stdio.h>
#include <assert.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

#include "executor.h"
#include "shell.h"
#include <sys/stat.h>

#define BACKGROUND_TEST_FILE "/tmp/simple_shell_executor_bg_test.txt"

static pid_t read_background_pid(void)
{
    FILE *file = fopen(BACKGROUND_TEST_FILE, "r");

    if (file == NULL)
        return -1;

    long value = -1;
    int result = fscanf(file, "%ld", &value);
    fclose(file);

    if (result != 1 || value <= 0)
        return -1;

    return (pid_t)value;
}

static pid_t wait_for_background_pid_file(void)
{
    for (int i = 0; i < 100; i++) {
        pid_t pid = read_background_pid();

        if (pid > 0)
            return pid;

        usleep(10000);
    }

    return -1;
}

void test_executor_invalid_command(void)
{
    printf("Running test: test_executor_invalid_command...\n");

    ParsedCommand cmd = {0};
    cmd.argv[0] = "nonexistent_command_xyz_12345";
    cmd.argv[1] = NULL;
    cmd.is_background = false;

    int result = executor_run(&cmd);
    assert(result == 127);

    printf("PASSED: test_executor_invalid_command\n");
}

void test_executor_foreground_valid(void)
{
    printf("Running test: test_executor_foreground_valid...\n");

    ParsedCommand cmd = {0};
    cmd.argv[0] = "true";
    cmd.argv[1] = NULL;
    cmd.is_background = false;

    int result = executor_run(&cmd);
    assert(result == 0);

    printf("PASSED: test_executor_foreground_valid\n");
}

void test_executor_background(void)
{
    printf("Running test: test_executor_background...\n");

    unlink(BACKGROUND_TEST_FILE);

    char command[256];
    snprintf(command, sizeof(command),
             "echo $$ > %s; sleep 0.2",
             BACKGROUND_TEST_FILE);

    ParsedCommand cmd = {0};
    cmd.argv[0] = "sh";
    cmd.argv[1] = "-c";
    cmd.argv[2] = command;
    cmd.argv[3] = NULL;
    cmd.is_background = true;

    int result = executor_run(&cmd);
    assert(result == 0);

    pid_t background_pid = wait_for_background_pid_file();

    assert(background_pid > 0);

    /*
     * A background external command must be the leader of its own
     * process group, separate from the test process group.
     */
    pid_t background_pgid = getpgid(background_pid);

    assert(background_pgid == background_pid);
    assert(background_pgid != getpgrp());

    usleep(300000);
    reap_background_jobs();

    errno = 0;
    pid_t res = waitpid(-1, NULL, WNOHANG);

    assert(res == -1);
    assert(errno == ECHILD);

    unlink(BACKGROUND_TEST_FILE);

    printf("PASSED: test_executor_background\n");
}

void test_executor_abnormal_termination(void)
{
    printf("Running test: test_executor_abnormal_termination...\n");

    ParsedCommand cmd = {0};
    cmd.argv[0] = "sh";
    cmd.argv[1] = "-c";
    cmd.argv[2] = "kill -TERM $$";
    cmd.argv[3] = NULL;
    cmd.is_background = false;

    int result = executor_run(&cmd);

    assert(result == 128 + SIGTERM);

    printf("PASSED: test_executor_abnormal_termination\n");
}

void test_executor_child_sigint(void)
{
    printf("Running test: test_executor_child_sigint...\n");

    ParsedCommand cmd = {0};
    cmd.argv[0] = "sh";
    cmd.argv[1] = "-c";
    cmd.argv[2] = "kill -INT $$";
    cmd.argv[3] = NULL;
    cmd.is_background = false;

    int result = executor_run(&cmd);

    assert(result == 128 + SIGINT);

    printf("PASSED: test_executor_child_sigint\n");
}

void test_shell_ignores_sigint(void)
{
    printf("Running test: test_shell_ignores_sigint...\n");

    struct sigaction sa;

    assert(sigaction(SIGINT, NULL, &sa) == 0);
    assert(sa.sa_handler == SIG_IGN);

    printf("PASSED: test_shell_ignores_sigint\n");
}

void test_executor_null_command(void)
{
    ParsedCommand cmd = {0};
    assert(executor_run(&cmd) == 2);
    printf("PASSED: null command\n");
}

void test_executor_redirection(void)
{
    const char *input = "/tmp/executor_input.txt";
    const char *output = "/tmp/executor_output.txt";

    FILE *file = fopen(input, "w");
    assert(file != NULL);
    fprintf(file, "redirection works\n");
    fclose(file);

    ParsedCommand cmd = {0};
    cmd.argv[0] = "cat";
    cmd.input_file = (char *)input;
    cmd.output_file = (char *)output;

    assert(executor_run(&cmd) == 0);

    file = fopen(output, "r");
    assert(file != NULL);

    char buffer[128] = {0};
    assert(fgets(buffer, sizeof(buffer), file) != NULL);
    fclose(file);

    assert(strcmp(buffer, "redirection works\n") == 0);

    unlink(input);
    unlink(output);
    printf("PASSED: input and output redirection\n");
}

void test_executor_append_redirection(void)
{
    const char *output = "/tmp/executor_append.txt";

    FILE *file = fopen(output, "w");
    assert(file != NULL);
    fprintf(file, "first");
    fclose(file);

    ParsedCommand cmd = {0};
    cmd.argv[0] = "printf";
    cmd.argv[1] = "second";
    cmd.output_file = (char *)output;
    cmd.append_mode = true;

    assert(executor_run(&cmd) == 0);

    file = fopen(output, "r");
    assert(file != NULL);

    char buffer[128] = {0};
    assert(fgets(buffer, sizeof(buffer), file) != NULL);
    fclose(file);

    assert(strcmp(buffer, "firstsecond") == 0);

    unlink(output);
    printf("PASSED: append redirection\n");
}

void test_executor_redirection_failure(void)
{
    ParsedCommand cmd = {0};
    cmd.argv[0] = "cat";
    cmd.input_file = "/nonexistent/executor_input_file";

    assert(executor_run(&cmd) == 1);
    printf("PASSED: input redirection failure\n");
}

void test_executor_output_redirection_failure(void)
{
    ParsedCommand cmd = {0};
    cmd.argv[0] = "printf";
    cmd.argv[1] = "test";
    cmd.output_file = "/nonexistent/executor_output_file";

    assert(executor_run(&cmd) == 1);
    printf("PASSED: output redirection failure\n");
}

void test_executor_permission_denied(void)
{
    char path[] = "/tmp/executor_no_permission_XXXXXX";
    int fd = mkstemp(path);
    assert(fd >= 0);
    close(fd);

    assert(chmod(path, 0600) == 0);

    ParsedCommand cmd = {0};
    cmd.argv[0] = path;

    assert(executor_run(&cmd) == 126);

    unlink(path);
    printf("PASSED: permission denied (exit 126)\n");
}

int main(void)
{
    printf("=== STARTING EXECUTOR UNIT TESTS ===\n");

    init_shell_signals();

    test_executor_foreground_valid();
    test_executor_invalid_command();
    test_executor_background();
    test_executor_abnormal_termination();
    test_executor_child_sigint();
    test_shell_ignores_sigint();
    test_executor_null_command();
    test_executor_redirection();
    test_executor_append_redirection();
    test_executor_redirection_failure();
    test_executor_output_redirection_failure();
    test_executor_permission_denied();

    printf("=== ALL EXECUTOR TESTS PASSED SUCCESSFULLY ===\n");

    return 0;
}
