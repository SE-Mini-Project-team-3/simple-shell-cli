#include <sys/types.h>

pid_t __gcov_fork(void);
int __gcov_execvp(const char *file, char *const argv[]);
void gcov_coverage_exit(int status);
