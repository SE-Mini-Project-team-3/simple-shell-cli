#include "env_manager.h"

#include <stdio.h>
#include <stdlib.h>

extern char **environ;

static int is_valid_name(const char *name)
{
    if (name == NULL || name[0] == '\0')
    {
        return 0;
    }

    for (const char *p = name; *p != '\0'; p++)
    {
        if (*p == '=')
        {
            return 0;
        }
    }

    return 1;
}

int env_set(const char *name, const char *value)
{
    if (!is_valid_name(name) || value == NULL)
    {
        return -1;
    }

    return setenv(name, value, 1) == 0 ? 0 : -1;
}

int env_unset(const char *name)
{
    if (!is_valid_name(name))
    {
        return -1;
    }

    return unsetenv(name) == 0 ? 0 : -1;
}

char **env_list(void)
{
    return environ;
}

void env_print(void)
{
    for (char **entry = environ; entry != NULL && *entry != NULL; entry++)
    {
        printf("%s\n", *entry);
    }
}
