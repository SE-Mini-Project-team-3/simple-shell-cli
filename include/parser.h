#ifndef PARSER_H
#define PARSER_H

#include "shell.h"

int parse_command(char *input, ParsedCommand *cmd);
void free_parsed_command(ParsedCommand *cmd);

#endif
