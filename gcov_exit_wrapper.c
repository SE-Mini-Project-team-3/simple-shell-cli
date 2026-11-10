#include <unistd.h>
#include <gcov.h>

void gcov_coverage_exit(int status)
{
    __gcov_dump();
    _exit(status);
}
