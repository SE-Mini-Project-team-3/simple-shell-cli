#ifndef SHELL_H
#define SHELL_H

#include <stdbool.h>

#define SHELL_MAX_ARGS 64
#define SHELL_MAX_INPUT 1024
#define SHELL_MAX_PIPELINE_STAGES 5
#define SHELL_MAX_HISTORY 50

typedef struct {
    char *argv[SHELL_MAX_ARGS + 1];
    bool is_builtin;
    char *input_file;
    char *output_file;
    bool append_mode;
    bool is_background;
} ParsedCommand;

#endif
