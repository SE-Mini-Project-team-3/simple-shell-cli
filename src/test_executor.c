#include <stdio.h>
#include <assert.h>
#include <stdbool.h>
#include <unistd.h>
#include "executor.h"
#include "shell.h"

void test_executor_invalid_command(void) {
    printf("Running test: test_executor_invalid_command...\n");
    
    ParsedCommand cmd;
    cmd.argv[0] = "nonexistent_command_xyz";
    cmd.argv[1] = NULL;
    cmd.is_background = false;

    int result = executor_run(&cmd);
    // Stricter assertion checking for proper failure code/status
    assert(result < 0 || result == 127 || result == 1); 
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
    cmd.argv[0] = "sleep";
    cmd.argv[1] = "1";
    cmd.argv[2] = NULL;
    cmd.is_background = true;

    int result = executor_run(&cmd);
    assert(result == 0); 
    
    reap_background_jobs();
    
    printf("PASSED: test_executor_background\n");
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