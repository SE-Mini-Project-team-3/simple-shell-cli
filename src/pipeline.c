
#include "pipeline.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int pipeline_execute(ParsedCommand *stages[], int stage_count)
{
    if (!stages || stage_count < 1 ||
        stage_count > SHELL_MAX_PIPELINE_STAGES) {
        fprintf(stderr, "shell: invalid pipeline\n");
        return 2;
    }

    int pipes[SHELL_MAX_PIPELINE_STAGES - 1][2];
    pid_t pids[SHELL_MAX_PIPELINE_STAGES];

    for (int i = 0; i < stage_count; i++) {
        if (!stages[i] || !stages[i]->argv[0]) {
            fprintf(stderr, "shell: empty pipeline stage\n");
            return 2;
        }
    }

    for (int i = 0; i < stage_count - 1; i++) {
        if (pipe(pipes[i]) < 0) {
            perror("pipe");
            for (int j = 0; j < i; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }
            return 1;
        }
    }

    int started = 0;

    for (int i = 0; i < stage_count; i++) {
        pid_t pid = fork();

        if (pid < 0) {
            perror("fork");
            for (int j = 0; j < stage_count - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }
            for (int j = 0; j < started; j++)
                waitpid(pids[j], NULL, 0);
            return 1;
        }

        if (pid == 0) {
            if (i > 0 && dup2(pipes[i - 1][0], STDIN_FILENO) < 0) {
                perror("dup2");
                _exit(1);
            }

            if (i < stage_count - 1 &&
                dup2(pipes[i][1], STDOUT_FILENO) < 0) {
                perror("dup2");
                _exit(1);
            }

            for (int j = 0; j < stage_count - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            if (stages[i]->input_file) {
                int fd = open(stages[i]->input_file, O_RDONLY);
                if (fd < 0 || dup2(fd, STDIN_FILENO) < 0) {
                    perror(stages[i]->input_file);
                    if (fd >= 0)
                        close(fd);
                    _exit(1);
                }
                close(fd);
            }

            if (stages[i]->output_file) {
                int flags = O_WRONLY | O_CREAT |
                    (stages[i]->append_mode ? O_APPEND : O_TRUNC);
                int fd = open(stages[i]->output_file, flags, 0644);
                if (fd < 0 || dup2(fd, STDOUT_FILENO) < 0) {
                    perror(stages[i]->output_file);
                    if (fd >= 0)
                        close(fd);
                    _exit(1);
                }
                close(fd);
            }

            execvp(stages[i]->argv[0], stages[i]->argv);
            perror(stages[i]->argv[0]);
            _exit(127);
        }

        pids[started++] = pid;
    }

    for (int i = 0; i < stage_count - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    int last_status = 0;

    for (int i = 0; i < started; i++) {
        int status;
        if (waitpid(pids[i], &status, 0) < 0) {
            perror("waitpid");
            last_status = 1;
        } else if (i == started - 1) {
            if (WIFEXITED(status))
                last_status = WEXITSTATUS(status);
            else if (WIFSIGNALED(status))
                last_status = 128 + WTERMSIG(status);
            else
                last_status = 1;
        }
    }

    return last_status;
}
