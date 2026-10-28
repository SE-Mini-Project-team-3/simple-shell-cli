#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "shell.h"
#include "executor.h"
#include "parser.h" // Include your team's parser header

#define MAX_LINE 1024

int main(void) {
    char input[MAX_LINE];
    
    // 1. Initialize shell signals (e.g., handling SIGINT)
    init_shell_signals();

    printf("Simple Shell CLI - Ready\n");

    while (1) {
        // 2. Reap any finished background jobs before showing prompt
        reap_background_jobs();

        // Print prompt
        printf("shell> ");
        fflush(stdout);

        // Read user input
        if (fgets(input, sizeof(input), stdin) == NULL) {
            printf("\n");
            break; // Handle EOF (Ctrl+D)
        }

        // Remove trailing newline
        input[strcspn(input, "\n")] = 0;

        // Skip empty lines
        if (strlen(input) == 0) {
            continue;
        }

        // Handle exit built-in shortcut directly if desired
        if (strcmp(input, "exit") == 0) {
            break;
        }

        // 3. Parse the command using the parser module (F1)
        ParsedCommand cmd;
        // Note: adjust 'parse_command' function name based on Rekha's parser.h implementation
        if (parse_command(input, &cmd) != 0) {
            fprintf(stderr, "shell: parse error\n");
            continue;
        }

        // Skip if command is empty after parsing
        if (cmd.argv[0] == NULL) {
            continue;
        }

        // 4. Run the command using your executor module (F2)
        executor_run(&cmd);
    }

    return 0;
}