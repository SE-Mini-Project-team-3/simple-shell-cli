
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "shell.h"
#include "parser.h"
#include "executor.h"

static int run_builtin(const ParsedCommand *cmd)
{
    if (strcmp(cmd->argv[0], "help") == 0) {
        puts("Simple Shell CLI");
        puts("Built-ins: cd, pwd, echo, exit, help");
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

        if (path == NULL) {
            path = getenv("HOME");
        }

        if (path == NULL || chdir(path) != 0) {
            perror("shell: cd");
            return 1;
        }

        return 0;
    }

    if (strcmp(cmd->argv[0], "echo") == 0) {
        for (int i = 1; cmd->argv[i] != NULL; i++) {
            if (i > 1) {
                putchar(' ');
            }
            fputs(cmd->argv[i], stdout);
        }

        putchar('\n');
        return 0;
    }

    if (strcmp(cmd->argv[0], "exit") == 0) {
        return -1;
    }

    return 1;
}

int main(void)
{
    char input[SHELL_MAX_INPUT];

    puts("Simple Shell CLI");
    puts("Type 'help' for available commands.");

    while (1) {
        printf("simple-shell> ");
        fflush(stdout);

        if (fgets(input, sizeof(input), stdin) == NULL) {
            putchar('\n');
            break;
        }

        if (strchr(input, '\n') == NULL && !feof(stdin)) {
            int ch;
            while ((ch = getchar()) != '\n' && ch != EOF) {
                /* Discard the remainder of an overlong input line. */
            }
            fprintf(stderr, "shell: input line too long\n");
            continue;
        }

        ParsedCommand cmd;

        if (parse_command(input, &cmd) != 0) {
            fprintf(stderr, "shell: invalid command\n");
            continue;
        }

        if (cmd.argv[0] == NULL) {
            free_parsed_command(&cmd);
            continue;
        }

        if (cmd.is_builtin) {
            int result = run_builtin(&cmd);
            free_parsed_command(&cmd);

            if (result == -1) {
                break;
            }
        } else {
            executor_run(&cmd);
            free_parsed_command(&cmd);
        }
    }

    return 0;
}
