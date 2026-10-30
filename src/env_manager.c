#include "env_manager.h"

int env_set(const char *name, const char *value)
{
    (void)name;
    (void)value;
    return -1;
}

int env_unset(const char *name)
{
    (void)name;
    return -1;
}

char **env_list(void)
{
    return 0;
}
