
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>

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
    if (cmd == NULL || cmd->argv[0] == NULL) {
        fprintf(stderr, "shell: invalid command\n");
        return 1;
    }

    // FR-PE-01: Create a child process using fork()
    pid_t pid = fork();

    if (pid < 0) {
        perror("shell: fork");
        return 1;
    }

    if (pid == 0) {
        // Restore default SIGINT behavior in the child.
        struct sigaction sa_default;
        sa_default.sa_handler = SIG_DFL;
        sigemptyset(&sa_default.sa_mask);
        sa_default.sa_flags = 0;
        sigaction(SIGINT, &sa_default, NULL);

        // FR-PE-02: Execute the external program.
        execvp(cmd->argv[0], cmd->argv);

        // execvp returns only if execution fails.
        fprintf(stderr, "shell: %s: %s\n",
                cmd->argv[0], strerror(errno));
        _exit(127);
    }

    // FR-PE-04: Background execution.
    if (cmd->is_background) {
        printf("[Background PID: %d]\n", (int)pid);
        return 0;
    }

    // FR-PE-03: Wait for foreground processes.
    int status;

    while (waitpid(pid, &status, 0) < 0) {
        if (errno == EINTR) {
            continue;
        }

        perror("shell: waitpid");
        return 1;
    }

    // FR-PE-05: Return the child's exit status.
    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }

    if (WIFSIGNALED(status)) {
        fprintf(stderr,
                "shell: process terminated abnormally by signal %d\n",
                WTERMSIG(status));
        return 128 + WTERMSIG(status);
    }

    return 1;
}

// FR-PE-06 / SEC-04: Non-blocking background child reaping function
void reap_background_jobs(void) {
    int status;
    // WNOHANG ensures waitpid doesn't block if no background children have exited yet.
    while (waitpid(-1, &status, WNOHANG) > 0) {
        // Successfully reaped a completed background process, preventing zombie states.
    }
}

// FR-PE-07 / SEC-04: Initialize shell-level signal handling to prevent Ctrl+C from killing the shell itself
void init_shell_signals(void) {
    struct sigaction sa;
    sa.sa_handler = SIG_IGN; // Ignore SIGINT (Ctrl+C) in the parent shell loop
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
}