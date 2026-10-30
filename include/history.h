#ifndef HISTORY_H
#define HISTORY_H

void history_add(const char *command_line);
void history_print(void);

int history_count(void);
const char *history_get(int position);
unsigned long history_get_number(int position);
void history_clear(void);

#endif
