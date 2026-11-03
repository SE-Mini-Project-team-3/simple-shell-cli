
#include "shell.h"
#include "parser.h"
#include "executor.h"
#include "pipeline.h"
#include "history.h"
#include "env_manager.h"

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
        puts("Built-ins: cd, pwd, echo, exit, help, history, export, unset, env");
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
        const char *home = getenv("HOME");
        char expanded_path[SHELL_MAX_INPUT + 1];

        if (path == NULL || strcmp(path, "~") == 0) {
            path = home;
        } else if (path[0] == '~' && path[1] == '/') {
            if (home == NULL) {
                fprintf(stderr, "shell: cd: HOME is not set\n");
                return 1;
            }

            int written = snprintf(expanded_path, sizeof(expanded_path),
                                   "%s%s", home, path + 1);

            if (written < 0 ||
                (size_t)written >= sizeof(expanded_path)) {
                fprintf(stderr, "shell: cd: path too long\n");
                return 1;
            }

            path = expanded_path;
        }

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

    if (strcmp(cmd->argv[0], "export") == 0) {
        if (cmd->argv[1] == NULL) {
            fprintf(stderr, "shell: export: usage: export NAME=VALUE\n");
            return 1;
        }

        int status = 0;

        for (int i = 1; cmd->argv[i] != NULL; i++) {
            char *eq = strchr(cmd->argv[i], '=');

            if (eq == NULL) {
                fprintf(stderr,
                        "shell: export: invalid argument '%s'\n",
                        cmd->argv[i]);
                status = 1;
                continue;
            }

            *eq = '\0';

            if (env_set(cmd->argv[i], eq + 1) != 0) {
                fprintf(stderr,
                        "shell: export: failed to set '%s'\n",
                        cmd->argv[i]);
                status = 1;
            }

            *eq = '=';
        }

        return status;
    }

    if (strcmp(cmd->argv[0], "unset") == 0) {
        if (cmd->argv[1] == NULL) {
            fprintf(stderr, "shell: unset: usage: unset NAME\n");
            return 1;
        }

        int status = 0;

        for (int i = 1; cmd->argv[i] != NULL; i++) {
            if (env_unset(cmd->argv[i]) != 0) {
                fprintf(stderr,
                        "shell: unset: failed to unset '%s'\n",
                        cmd->argv[i]);
                status = 1;
            }
        }

        return status;
    }

    if (strcmp(cmd->argv[0], "env") == 0) {
        env_print();
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
    int last_status = 0;

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
                while (ch != '\n' && ch != EOF)
                    ch = getchar();

                fprintf(stderr, "shell: input line too long\n");
                last_status = 2;
                continue;
            }
        }

        if (complete_line)
            line[--input_len] = '\0';

        if (input_len > SHELL_MAX_INPUT) {
            fprintf(stderr, "shell: input line too long\n");
            last_status = 2;
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
            last_status = 2;
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
                last_status = 2;
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

                last_status = result;
            } else {
                last_status = executor_run(cmd);
            }
        } else if (!failed) {
            last_status = pipeline_execute(stages, count);
        }

        for (int i = 0; i < parsed; i++)
            free_parsed_command(&commands[i]);
    }

    return last_status;
}
