#ifndef ENV_MANAGER_H
#define ENV_MANAGER_H

int env_set(const char *name, const char *value);
int env_unset(const char *name);
char **env_list(void);
void env_print(void);

#endif
