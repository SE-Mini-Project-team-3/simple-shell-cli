#ifndef EXECUTOR_H
#define EXECUTOR_H

#include "shell.h"

int executor_run(const ParsedCommand *cmd);
void reap_background_jobs(void);
void init_shell_signals(void);

#endif