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

    if (strlen(input) > SHELL_MAX_INPUT ||
        !has_valid_syntax(input)) {
        return -1;
    }

    memset(cmd, 0, sizeof(*cmd));

    char *read = input;
    char *write = input;
    int argc = 0;
    char quote = '\0';
    bool in_token = false;

    while (*read != '\0') {
        if (quote == '\0' &&
            (*read == '"' || *read == '\'')) {
            if (!in_token) {
                if (argc >= SHELL_MAX_ARGS) {
                    memset(cmd, 0, sizeof(*cmd));
                    return -1;
                }

                cmd->argv[argc++] = write;
                in_token = true;
            }

            quote = *read++;
            continue;
        }

        if (quote != '\0' && *read == quote) {
            quote = '\0';
            read++;
            continue;
        }

        if (quote == '\0' &&
            isspace((unsigned char)*read)) {
            if (in_token) {
                *write++ = '\0';
                in_token = false;
            }
            read++;
            continue;
        }

        if (!in_token) {
            if (argc >= SHELL_MAX_ARGS) {
                memset(cmd, 0, sizeof(*cmd));
                return -1;
            }

            cmd->argv[argc++] = write;
            in_token = true;
        }

        *write++ = *read++;
    }

    if (in_token) {
        *write = '\0';
    }

    cmd->argv[argc] = NULL;
    cmd->is_builtin =
        argc > 0 && is_builtin_command(cmd->argv[0]);

    return 0;
}


void free_parsed_command(ParsedCommand *cmd)
{
    if (cmd != NULL) {
        memset(cmd, 0, sizeof(*cmd));
    }
}
