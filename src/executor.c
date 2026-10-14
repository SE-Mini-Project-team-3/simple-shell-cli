#include "executor.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>

int executor_run(const ParsedCommand *cmd)
{
    if (cmd == NULL || cmd->argv == NULL || cmd->argv[0] == NULL) {
        return -1;
    }

    // FR-PE-01: Create a child process using fork()
    pid_t pid = fork();
    
    if (pid < 0) {
        perror("shell: fork failed");
        return -1;
    } 
    else if (pid == 0) {
        // --- CHILD PROCESS ---
        // FR-PE-02: Execute external programs using an exec-family function
        execvp(cmd->argv[0], cmd->argv);
        
        // If execvp returns, an error occurred (e.g., command not found)
        perror("shell: command execution failed");
        exit(EXIT_FAILURE);
    } 
    else {
        // --- PARENT PROCESS ---
        if (!cmd->is_background) {
            int status;
            
            // FR-PE-03: Wait for foreground processes to complete
            if (waitpid(pid, &status, 0) < 0) {
                perror("shell: waitpid failed");
                return -1;
            }
            
            // FR-PE-05: Report abnormal process termination
            if (WIFEXITED(status)) {
                return WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                fprintf(stderr, "shell: process terminated abnormally by signal %d\n", WTERMSIG(status));
                return 128 + WTERMSIG(status);
            }
        } else {
            // For background jobs, you can optionally print the PID or handle job tracking here
            printf("[Background PID: %d]\n", pid);
        }
    }
    
    return 0;
}