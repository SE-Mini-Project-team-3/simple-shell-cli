#include <ctype.h>
#include <string.h>
#include "parser.h"

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
