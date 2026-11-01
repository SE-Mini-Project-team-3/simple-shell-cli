#include "parser.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

static char *copy_text(const char *start, size_t length)
{
    char *copy = malloc(length + 1);

    if (copy == NULL)
        return NULL;

    memcpy(copy, start, length);
    copy[length] = '\0';

    return copy;
}

static bool is_builtin_command(const char *name)
{
    return strcmp(name, "cd") == 0 ||
           strcmp(name, "pwd") == 0 ||
           strcmp(name, "echo") == 0 ||
           strcmp(name, "exit") == 0 ||
           strcmp(name, "help") == 0 ||
           strcmp(name, "history") == 0;
}

int parse_command(char *input, ParsedCommand *cmd)
{
    if (input == NULL || cmd == NULL)
        return -1;

    memset(cmd, 0, sizeof(*cmd));

    /* Enforce the input limit even when called directly. */
    if (strlen(input) > SHELL_MAX_INPUT)
        return -1;

    char *p = input;
    int argc = 0;

    while (*p != '\0') {
        while (isspace((unsigned char)*p))
            p++;

        if (*p == '\0')
            break;

        if (*p == '<' || *p == '>') {
            char op = *p++;
            int append = 0;

            if (op == '>' && *p == '>') {
                append = 1;
                p++;
            }

            while (isspace((unsigned char)*p))
                p++;

            if (*p == '\0' || *p == '<' || *p == '>')
                goto error;

            char *start = p;

            while (*p != '\0' &&
                   !isspace((unsigned char)*p) &&
                   *p != '<' && *p != '>')
                p++;

            char *filename = copy_text(start, (size_t)(p - start));

            if (filename == NULL)
                goto error;

            if (op == '<') {
                if (cmd->input_file != NULL) {
                    free(filename);
                    goto error;
                }

                cmd->input_file = filename;
            } else {
                if (cmd->output_file != NULL) {
                    free(filename);
                    goto error;
                }

                cmd->output_file = filename;
                cmd->append_mode = append;
            }

            continue;
        }

        if (argc >= SHELL_MAX_ARGS)
            goto error;

        char *arg = malloc(strlen(p) + 1);

        if (arg == NULL)
            goto error;

        size_t len = 0;
        char quote = '\0';

        while (*p != '\0') {
            if (quote != '\0') {
                if (*p == quote) {
                    quote = '\0';
                    p++;
                } else {
                    arg[len++] = *p++;
                }
            } else if (*p == '\'' || *p == '"') {
                quote = *p++;
            } else if (isspace((unsigned char)*p) ||
                       *p == '<' || *p == '>') {
                break;
            } else {
                arg[len++] = *p++;
            }
        }

        if (quote != '\0') {
            free(arg);
            goto error;
        }

        arg[len] = '\0';
        cmd->argv[argc++] = arg;
        cmd->argv[argc] = NULL;
    }

    if (argc == 0) {
        cmd->argv[0] = NULL;
        cmd->is_builtin = false;
        return 0;
    }

    cmd->is_builtin = is_builtin_command(cmd->argv[0]);

    return 0;

error:
    free_parsed_command(cmd);
    return -1;
}

void free_parsed_command(ParsedCommand *cmd)
{
    if (cmd == NULL)
        return;

    for (int i = 0; i < SHELL_MAX_ARGS + 1; i++) {
        free(cmd->argv[i]);
        cmd->argv[i] = NULL;
    }

    free(cmd->input_file);
    free(cmd->output_file);

    cmd->input_file = NULL;
    cmd->output_file = NULL;
    cmd->append_mode = false;
    cmd->is_builtin = false;
    cmd->is_background = false;
}
