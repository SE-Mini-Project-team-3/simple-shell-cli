#ifndef PIPELINE_H
#define PIPELINE_H

#include "shell.h"

int pipeline_execute(ParsedCommand *stages[], int stage_count);

#endif
