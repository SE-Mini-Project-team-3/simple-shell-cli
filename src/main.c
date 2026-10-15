#include "shell.h"
#include "parser.h"
#include "executor.h"
#include "pipeline.h"
#include "history.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int split_pipeline(char *line, char *parts[])
{
    int count = 0;
    char quote = '\0';
    char *start = line;

    for (char *p = line; ; p++) {
        if (*p == '\'' || *p == '"') {
            if (!quote)
                quote = *p;
            else if (quote == *p)
                quote = '\0';
        }

        if ((*p == '|' && !quote) || *p == '\0') {
            if (*p == '|') {
                *p = '\0';
                if (count >= SHELL_MAX_PIPELINE_STAGES - 1)
                    return -1;
                parts[count++] = start;
                start = p + 1;
            } else {
                if (quote)
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
    while (n && (s[n - 1] == ' ' || s[n - 1] == '\t' ||
                 s[n - 1] == '\n'))
        s[--n] = '\0';

    return s;
}

int main(void)
{
    char line[SHELL_MAX_INPUT];

    while (1) {
        printf("simple-shell> ");
        fflush(stdout);

        if (!fgets(line, sizeof(line), stdin))
            break;

        line[strcspn(line, "\n")] = '\0';
        char *command_line = trim(line);

        if (!*command_line)
            continue;

        history_add(command_line);

        char *parts[SHELL_MAX_PIPELINE_STAGES];
        int count = split_pipeline(command_line, parts);

        if (count < 1) {
            fprintf(stderr, "shell: invalid pipeline or too many stages\n");
            continue;
        }

        ParsedCommand commands[SHELL_MAX_PIPELINE_STAGES] = {0};
        ParsedCommand *stages[SHELL_MAX_PIPELINE_STAGES];
        int parsed = 0;
        int failed = 0;

        for (int i = 0; i < count; i++) {
            parts[i] = trim(parts[i]);

            if (!*parts[i] || parse_command(parts[i], &commands[i]) != 0) {
                fprintf(stderr, "shell: invalid command syntax\n");
                failed = 1;
                break;
            }

            parsed++;
            stages[i] = &commands[i];
        }

        if (!failed && count == 1) {
            ParsedCommand *cmd = &commands[0];

            if (strcmp(cmd->argv[0], "exit") == 0) {
                for (int i = 0; i < parsed; i++)
                    free_parsed_command(&commands[i]);
                break;
            } else if (strcmp(cmd->argv[0], "cd") == 0) {
                const char *path = cmd->argv[1];

                if (!path)
                    path = getenv("HOME");

                if (!path || chdir(path) != 0)
                    perror("cd");
            } else if (strcmp(cmd->argv[0], "history") == 0) {
                history_print();
            } else {
                executor_run(cmd);
            }
        } else if (!failed) {
            pipeline_execute(stages, count);
        }

        for (int i = 0; i < parsed; i++)
            free_parsed_command(&commands[i]);
    }

    putchar('\n');
    return 0;
}