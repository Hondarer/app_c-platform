/* process.c の Linux static 関数へアクセスするテスト用アクセサーです。 */
#include "process.inject.h"

#if defined(PLATFORM_LINUX)

uint64_t test_process_monotonic_ms(void)
{
    return monotonic_ms();
}

char *test_process_string_duplicate(const char *text)
{
    return string_duplicate(text);
}

size_t test_process_env_key_len(const char *entry)
{
    return env_key_len(entry);
}

int test_process_env_key_matches(const char *entry, const char *key, size_t key_len)
{
    return env_key_matches(entry, key, key_len);
}

int test_process_set_env_entry(char **envp, size_t capacity, const char *entry)
{
    return set_env_entry(envp, capacity, entry);
}

char **test_process_build_environment(char *const *overrides)
{
    return build_environment(overrides);
}

void test_process_free_envp(char **envp)
{
    free_envp(envp);
}

const char *test_process_find_env_value(char *const *envp, const char *key)
{
    return find_env_value(envp, key);
}

int test_process_setup_child_stdio_one(const com_util_process_stdio *spec, int target_fd, int null_flags)
{
    return setup_child_stdio_one(spec, target_fd, null_flags);
}

int test_process_setup_child_stdio(const com_util_process_options *options)
{
    return setup_child_stdio(options);
}

void test_process_exec_with_path(char *const *argv, char *const *envp)
{
    exec_with_path(argv, envp);
}

int test_process_run_child(const com_util_process_options *options, char *const *envp)
{
    return run_child_process(options, envp);
}

#endif /* PLATFORM_LINUX */
