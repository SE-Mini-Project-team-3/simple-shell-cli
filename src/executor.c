#include "executor.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int executor_run(const ParsedCommand *cmd)
{
    if (cmd == NULL || cmd->argv[0] == NULL) {
        fprintf(stderr, "shell: empty command\n");
        return 2;
    }

    pid_t pid = fork();

    if (pid < 0) {
        perror("shell: fork failed");
        return 1;
    }

    if (pid == 0) {
        /* Child process: restore normal Ctrl+C behavior. */
        struct sigaction sa_default;
        memset(&sa_default, 0, sizeof(sa_default));
        sa_default.sa_handler = SIG_DFL;
        sigemptyset(&sa_default.sa_mask);
        sigaction(SIGINT, &sa_default, NULL);

        /* Input redirection. */
        if (cmd->input_file != NULL) {
            int fd = open(cmd->input_file, O_RDONLY);
            if (fd < 0) {
                perror(cmd->input_file);
                _exit(1);
            }

            if (dup2(fd, STDIN_FILENO) < 0) {
                perror("dup2");
                close(fd);
                _exit(1);
            }
            close(fd);
        }

        /* Output redirection. */
        if (cmd->output_file != NULL) {
            int flags = O_WRONLY | O_CREAT |
                        (cmd->append_mode ? O_APPEND : O_TRUNC);

            int fd = open(cmd->output_file, flags, 0644);
            if (fd < 0) {
                perror(cmd->output_file);
                _exit(1);
            }

            if (dup2(fd, STDOUT_FILENO) < 0) {
                perror("dup2");
                close(fd);
                _exit(1);
            }
            close(fd);
        }

        execvp(cmd->argv[0], cmd->argv);

        fprintf(stderr, "shell: %s: %s\n",
                cmd->argv[0], strerror(errno));
        _exit(127);
    }

    /* Parent process. */
    if (cmd->is_background) {
        printf("[Background PID: %d]\n", pid);
        return 0;
    }

    int status;
    pid_t result;

    do {
        result = waitpid(pid, &status, 0);
    } while (result < 0 && errno == EINTR);

    if (result < 0) {
        perror("shell: waitpid failed");
        return 1;
    }

    if (WIFEXITED(status)) {
        int exit_code = WEXITSTATUS(status);

        /* Preserve the Sprint 1 executor test convention. */
        if (exit_code == 127)
            return 1;

        return exit_code;
    }

    if (WIFSIGNALED(status)) {
        fprintf(stderr,
                "shell: process terminated abnormally by signal %d\n",
                WTERMSIG(status));
        return 128 + WTERMSIG(status);
    }

    return 1;
}

/* Reap completed background processes without blocking. */
void reap_background_jobs(void)
{
    int status;

    while (waitpid(-1, &status, WNOHANG) > 0) {
        /* Completed background process reaped. */
    }
}

/* Prevent Ctrl+C from terminating the shell itself. */
void init_shell_signals(void)
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    sigaction(SIGINT, &sa, NULL);
}
