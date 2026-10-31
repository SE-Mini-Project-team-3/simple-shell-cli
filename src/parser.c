#include <ctype.h>
#include <string.h>
#include "parser.h"

static bool has_valid_syntax(const char *input)
{
    char quote = '\0';

    for (size_t i = 0; input[i] != '\0'; i++) {
        if ((input[i] == '"' || input[i] == '\'') &&
            (quote == '\0' || quote == input[i])) {
            quote = (quote == '\0') ? input[i] : '\0';
            continue;
        }

        if (input[i] == '>' && quote == '\0') {
            if (input[i + 1] == '>') {
                i++;
            }

            size_t j = i + 1;
            while (input[j] != '\0' &&
                   isspace((unsigned char)input[j])) {
                j++;
            }

            if (input[j] == '\0' ||
                input[j] == '<' || input[j] == '>') {
                return false;
            }
        }

        if (input[i] == '<' && quote == '\0') {
            size_t j = i + 1;
            while (input[j] != '\0' &&
                   isspace((unsigned char)input[j])) {
                j++;
            }

            if (input[j] == '\0' ||
                input[j] == '<' || input[j] == '>') {
                return false;
            }
        }
    }

    return quote == '\0';
}

static bool is_builtin_command(const char *name)
{
    return strcmp(name, "cd") == 0 ||
           strcmp(name, "pwd") == 0 ||
           strcmp(name, "echo") == 0 ||
           strcmp(name, "exit") == 0 ||
           strcmp(name, "help") == 0;
}

int parse_command(char *input, ParsedCommand *cmd)
{
    if (input == NULL || cmd == NULL) {
        return -1;
    }

    if (strlen(input) > SHELL_MAX_INPUT) {
        return -1;
    }

    if (!has_valid_syntax(input)) {
        return -1;
    }

    memset(cmd, 0, sizeof(*cmd));

    char *p = input;
    int argc = 0;

    while (*p != '\0') {
        while (*p != '\0' && isspace((unsigned char)*p)) {
            p++;
        }

        if (*p == '\0') {
            break;
        }

        if (argc >= SHELL_MAX_ARGS) {
            memset(cmd, 0, sizeof(*cmd));
            return -1;
        }

        cmd->argv[argc++] = p;

        while (*p != '\0' && !isspace((unsigned char)*p)) {
            p++;
        }

        if (*p != '\0') {
            *p = '\0';
            p++;
        }
    }

    if (argc == 0) {
        cmd->argv[0] = NULL;
        cmd->is_builtin = false;
        return 0;
    }

    cmd->argv[argc] = NULL;

    if (argc > 0) {
        cmd->is_builtin = is_builtin_command(cmd->argv[0]);
    }

    return 0;
}

void free_parsed_command(ParsedCommand *cmd)
{
    if (cmd != NULL) {
        memset(cmd, 0, sizeof(*cmd));
    }
}
