
#include "shell.h"
#include "parser.h"
#include "executor.h"
#include "pipeline.h"
#include "history.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

static int split_pipeline(char *line, char *parts[])
{
    int count = 0;
    char quote = '\0';
    char *start = line;

    for (char *p = line; ; p++) {
        if (*p == '\'' || *p == '"') {
            if (quote == '\0')
                quote = *p;
            else if (quote == *p)
                quote = '\0';
        }

        if ((*p == '|' && quote == '\0') || *p == '\0') {
            if (*p == '|') {
                *p = '\0';

                if (count >= SHELL_MAX_PIPELINE_STAGES - 1)
                    return -1;

                parts[count++] = start;
                start = p + 1;
            } else {
                if (quote != '\0')
                    return -1;

                parts[count++] = start;
                break;
            }
        }
    }

    return count;
}

static char *trim(char *s)
{
    while (*s == ' ' || *s == '\t' || *s == '\n')
        s++;

    size_t n = strlen(s);

    while (n > 0 &&
           (s[n - 1] == ' ' || s[n - 1] == '\t' || s[n - 1] == '\n')) {
        s[--n] = '\0';
    }

    return s;
}

static int run_builtin(const ParsedCommand *cmd)
{
    if (strcmp(cmd->argv[0], "help") == 0) {
        puts("Simple Shell CLI");
        puts("Built-ins: cd, pwd, echo, exit, help, history");
        puts("External commands are executed through the process executor.");
        return 0;
    }

    if (strcmp(cmd->argv[0], "pwd") == 0) {
        char cwd[SHELL_MAX_INPUT];

        if (getcwd(cwd, sizeof(cwd)) == NULL) {
            perror("shell: pwd");
            return 1;
        }

        puts(cwd);
        return 0;
    }

    if (strcmp(cmd->argv[0], "cd") == 0) {
        const char *path = cmd->argv[1];

        if (path == NULL)
            path = getenv("HOME");

        if (path == NULL || chdir(path) != 0) {
            perror("shell: cd");
            return 1;
        }

        return 0;
    }

    if (strcmp(cmd->argv[0], "echo") == 0) {
        for (int i = 1; cmd->argv[i] != NULL; i++) {
            if (i > 1)
                putchar(' ');

            fputs(cmd->argv[i], stdout);
        }

        putchar('\n');
        return 0;
    }

    if (strcmp(cmd->argv[0], "history") == 0) {
        history_print();
        return 0;
    }

    if (strcmp(cmd->argv[0], "exit") == 0)
        return -1;

    return 1;
}

static int run_builtin_with_redirection(const ParsedCommand *cmd)
{
    int saved_stdin = dup(STDIN_FILENO);
    int saved_stdout = dup(STDOUT_FILENO);

    if (saved_stdin < 0 || saved_stdout < 0) {
        perror("shell: dup");

        if (saved_stdin >= 0)
            close(saved_stdin);

        if (saved_stdout >= 0)
            close(saved_stdout);

        return 1;
    }

    int result = 0;
    int fd;

    if (cmd->input_file != NULL) {
        fd = open(cmd->input_file, O_RDONLY);

        if (fd < 0 || dup2(fd, STDIN_FILENO) < 0) {
            perror(cmd->input_file);

            if (fd >= 0)
                close(fd);

            result = 1;
            goto restore;
        }

        close(fd);
    }

    if (cmd->output_file != NULL) {
        int flags = O_WRONLY | O_CREAT |
                    (cmd->append_mode ? O_APPEND : O_TRUNC);

        fd = open(cmd->output_file, flags, 0644);

        if (fd < 0 || dup2(fd, STDOUT_FILENO) < 0) {
            perror(cmd->output_file);

            if (fd >= 0)
                close(fd);

            result = 1;
            goto restore;
        }

        close(fd);
    }

    result = run_builtin(cmd);
    fflush(stdout);

restore:
    if (dup2(saved_stdin, STDIN_FILENO) < 0 ||
        dup2(saved_stdout, STDOUT_FILENO) < 0) {
        perror("shell: restoring standard streams");
        result = 1;
    }

    close(saved_stdin);
    close(saved_stdout);

    return result;
}

int main(void)
{
    /*
     * Allow SHELL_MAX_INPUT characters, an optional newline,
     * and the terminating null character.
     */
    char line[SHELL_MAX_INPUT + 2];

    init_shell_signals();

    puts("Simple Shell CLI");
    puts("Type 'help' for available commands.");

    while (1) {
        reap_background_jobs();

        printf("simple-shell> ");
        fflush(stdout);

        if (fgets(line, sizeof(line), stdin) == NULL) {
            putchar('\n');
            break;
        }

        size_t input_len = strlen(line);
        int complete_line =
            (input_len > 0 && line[input_len - 1] == '\n');

        /*
         * If no newline was read, check whether more input remains.
         * This distinguishes a complete final line from an
         * oversized line that has not been fully read.
         */
        if (!complete_line && !feof(stdin)) {
            int ch = getchar();

            if (ch != '\n' && ch != EOF) {
                /* Discard the remainder of the oversized line. */
                while (ch != '\n' && ch != EOF)
                    ch = getchar();

                fprintf(stderr, "shell: input line too long\n");
                continue;
            }
        }

        /* Remove the newline if one was read. */
        if (complete_line)
            line[--input_len] = '\0';

        /* Enforce the maximum input length. */
        if (input_len > SHELL_MAX_INPUT) {
            fprintf(stderr, "shell: input line too long\n");
            continue;
        }

        char *command_line = trim(line);

        if (*command_line == '\0')
            continue;

        history_add(command_line);

        char *parts[SHELL_MAX_PIPELINE_STAGES];
        int count = split_pipeline(command_line, parts);

        if (count < 1) {
            fprintf(stderr,
                    "shell: invalid pipeline or too many stages\n");
            continue;
        }

        ParsedCommand commands[SHELL_MAX_PIPELINE_STAGES] = {0};
        ParsedCommand *stages[SHELL_MAX_PIPELINE_STAGES];
        int parsed = 0;
        int failed = 0;

        for (int i = 0; i < count; i++) {
            parts[i] = trim(parts[i]);

            if (*parts[i] == '\0' ||
                parse_command(parts[i], &commands[i]) != 0) {
                fprintf(stderr, "shell: invalid command syntax\n");
                failed = 1;
                break;
            }

            parsed++;
            stages[i] = &commands[i];
        }

        if (!failed && count == 1) {
            ParsedCommand *cmd = &commands[0];

            if (cmd->is_builtin) {
                int result;

                if (strcmp(cmd->argv[0], "cd") == 0 ||
                    strcmp(cmd->argv[0], "exit") == 0) {
                    result = run_builtin(cmd);
                } else {
                    result = run_builtin_with_redirection(cmd);
                }

                if (result == -1) {
                    for (int i = 0; i < parsed; i++)
                        free_parsed_command(&commands[i]);

                    break;
                }
            } else {
                executor_run(cmd);
            }
        } else if (!failed) {
            pipeline_execute(stages, count);
        }

        for (int i = 0; i < parsed; i++)
            free_parsed_command(&commands[i]);
    }

    return 0;
}
