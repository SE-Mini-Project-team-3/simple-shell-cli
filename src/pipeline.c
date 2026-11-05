
#include "pipeline.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

static void close_pipeline_pipes(
    int pipes[SHELL_MAX_PIPELINE_STAGES - 1][2],
    int pipe_count)
{
    for (int i = 0; i < pipe_count; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }
}

static void terminate_pipeline(
    pid_t pids[SHELL_MAX_PIPELINE_STAGES],
    int started,
    pid_t process_group)
{
    if (process_group > 0)
        kill(-process_group, SIGTERM);

    for (int i = 0; i < started; i++)
        kill(pids[i], SIGTERM);

    for (int i = 0; i < started; i++) {
        while (waitpid(pids[i], NULL, 0) < 0 && errno == EINTR) {
            /* Retry interrupted wait. */
        }
    }
}

int pipeline_execute(ParsedCommand *stages[], int stage_count)
{
    if (stages == NULL || stage_count < 1 ||
        stage_count > SHELL_MAX_PIPELINE_STAGES) {
        fprintf(stderr, "shell: invalid pipeline\n");
        return 2;
    }

    int pipes[SHELL_MAX_PIPELINE_STAGES - 1][2];
    int gate[2] = {-1, -1};
    pid_t pids[SHELL_MAX_PIPELINE_STAGES];

    /*
     * A trailing '&' is stored on the final parsed pipeline stage.
     */
    int is_background = stages[stage_count - 1] != NULL &&
                        stages[stage_count - 1]->is_background;

    /* Validate all pipeline stages before creating processes. */
    for (int i = 0; i < stage_count; i++) {
        if (stages[i] == NULL || stages[i]->argv[0] == NULL) {
            fprintf(stderr, "shell: empty pipeline stage\n");
            return 2;
        }
    }

    /* Create the required data pipes. */
    for (int i = 0; i < stage_count - 1; i++) {
        if (pipe(pipes[i]) < 0) {
            perror("pipe");
            close_pipeline_pipes(pipes, i);
            return 1;
        }
    }

    /*
     * Background children wait on this gate until every stage
     * has joined the shared process group.
     */
    if (is_background && pipe(gate) < 0) {
        perror("background pipeline gate");
        close_pipeline_pipes(pipes, stage_count - 1);
        return 1;
    }

    int started = 0;
    pid_t background_pgid = 0;

    /* Fork one child process for each pipeline stage. */
    for (int i = 0; i < stage_count; i++) {
        pid_t pid = fork();

        if (pid < 0) {
            perror("fork");

            close_pipeline_pipes(pipes, stage_count - 1);

            if (is_background) {
                terminate_pipeline(pids, started, background_pgid);
                close(gate[0]);
                close(gate[1]);
            } else {
                terminate_pipeline(pids, started, 0);
            }

            return 1;
        }

        if (pid == 0) {
            /*
             * All background stages belong to one process group.
             * The first child becomes the process-group leader.
             */
            if (is_background) {
                pid_t target_pgid =
                    (i == 0) ? 0 : background_pgid;

                if (setpgid(0, target_pgid) < 0) {
                    perror("shell: setpgid");
                    _exit(1);
                }

                /* The child must not keep the gate write end open. */
                close(gate[1]);

                /*
                 * Wait until the parent closes its gate write end.
                 * EOF means all pipeline stages have been created.
                 */
                char buffer;
                ssize_t result;

                do {
                    result = read(gate[0], &buffer, 1);
                } while (result < 0 && errno == EINTR);

                close(gate[0]);

                if (result < 0) {
                    perror("background pipeline gate read");
                    _exit(1);
                }
            }

            /*
             * Restore default SIGINT handling in pipeline children.
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

        /*
         * Parent process: establish the process group as well.
         */
        if (is_background) {
            if (i == 0)
                background_pgid = pid;

            if (setpgid(pid, background_pgid) < 0 &&
                errno != EACCES && errno != ESRCH) {
                perror("shell: setpgid");

                pids[started++] = pid;

                close_pipeline_pipes(pipes, stage_count - 1);
                terminate_pipeline(pids, started, background_pgid);

                close(gate[0]);
                close(gate[1]);

                return 1;
            }
        }

        pids[started++] = pid;
    }

    /* Close all data-pipe descriptors in the parent. */
    close_pipeline_pipes(pipes, stage_count - 1);

    if (is_background) {
        /*
         * Release every child simultaneously by closing the gate.
         * Children see EOF because they already closed gate[1].
         */
        close(gate[0]);
        close(gate[1]);

        printf("[Background PID: %d]\n", (int)pids[0]);
        fflush(stdout);
        return 0;
    }

    /*
     * Foreground pipelines wait for every child and return the exit
     * status of the final pipeline stage.
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
