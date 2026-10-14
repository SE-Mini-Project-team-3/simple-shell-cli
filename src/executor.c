#include "executor.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>
#include <signal.h>

int executor_run(const ParsedCommand *cmd)
{
    if (cmd == NULL || cmd->argv == NULL || cmd->argv[0] == NULL) {
        return -1;
    }

    // FR-PE-01: Create a child process using fork()[cite: 16]
    pid_t pid = fork();
    
    if (pid < 0) {
        perror("shell: fork failed");
        return -1;
    } 
    else if (pid == 0) {
        // --- CHILD PROCESS ---
        
        // FR-PE-07: Restore default signal behavior for the foreground/background child 
        // so it can respond to signals like SIGINT (Ctrl+C) normally[cite: 16].
        struct sigaction sa_default;
        sa_default.sa_handler = SIG_DFL;
        sigemptyset(&sa_default.sa_mask);
        sa_default.sa_flags = 0;
        sigaction(SIGINT, &sa_default, NULL);

        // FR-PE-02: Execute external programs using an exec-family function[cite: 16]
        execvp(cmd->argv[0], cmd->argv);
        
        // If execvp returns, an error occurred (e.g., command not found)
        // Complies with SAD §4.4 standard error format: "shell: <command>: <reason>"[cite: 10]
        fprintf(stderr, "shell: %s: %s\n", cmd->argv[0], strerror(errno));
        exit(EXIT_FAILURE);
    } 
    else {
        // --- PARENT PROCESS ---
        if (!cmd->is_background) {
            int status;
            
            // FR-PE-03: Wait for foreground processes to complete[cite: 16]
            if (waitpid(pid, &status, 0) < 0) {
                perror("shell: waitpid failed");
                return -1;
            }
            
            // FR-PE-05: Report abnormal process termination[cite: 16]
            if (WIFEXITED(status)) {
                return WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                fprintf(stderr, "shell: process terminated abnormally by signal %d\n", WTERMSIG(status));
                return 128 + WTERMSIG(status);
            }
        } else {
            // FR-PE-04: Background execution - print background PID and return immediately[cite: 16]
            printf("[Background PID: %d]\n", pid);
            return 0;
        }
    }
    
    return 0;
}

// FR-PE-06 / SEC-04: Non-blocking background child reaping function[cite: 16, 17]
// Call this function periodically in your main shell prompt loop to clean up finished background jobs.
void reap_background_jobs(void) {
    int status;
    // WNOHANG ensures waitpid doesn't block if no background children have exited yet.
    while (waitpid(-1, &status, WNOHANG) > 0) {
        // Successfully reaped a completed background process, preventing zombie states[cite: 8, 16].
    }
}

// FR-PE-07 / SEC-04: Initialize shell-level signal handling to prevent Ctrl+C from killing the shell itself[cite: 16, 17].
void init_shell_signals(void) {
    struct sigaction sa;
    sa.sa_handler = SIG_IGN; // Ignore SIGINT (Ctrl+C) in the parent shell loop
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
}