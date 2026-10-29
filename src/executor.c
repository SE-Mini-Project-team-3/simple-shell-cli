
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>

#include "executor.h"

int executor_run(const ParsedCommand *cmd)
{
    if (cmd == NULL || cmd->argv[0] == NULL) {
        fprintf(stderr, "shell: invalid command\n");
        return 1;
    }

    pid_t pid = fork();

    if (pid < 0) {
        perror("shell: fork");
        return 1;
    }

    if (pid == 0) {
        signal(SIGINT, SIG_DFL);
        execvp(cmd->argv[0], cmd->argv);

        fprintf(stderr, "shell: %s: %s\n",
                cmd->argv[0], strerror(errno));
        _exit(127);
    }

    if (cmd->is_background) {
        printf("[Background PID: %d]\n", (int)pid);
        return 0;
    }

    int status;

    while (waitpid(pid, &status, 0) < 0) {
        if (errno == EINTR) {
            continue;
        }

        perror("shell: waitpid");
        return 1;
    }

    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }

    if (WIFSIGNALED(status)) {
        return 128 + WTERMSIG(status);
    }

    return 1;
}
