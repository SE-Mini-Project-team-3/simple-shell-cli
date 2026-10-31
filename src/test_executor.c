#define _DEFAULT_SOURCE
#include <stdio.h>
#include <assert.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include "executor.h"
#include "shell.h"

void test_executor_invalid_command(void) {
    printf("Running test: test_executor_invalid_command...\n");
    
    ParsedCommand cmd;
    cmd.argv[0] = "nonexistent_command_xyz_12345";
    cmd.argv[1] = NULL;
    cmd.is_background = false;

    int result = executor_run(&cmd);
    assert(result == 127); 
    printf("PASSED: test_executor_invalid_command\n");
}

void test_executor_foreground_valid(void) {
    printf("Running test: test_executor_foreground_valid...\n");
    
    ParsedCommand cmd;
    cmd.argv[0] = "true";
    cmd.argv[1] = NULL;
    cmd.is_background = false;

    int result = executor_run(&cmd);
    assert(result == 0);
    printf("PASSED: test_executor_foreground_valid\n");
}

void test_executor_background(void) {
    printf("Running test: test_executor_background...\n");
    
    ParsedCommand cmd;
    cmd.argv[0] = "true";
    cmd.argv[1] = NULL;
    cmd.is_background = true;

    // Run a background job and capture its execution or verify reaping behavior
    int result = executor_run(&cmd);
    assert(result == 0); 
    
    // Give a short pause for the background task to complete
    usleep(100000); // 100ms
    
    // Reap background jobs
    reap_background_jobs();
    
    // Verify reaping: trying to wait on any child should now yield no more active background children (ECHILD)
    errno = 0;
    pid_t res = waitpid(-1, NULL, WNOHANG);
    assert(res <= 0);
    
    printf("PASSED: test_executor_background (verified child reaping)\n");
}

int main(void) {
    printf("=== STARTING EXECUTOR UNIT TESTS ===\n");
    
    init_shell_signals();
    test_executor_foreground_valid();
    test_executor_invalid_command();
    test_executor_background();
    
    printf("=== ALL EXECUTOR TESTS PASSED SUCCESSFULLY ===\n");
    return 0;
}