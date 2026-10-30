#include "env_manager.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define TEST_VAR "SIMPLE_SHELL_TEST_VAR"

static int failures = 0;

#define CHECK(cond, message)                \
    do                                        \
    {                                          \
        if (!(cond))                           \
        {                                       \
            printf("FAIL: %s\n", message);      \
            failures++;                          \
        }                                           \
        else                                         \
        {                                             \
            printf("PASS: %s\n", message);              \
        }                                                  \
    } while (0)

static void test_set_and_get(void)
{
    CHECK(env_set(TEST_VAR, "hello") == 0, "env_set succeeds for a valid name/value");

    const char *value = getenv(TEST_VAR);
    CHECK(value != NULL && strcmp(value, "hello") == 0, "the variable is visible via getenv after env_set");

    env_unset(TEST_VAR);
}

static void test_set_overwrites(void)
{
    env_set(TEST_VAR, "first");
    env_set(TEST_VAR, "second");

    const char *value = getenv(TEST_VAR);
    CHECK(value != NULL && strcmp(value, "second") == 0, "env_set overwrites an existing value");

    env_unset(TEST_VAR);
}

static void test_unset(void)
{
    env_set(TEST_VAR, "hello");
    CHECK(env_unset(TEST_VAR) == 0, "env_unset succeeds");
    CHECK(getenv(TEST_VAR) == NULL, "the variable is gone after env_unset");
}

static void test_rejects_invalid_names(void)
{
    CHECK(env_set(NULL, "value") == -1, "env_set rejects a NULL name");
    CHECK(env_set("", "value") == -1, "env_set rejects an empty name");
    CHECK(env_set("BAD=NAME", "value") == -1, "env_set rejects a name containing '='");
    CHECK(env_unset(NULL) == -1, "env_unset rejects a NULL name");
}

static void test_list_contains_set_variable(void)
{
    env_set(TEST_VAR, "listed");

    int found = 0;
    for (char **entry = env_list(); entry != NULL && *entry != NULL; entry++)
    {
        if (strncmp(*entry, TEST_VAR "=", strlen(TEST_VAR) + 1) == 0)
        {
            found = 1;
            break;
        }
    }

    CHECK(found, "env_list includes a variable set via env_set");

    env_unset(TEST_VAR);
}

static void test_child_process_inherits_updates(void)
{
    env_set(TEST_VAR, "inherited");

    pid_t pid = fork();
    if (pid == 0)
    {
        const char *value = getenv(TEST_VAR);
        _exit(value != NULL && strcmp(value, "inherited") == 0 ? 0 : 1);
    }

    int status = 0;
    waitpid(pid, &status, 0);
    CHECK(WIFEXITED(status) && WEXITSTATUS(status) == 0, "a forked child process inherits environment updates");

    env_unset(TEST_VAR);
}

int main(void)
{
    test_set_and_get();
    test_set_overwrites();
    test_unset();
    test_rejects_invalid_names();
    test_list_contains_set_variable();
    test_child_process_inherits_updates();

    if (failures == 0)
    {
        printf("\nAll env_manager tests passed.\n");
        return 0;
    }

    printf("\n%d env_manager test(s) failed.\n", failures);
    return 1;
}
