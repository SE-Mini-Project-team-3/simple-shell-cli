
#include "pipeline.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int pipeline_execute(ParsedCommand *stages[], int stage_count)
{
    if (stages == NULL || stage_count < 1 ||
        stage_count > SHELL_MAX_PIPELINE_STAGES) {
        fprintf(stderr, "shell: invalid pipeline\n");
        return 2;
    }

    int pipes[SHELL_MAX_PIPELINE_STAGES - 1][2];
    pid_t pids[SHELL_MAX_PIPELINE_STAGES];

    /* Validate all pipeline stages before creating processes. */
    for (int i = 0; i < stage_count; i++) {
        if (stages[i] == NULL || stages[i]->argv[0] == NULL) {
            fprintf(stderr, "shell: empty pipeline stage\n");
            return 2;
        }
    }

    /* Create the required pipes. */
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

    /* Fork one child process for each pipeline stage. */
    for (int i = 0; i < stage_count; i++) {
        pid_t pid = fork();

        if (pid < 0) {
            perror("fork");

            /* Close all pipe descriptors. */
            for (int j = 0; j < stage_count - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            /* Reap children that were already created. */
            for (int j = 0; j < started; j++) {
                pid_t result;

                do {
                    result = waitpid(pids[j], NULL, 0);
                } while (result < 0 && errno == EINTR);
            }

            return 1;
        }

        if (pid == 0) {
            /*
             * Restore default SIGINT handling in pipeline children.
             * The parent shell may ignore SIGINT, but child processes
             * must respond to Ctrl+C normally.
             */
            struct sigaction sa = {0};
            sa.sa_handler = SIG_DFL;
            sigemptyset(&sa.sa_mask);

            if (sigaction(SIGINT, &sa, NULL) < 0) {
                perror("sigaction");
                _exit(1);
            }

            /* Connect input from the previous pipeline stage. */
            if (i > 0 &&
                dup2(pipes[i - 1][0], STDIN_FILENO) < 0) {
                perror("dup2 stdin");
                _exit(1);
            }

            /* Connect output to the next pipeline stage. */
            if (i < stage_count - 1 &&
                dup2(pipes[i][1], STDOUT_FILENO) < 0) {
                perror("dup2 stdout");
                _exit(1);
            }

            /* Close unused pipe descriptors in the child. */
            for (int j = 0; j < stage_count - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            /* Apply input redirection if specified. */
            if (stages[i]->input_file != NULL) {
                int fd = open(stages[i]->input_file, O_RDONLY);

                if (fd < 0 || dup2(fd, STDIN_FILENO) < 0) {
                    perror(stages[i]->input_file);

                    if (fd >= 0)
                        close(fd);

                    _exit(1);
                }

                close(fd);
            }

            /* Apply output redirection if specified. */
            if (stages[i]->output_file != NULL) {
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

            /* Execute the command for this pipeline stage. */
            execvp(stages[i]->argv[0], stages[i]->argv);

            perror(stages[i]->argv[0]);
            _exit(127);
        }

        pids[started++] = pid;
    }

    /* Close all pipe descriptors in the parent. */
    for (int i = 0; i < stage_count - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    /*
     * Wait for every child and return the exit status
     * of the final pipeline stage.
     */
    int last_status = 0;

    for (int i = 0; i < started; i++) {
        int status;
        pid_t result;

        do {
            result = waitpid(pids[i], &status, 0);
        } while (result < 0 && errno == EINTR);

        if (result < 0) {
            perror("waitpid");

            if (i == started - 1)
                last_status = 1;

            continue;
        }

        if (i == started - 1) {
            if (WIFEXITED(status)) {
                last_status = WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                last_status = 128 + WTERMSIG(status);
            } else {
                last_status = 1;
            }
        }
    }

    return last_status;
}
