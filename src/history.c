#include "history.h"
#include "shell.h"

#include <stdio.h>
#include <string.h>

typedef struct {
    char command[SHELL_MAX_INPUT];
    unsigned long number;
} HistoryEntry;

static HistoryEntry entries[SHELL_MAX_HISTORY];
static int count = 0;
static unsigned long next_number = 1;

void history_add(const char *command_line)
{
    if (command_line == NULL || command_line[0] == '\0')
    {
        return;
    }

    if (count == SHELL_MAX_HISTORY)
    {
        memmove(&entries[0], &entries[1], (size_t)(SHELL_MAX_HISTORY - 1) * sizeof(HistoryEntry));
        count--;
    }

    HistoryEntry *slot = &entries[count];
    strncpy(slot->command, command_line, SHELL_MAX_INPUT - 1);
    slot->command[SHELL_MAX_INPUT - 1] = '\0';
    slot->number = next_number++;
    count++;
}

void history_print(void)
{
    for (int i = 0; i < count; i++)
    {
        printf("%5lu  %s\n", entries[i].number, entries[i].command);
    }
}

int history_count(void)
{
    return count;
}

const char *history_get(int position)
{
    if (position < 0 || position >= count)
    {
        return NULL;
    }

    return entries[position].command;
}

unsigned long history_get_number(int position)
{
    if (position < 0 || position >= count)
    {
        return 0;
    }

    return entries[position].number;
}

void history_clear(void)
{
    count = 0;
    next_number = 1;
}
