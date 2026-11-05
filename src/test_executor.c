
#define _DEFAULT_SOURCE

#include <stdio.h>
#include <assert.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>

#include "executor.h"
#include "shell.h"

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

    ParsedCommand cmd = {0};
    cmd.argv[0] = "true";
    cmd.argv[1] = NULL;
    cmd.is_background = true;

    int result = executor_run(&cmd);
    assert(result == 0);

    usleep(100000);
    reap_background_jobs();

    errno = 0;
    pid_t res = waitpid(-1, NULL, WNOHANG);

    assert(res == -1);
    assert(errno == ECHILD);

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

    printf("=== ALL EXECUTOR TESTS PASSED SUCCESSFULLY ===\n");

    return 0;
}
