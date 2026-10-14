#include "history.h"
#include "shell.h"

#include <stdio.h>
#include <string.h>

static int failures = 0;

#define CHECK(cond, message)                               \
    do                                                      \
    {                                                        \
        if (!(cond))                                         \
        {                                                    \
            printf("FAIL: %s\n", message);                   \
            failures++;                                       \
        }                                                       \
        else                                                      \
        {                                                          \
            printf("PASS: %s\n", message);                          \
        }                                                               \
    } while (0)

static void test_add_and_retrieve(void)
{
    history_clear();
    history_add("ls -la");
    history_add("pwd");

    CHECK(history_count() == 2, "two commands are recorded");
    CHECK(strcmp(history_get(0), "ls -la") == 0, "first entry stores the oldest command");
    CHECK(strcmp(history_get(1), "pwd") == 0, "second entry stores the newest command");
}

static void test_numbering(void)
{
    history_clear();
    history_add("echo one");
    history_add("echo two");
    history_add("echo three");

    CHECK(history_get_number(0) == 1, "first command is numbered 1");
    CHECK(history_get_number(1) == 2, "second command is numbered 2");
    CHECK(history_get_number(2) == 3, "third command is numbered 3");
}

static void test_capacity_eviction(void)
{
    history_clear();

    char command[32];
    for (int i = 1; i <= SHELL_MAX_HISTORY + 5; i++)
    {
        snprintf(command, sizeof(command), "cmd%d", i);
        history_add(command);
    }

    CHECK(history_count() == SHELL_MAX_HISTORY, "history never exceeds the maximum size");
    CHECK(strcmp(history_get(0), "cmd6") == 0, "oldest entries are evicted once the limit is exceeded");
    CHECK(strcmp(history_get(SHELL_MAX_HISTORY - 1), "cmd55") == 0, "the most recent command is retained");
    CHECK(history_get_number(0) == 6, "numbering keeps counting up instead of resetting after eviction");
}

static void test_ignores_empty_input(void)
{
    history_clear();
    history_add("");
    history_add(NULL);

    CHECK(history_count() == 0, "empty or null commands are not recorded");
}

int main(void)
{
    test_add_and_retrieve();
    test_numbering();
    test_capacity_eviction();
    test_ignores_empty_input();

    if (failures == 0)
    {
        printf("\nAll history tests passed.\n");
        return 0;
    }

    printf("\n%d history test(s) failed.\n", failures);
    return 1;
}
