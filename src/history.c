#include "history.h"
#include "shell.h"

#include <stdio.h>
#include <string.h>

static char entries[SHELL_MAX_HISTORY][SHELL_MAX_INPUT];
static int entry_count;
static int next_entry;

void history_add(const char *command_line)
{
    if (!command_line || !*command_line)
        return;

    snprintf(entries[next_entry], sizeof(entries[next_entry]), "%s",
             command_line);

    next_entry = (next_entry + 1) % SHELL_MAX_HISTORY;

    if (entry_count < SHELL_MAX_HISTORY)
        entry_count++;
}

void history_print(void)
{
    int first = (next_entry - entry_count + SHELL_MAX_HISTORY)
                % SHELL_MAX_HISTORY;

    for (int i = 0; i < entry_count; i++) {
        int index = (first + i) % SHELL_MAX_HISTORY;
        printf("%d  %s\n", i + 1, entries[index]);
    }
}