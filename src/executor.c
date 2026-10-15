#include "executor.h"

#include <fcntl.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>

int executor_run(const ParsedCommand *cmd)
{
    if (!cmd || !cmd->argv[0]) {
        fprintf(stderr, "shell: empty command\n");
        return 2;
    }

    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        if (cmd->input_file) {
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

        if (cmd->output_file) {
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
        perror(cmd->argv[0]);
        _exit(127);
    }

    if (cmd->is_background)
        return 0;

    int status;
    while (waitpid(pid, &status, 0) < 0) {
        perror("waitpid");
        return 1;
    }

    if (WIFEXITED(status))
        return WEXITSTATUS(status);

    if (WIFSIGNALED(status))
        return 128 + WTERMSIG(status);

    return 1;
}